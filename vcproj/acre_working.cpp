#include "NamedPipeServer.h"
#include "TextMessage.h"
#include <sddl.h>
#include "Log.h"
#include "Engine.h"

CNamedPipeServer::CNamedPipeServer(void) {
	this->setConnectedWrite(false);
	this->setConnectedRead(false);
	this->setPipeHandleWrite(INVALID_HANDLE_VALUE);
	this->setPipeHandleRead(INVALID_HANDLE_VALUE);
	this->setShuttingDown(false);


}

CNamedPipeServer::~CNamedPipeServer(void) {
	this->shutdown();
}

ACRE_RESULT CNamedPipeServer::initialize() {
	HANDLE writeHandle, readHandle;




	SECURITY_DESCRIPTOR SDWrite;
	InitializeSecurityDescriptor(&SDWrite, SECURITY_DESCRIPTOR_REVISION);
	SetSecurityDescriptorDacl(&SDWrite, TRUE, NULL, FALSE);
	SECURITY_ATTRIBUTES SAWrite;
	SAWrite.nLength = sizeof(SAWrite);
	SAWrite.lpSecurityDescriptor = &SDWrite;
	SAWrite.bInheritHandle = TRUE;

	// open our pipe handle, then kick up a thread to monitor it and add shit to our queue
	// this end LISTENS and CREATES the pipe
	LOG("Opening game pipe...");
	BOOL tryAgain = TRUE;

	while (tryAgain) {
		writeHandle = CreateNamedPipeA(
			"\\\\.\\pipe\\acre_comm_pipe_fromTS", // name of the pipe
			PIPE_ACCESS_DUPLEX,
			PIPE_TYPE_MESSAGE |		// message-type pipe 
			PIPE_READMODE_MESSAGE,	// send data as message
			PIPE_UNLIMITED_INSTANCES,
			4096, // no outbound buffer
			4096, // no inbound buffer
			0, // use default wait time
			&SAWrite // use default security attributes
			);
		if (writeHandle == INVALID_HANDLE_VALUE) {
			char errstr[1024];

			_snprintf_s(errstr, sizeof(errstr), "Conflicting game write pipe detected, could not create pipe!\nERROR CODE: %d", GetLastError());
			int ret = MessageBoxA(NULL, errstr, "ACRE Error", MB_RETRYCANCEL | MB_ICONEXCLAMATION);
			if (ret != IDRETRY) {
				tryAgain = FALSE;
				TerminateProcess(GetCurrentProcess(), 0);
			}
		}
		else {
			tryAgain = FALSE;
		}
	}

	SECURITY_DESCRIPTOR SDRead;
	InitializeSecurityDescriptor(&SDRead, SECURITY_DESCRIPTOR_REVISION);
	SetSecurityDescriptorDacl(&SDRead, TRUE, NULL, FALSE);
	SECURITY_ATTRIBUTES SARead;
	SARead.nLength = sizeof(SARead);
	SARead.lpSecurityDescriptor = &SDRead;
	SARead.bInheritHandle = TRUE;

	tryAgain = TRUE;
	while (tryAgain) {
		readHandle = CreateNamedPipeA(
			"\\\\.\\pipe\\acre_comm_pipe_toTS", // name of the pipe
			PIPE_ACCESS_DUPLEX,
			PIPE_TYPE_MESSAGE |		// message-type pipe 
			PIPE_NOWAIT |			// Depricated but fuck it, it is simpler.
			PIPE_READMODE_MESSAGE,	// send data as message
			PIPE_UNLIMITED_INSTANCES,
			4096, // no outbound buffer
			4096, // no inbound buffer
			0, // use default wait time
			&SARead // use default security attributes
			);
		if (readHandle == INVALID_HANDLE_VALUE) {
			char errstr[1024];

			_snprintf_s(errstr, sizeof(errstr), "Conflicting game read pipe detected, could not create pipe!\nERROR CODE: %d", GetLastError());
			int ret = MessageBoxA(NULL, errstr, "ACRE Error", MB_RETRYCANCEL | MB_ICONEXCLAMATION);
			if (ret != IDRETRY) {
				tryAgain = FALSE;
				TerminateProcess(GetCurrentProcess(), 0);
			}
		}
		else {
			tryAgain = FALSE;
		}
	}

	this->setPipeHandleRead(readHandle);
	this->setPipeHandleWrite(writeHandle);

	LOG("Game pipe opening successful. [%d & %d]", this->getPipeHandleRead(), this->getPipeHandleWrite());

	this->m_sendThread = std::thread(&CNamedPipeServer::sendLoop, this);
	this->m_readThread = std::thread(&CNamedPipeServer::readLoop, this);
	return ACRE_OK;
}

ACRE_RESULT CNamedPipeServer::shutdown(void) {

	this->setConnectedWrite(FALSE);
	this->setConnectedRead(FALSE);
	this->setShuttingDown(true);
	if (this->m_PipeHandleRead != INVALID_HANDLE_VALUE) {
		CloseHandle(this->m_PipeHandleRead);
		this->setPipeHandleRead(INVALID_HANDLE_VALUE);
	}

	if (this->m_PipeHandleWrite != INVALID_HANDLE_VALUE) {
		DeleteFileA("\\\\.\\pipe\\acre_comm_pipe_fromTS");
		CloseHandle(this->m_PipeHandleWrite);
		this->setPipeHandleWrite(INVALID_HANDLE_VALUE);
	}

	if (this->m_readThread.joinable()) {
		this->m_readThread.join();
	}
	if (this->m_sendThread.joinable()) {
		this->m_sendThread.join();
	}


	this->setShuttingDown(false);

	return ACRE_OK;
}

ACRE_RESULT CNamedPipeServer::sendLoop() {
	DWORD cbWritten, size;
	IMessage *msg;
	BOOL ret;
	clock_t lastTick, tick;

	while (!this->getShuttingDown()) {

		do {
			ConnectNamedPipe(this->m_PipeHandleWrite, NULL);
			if (GetLastError() == ERROR_PIPE_CONNECTED) {
				LOG("Client write connected");
				CEngine::getInstance()->getSoundEngine()->onClientGameConnected();
				this->setConnectedWrite(TRUE);
			}
			else {
				this->setConnectedWrite(FALSE);
				Sleep(1);
			}
		} while (GetLastError() != ERROR_PIPE_CONNECTED && !this->getShuttingDown());

		lastTick = clock() / CLOCKS_PER_SEC;
		while (this->getConnectedWrite()) {
			if (this->getShuttingDown())
				break;

			tick = clock() / CLOCKS_PER_SEC;
			if (tick - lastTick > (PIPE_TIMEOUT / 1000)) {
				LOG("No send message for %d seconds, disconnecting", (PIPE_TIMEOUT / 1000));
				this->setConnectedWrite(FALSE);
				this->setConnectedRead(FALSE);
				break;
			}

			if (this->m_sendQueue.try_pop(msg)) {
				if (msg) {
					lastTick = clock() / CLOCKS_PER_SEC;
					size = (DWORD)strlen((char *)msg->getData()) + 1;
					if (size > 3) {
						// send it and free it
						//LOCK(this);
						this->lock();
						ret = WriteFile(
							this->m_PipeHandleWrite,     // pipe handle 
							msg->getData(),					// message 
							size,					// message length 
							&cbWritten,             // bytes written 
							NULL);                  // not overlapped 
						this->unlock();

						if (!ret) {
							LOG("WriteFile failed, [%d]", GetLastError());
						}
					}
					delete msg;
				}
			}
			Sleep(1);
		}




		Sleep(1);
	}
	TRACE("Sending thread terminating");

	return ACRE_OK;
}

ACRE_RESULT CNamedPipeServer::readLoop() {
	DWORD cbRead;
	IMessage *msg;
	BOOL ret;
	clock_t lastTick, tick;
	char *mBuffer;


	mBuffer = (char *)LocalAlloc(LMEM_FIXED, BUFSIZE);
	if (!mBuffer) {
		LOG("LocalAlloc() failed: %d", GetLastError());
	}

	this->validTSServers.insert(std::string("mJuldN/TLy76bFc+1x48Lm2a+Y4=")); // United Operations
	this->validTSServers.insert(std::string("1VGxwrAxbdku8qssecdmhDV3aWE=")); // UST101 columdrum
	this->validTSServers.insert(std::string("UxJNKz23toFvh5xN/lWOSDmPiX0=")); // AAF deFrager
	this->validTSServers.insert(std::string("1RkVgLpjBNKDa6cKNdnr/2kurhI=")); // BTC Giallustio (ACE)
	this->validTSServers.insert(std::string("QrSN53oK0R8ujeStVrVPYwtLnB8=")); // ACE Rocko (ACE)
	this->validTSServers.insert(std::string("fSvR6n8G1n7JouQFe/adu6M03vA=")); // TI Diogo
	this->validTSServers.insert(std::string("2gqqEEUHL+ISOmFRFclP1oT0iwI=")); // Tupolov
	this->validTSServers.insert(std::string("T44wbWop9P47POYaWThI49DCkZc=")); // Armatec
	this->validTSServers.insert(std::string("Hbzu8Q8Thcu8UsFBBX0p5plC4QM=")); // JP
	this->validTSServers.insert(std::string("t4BAGwauftNFqnQ18UjUPXpboyc=")); // Pers (Swedish Defense Force?)

	while (!this->getShuttingDown()) {
		this->checkServer();
		ret = ConnectNamedPipe(this->m_PipeHandleRead, NULL);
		if (GetLastError() == ERROR_PIPE_CONNECTED) {
			LOG("Client read connected");
			CEngine::getInstance()->getSoundEngine()->onClientGameConnected();
			this->setConnectedRead(TRUE);
		}
		else {
			this->setConnectedRead(FALSE);
			Sleep(1);

			continue;
		}
		lastTick = clock() / CLOCKS_PER_SEC;
		while (this->getConnectedRead()) {
			this->checkServer();
			if (this->getShuttingDown())
				break;

			tick = clock() / CLOCKS_PER_SEC;
			//LOG("[%d] - [%d] = [%d] vs. [%d]", tick, lastTick, (tick - lastTick),(PIPE_TIMEOUT / 1000));
			if (tick - lastTick > (PIPE_TIMEOUT / 1000)) {
				LOG("No read message for %d seconds, disconnecting", (PIPE_TIMEOUT / 1000));

				break;
			}
			ret = FALSE;
			do {
				ret = ReadFile(this->m_PipeHandleRead, mBuffer, BUFSIZE, &cbRead, NULL);
				if (!ret && GetLastError() != ERROR_MORE_DATA)
					break;
				// handle the packet and run it
				mBuffer[cbRead] = 0x00;
				//LOG("READ: %s", (char *)mBuffer);
				msg = new CTextMessage((char *)mBuffer, cbRead);
				//TRACE("got and parsed message [%s]", msg->getData());
				if (msg && msg->getProcedureName()) {

					CEngine::getInstance()->getRpcEngine()->runProcedure(this, msg);

					lastTick = clock() / CLOCKS_PER_SEC;
					//TRACE("tick [%d], [%s]",lastTick, msg->getData());
				}
				// wait 1ms for new msg so we dont hog cpu cycles
			} while (!ret);

			//ret = ConnectNamedPipe(this->getPipeHandle(), NULL);	
			Sleep(1);
		}
		this->setConnectedWrite(FALSE);
		this->setConnectedRead(FALSE);
		FlushFileBuffers(this->m_PipeHandleRead);
		ret = DisconnectNamedPipe(this->m_PipeHandleRead);
		FlushFileBuffers(this->m_PipeHandleWrite);
		ret = DisconnectNamedPipe(this->m_PipeHandleWrite);

		CEngine::getInstance()->getSoundEngine()->onClientGameDisconnected();
		LOG("Client disconnected");


		// Clear the send queue since client disconnected
		this->m_sendQueue.clear();

		// send an event that we have disconnected 
		if (CEngine::getInstance()->getExternalServer()->getConnected()) {
			CEngine::getInstance()->getExternalServer()->sendMessage(
				CTextMessage::formatNewMessage("ext_reset",
				"%d,",
				CEngine::getInstance()->getSelf()->getId()
				)
				);
		}

		Sleep(1);
	}

	if (mBuffer)
		LocalFree(mBuffer);

	TRACE("Receiving thread terminating");

	return ACRE_OK;
}

ACRE_RESULT CNamedPipeServer::sendMessage(IMessage *message) {
	if (message) {
		TRACE("sending [%s]", message->getData());
		this->m_sendQueue.push(message);
		return ACRE_OK;
	}
	else {
		return ACRE_ERROR;
	}
}

ACRE_RESULT CNamedPipeServer::checkServer(void) {
	std::string uniqueId = CEngine::getInstance()->getClient()->getUniqueId();
	if (uniqueId != "" && this->validTSServers.find(uniqueId) == this->validTSServers.end()) {
		MessageBoxA(NULL, "This server is NOT registered for ACRE2 testing! Please remove the plugin! Teamspeak will now close.", "ACRE Error", MB_OK | MB_ICONEXCLAMATION);
		TerminateProcess(GetCurrentProcess(), 0);
	}
	return ACRE_OK;
}