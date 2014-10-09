#pragma once

#define LOW_INTEGRITY_SDDL_SACL_W 			L"S:(ML;;NW;;;LW)"
#define UNTRUSTED_INTEGRITY_SDDL_SACL_W 	L"S:(ML;;NW;;;S-1-16-0)"

/*! 
 * Named pipe implementation class. 
 */
#include <jio/xplatform.hpp>
#include <jio/exception.hpp>
#include <jio/transports/common.hpp>


#include <thread>
#include <string>
#include <iostream>

#include "comdef.h"

namespace jio {
	namespace transports {
		using namespace jio::messaging;

		/*!
		*	Defines settings for a win32 named pipe instance.
		*/
		typedef struct named_pipe_settings {
			uint32_t timeout;
			uint32_t max_buffer_in;
			uint32_t max_buffer_out;
			bool use_completion_port;
			std::wstring application_name;
			named_pipe_settings() {
				timeout = 1000;
				max_buffer_in = 4096;
				max_buffer_out = 4096;
				use_completion_port = true;
				application_name = L"";
			}
			named_pipe_settings(const named_pipe_settings & in) {
				timeout = in.timeout;
				max_buffer_in = in.max_buffer_in;
				max_buffer_out = in.max_buffer_out;
				use_completion_port = in.use_completion_port;
				application_name = in.application_name;
			}
		} named_pipe_settings;
		
		/*!
		*	implementation of base_pipe and i_transport for a win32 named pipe.
		*	This object is NOT thread safe.
		*/
		class named_pipe :
			public base_pipe {
		public:
			/*!
			*	Initialize named pipe implementation on the pipe of pipename. This will create the pipe and begin listening if its the server.
			*	This function is NOT thread safe.
			*
			* 	@param [in] pipename The fully qualified path of the named pipe to call.
			* 	@param [in] settings Settings to define this named pipe server handle instance, if applicable.
			*	@param [in] is_server Boolean on whether this is a client or server instance of the class.
			**/
			named_pipe(const std::string & pipename, named_pipe_settings settings, bool is_server = false) {
				_name = pipename;
				_server = is_server;
				_settings = settings;

				// initialize our handles appropriately
				_hPipe				= INVALID_HANDLE_VALUE;
				_hCompletionPort	= INVALID_HANDLE_VALUE;

				// create the server here and flag us as already opened
				if (_server) {
					_server_create();
				} else {
					open();
				}

				// finally, allocate a generic read and write buffer
				_inputBuffer = new unsigned char[_settings.max_buffer_in];
				_outputBuffer = new unsigned char[_settings.max_buffer_in];
			}

			/*!
			*	Destructor for our named pipe win32 implementation. This function will perform the appropriate disconnections and cleanup.
			*	This function is NOT thread safe.
			*/
			~named_pipe() {
				close();
				delete _inputBuffer;
				delete _outputBuffer;
			}

			/*!
			*	Opens the current named pipe channel if this object instance was defined as a client. If it was a server, we just return true.
			*	This function is NOT thread safe.
			*/
			bool open(void) { 
				
				// Bail out if we are the server and just pretend we are always open
				if (_server) {
					return true;
				}

				// Client functionality
				// Connect to the pipe
				_hPipe = CreateFileA(
					_name.c_str(),			// pipe name 
					GENERIC_READ |			// read and write access 
					GENERIC_WRITE,
					0,						// no sharing 
					NULL,					// default security attributes
					OPEN_EXISTING,			// opens existing pipe 
					FILE_ATTRIBUTE_NORMAL,	// default attributes 
					NULL);					// no template file 
				
				if (_hPipe == INVALID_HANDLE_VALUE) {
					if (GetLastError() == ERROR_PIPE_BUSY) {
						// TODO: Eventually handle reconnects here
						throw EXCEPT_TEXT(jio::exception, GetLastError(), jio::xplatform::GetLastErrorAsString());
					}
					else {
						// real error
						throw EXCEPT_TEXT(jio::exception, GetLastError(), jio::xplatform::GetLastErrorAsString());
					}
				}

				return true;
			}

			/*!
			*	Closes the current named pipe instance. This includes closing all server handles. 
			*	This object is not reusable if a server.
			*	This function is NOT thread safe.
			*/
			void close(void) {
					if (is_server()) {
				if (_hPipe != INVALID_HANDLE_VALUE) {
						DisconnectNamedPipe(_hPipe);
						if (_settings.use_completion_port) {
							CloseHandle(_hCompletionPort);
						}
					}
				
					CloseHandle(_hPipe);
				}

				_hCompletionPort = INVALID_HANDLE_VALUE;
				_hPipe = INVALID_HANDLE_VALUE;
			}

			/*!
			*	Reads the next full message off the named pipe buffer.
			*	This implementation of the read function returns a new allocated instance of a message object. 
			*	This makes for large amounts of heap memory allocations.
			*	This function is NOT thread safe.
			*/
			message *  read(void) {
				// Read a message off the buffer
				BOOL fSuccess = FALSE;
				DWORD cbRead = 0;
				DWORD cbTotalRead = 0;

				fSuccess = ReadFile(_hPipe, _inputBuffer, _settings.max_buffer_in, &cbRead, NULL);
				cbTotalRead += cbRead;
				if (!fSuccess && GetLastError() != ERROR_MORE_DATA) {
					return nullptr;
				} else if (GetLastError() == ERROR_MORE_DATA) {
					while (GetLastError() == ERROR_MORE_DATA) {
						cbTotalRead += cbRead;
						fSuccess = ReadFile(_hPipe, _inputBuffer + cbTotalRead, _settings.max_buffer_in - cbTotalRead, &cbRead, NULL);
					}
				}
				
				return new message(_inputBuffer, cbTotalRead, this);
			}

			/*!
			*	Reads the next full message off the named pipe buffer.
			*	This implementation of the read function returns the read message, as allocated by the allocation factory.
			*	This function is NOT thread safe.
			*
			*	@param [in] factory The allocation factory to read the new message into.
			*/
			message *  read(i_message_factory *factory) {
				// Read a message off the buffer
				BOOL fSuccess = FALSE;
				DWORD cbRead = 0;
				DWORD cbTotalRead = 0;

				fSuccess = ReadFile(_hPipe, _inputBuffer, _settings.max_buffer_in, &cbRead, NULL);
				cbTotalRead += cbRead;
				if (!fSuccess && GetLastError() != ERROR_MORE_DATA) {
					return nullptr;
				}
				else if (GetLastError() == ERROR_MORE_DATA) {
					while (GetLastError() == ERROR_MORE_DATA) {
						cbTotalRead += cbRead;
						fSuccess = ReadFile(_hPipe, _inputBuffer + cbTotalRead, _settings.max_buffer_in - cbTotalRead, &cbRead, NULL);
					}
				}

				return factory->createMessage(_inputBuffer, cbTotalRead, this);
			}

			/*!
			*	Reads the next full message off the named pipe buffer.
			*	This implementation of the read function copies the data into the pre-allocated message. 
			*	It assumes proper data and lengths for the message. Truncates message if not enough space.
			*	This function is NOT thread safe.
			*
			*	@param [in] factory The allocation factory to read the new message into.
			*/
			message *  read(message & msg) {
				// Read a message off the buffer
				BOOL fSuccess = FALSE;
				DWORD cbRead = 0;
				DWORD cbTotalRead = 0;
				DWORD maxRead = 0;

				if (msg.length < 1 || msg.data == nullptr)
					throw EXCEPT_TEXT(jio::exception, E_INVALIDARG, std::string("Invalid message object provided."));
				
				maxRead = min(_settings.max_buffer_in, msg.length);
				fSuccess = ReadFile(_hPipe, _inputBuffer, maxRead, &cbRead, NULL);
				cbTotalRead += cbRead;
				if (!fSuccess && GetLastError() != ERROR_MORE_DATA) {
					return nullptr;
				}
				else if (GetLastError() == ERROR_MORE_DATA) {
					while (GetLastError() == ERROR_MORE_DATA && cbTotalRead < msg.length) {
						cbTotalRead += cbRead;
						fSuccess = ReadFile(_hPipe, _inputBuffer + cbTotalRead, maxRead - cbTotalRead, &cbRead, NULL);
					}
				}

				return &msg;
			}

			/*!
			*	Writes the next message to the named pipe buffer.
			*	This function is NOT thread safe.
			*
			*	@param [in] data the message object to send the data from. 
			*/
			uint32_t write(const message & data) {
				BOOL fSuccess = FALSE;
				DWORD cbWritten = 0, rcbWritten = 0;

				// write the full packet
				while (cbWritten < data.length) {
					fSuccess = WriteFile(
						_hPipe,                  // pipe handle 
						data.data+cbWritten,             // message 
						data.length-cbWritten,              // message length 
						&rcbWritten,             // bytes written 
						NULL);                  // not overlapped 
					cbWritten += rcbWritten;
					if (!fSuccess) {
						throw EXCEPT_TEXT(jio::exception, GetLastError(), jio::xplatform::GetLastErrorAsString());
					}
				}

				return cbWritten;
			}

			/*!
			*	Flush the pipe where applicable.
			*	This function is NOT thread safe.
			*/
			void flush(void) {
				FlushFileBuffers(_hPipe);
			}

			/*!
			*	Returns the settings used to create this pipe.
			*/
			const named_pipe_settings & named_pipe_settings() const { return _settings; }


//
// Stream operators
//

			/*!
			* Write operator for jio::messaging::message
			*/
			named_pipe & operator<<(const message &msg) {
				this->write(msg);

				return *this;
			}
			/*!
			* Read operator for jio::messaging::message
			*/
			friend message & operator<<(message & msg, named_pipe & me) {
				THROW_NOT_IMPL();

				return msg;
			}
			/*!
			* Write operator for std::string
			*/
			named_pipe & operator<<(const std::string &str) {
				this->write(message((unsigned char *)str.c_str(), str.size() + 1, this));

				return *this;
			}
			/*!
			* Read operator for std::string
			*/
			friend std::string & operator<<(std::string & str, named_pipe & me) {
				THROW_NOT_IMPL();

				return str;
			}

		protected:
			// Internal win32-specific properties are here.
			jio::transports::named_pipe_settings _settings;
			HANDLE _hPipe;
			HANDLE _hCompletionPort;
			std::thread _listenerThread;
			unsigned char *_inputBuffer;
			unsigned char *_outputBuffer;

		protected:
			/*!
			*	Internal function for creating a named pipe in win32 land
			*/
			bool _server_create(void) {

				// This SA creates a untrusted IL named pipe allow all
				SECURITY_ATTRIBUTES sa;
				sa.nLength = sizeof(SECURITY_ATTRIBUTES);
				sa.lpSecurityDescriptor = jio::transports::named_pipe::security::get_untrusted_sa();
				sa.bInheritHandle = FALSE;

				// Try to create the named pipe. On error, throw an except with the detailed error.
				if ((_hPipe = ::CreateNamedPipeA(
					_name.c_str(),
					PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
					PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
					PIPE_UNLIMITED_INSTANCES, 
					_settings.max_buffer_out,
					_settings.max_buffer_in,
					_settings.timeout,
					&sa)) == INVALID_HANDLE_VALUE) { 
					
					// free the security descriptor on error
					::LocalFree(sa.lpSecurityDescriptor);
					throw EXCEPT_TEXT(jio::exception, GetLastError(), jio::xplatform::GetLastErrorAsString());
				}

				// free the security desc. anyways
				::LocalFree(sa.lpSecurityDescriptor);

				// 
				// if enabled, create the completion port and utilize threaded listener methodology
				//
				if (_settings.use_completion_port) {
					
					if ((_hCompletionPort = ::CreateIoCompletionPort(
						_hPipe,
						NULL,
						0,
						0)) == INVALID_HANDLE_VALUE) {
						close();
						throw EXCEPT_TEXT(jio::exception, GetLastError(), jio::xplatform::GetLastErrorAsString());
					}
				}
			
				return true;
			}

		public:
			/*!
			*	This inner-class handles security for the pipes including firewall, registry and security descriptor creation.
			*	This is intended to work around any possible permissions humbuggery involved in named pipe access in windows.
			*	from: http://msdn.microsoft.com/en-us/library/aa364726%28v=VS.85%29.aspx
			*/
			class security {
			public:
				/*!
				*	This function is a wrapper for its overload which performs all logic to add a firewall rule for
				*	a given image file name. This wrapper provides the currently running executables image for the 
				*	firewall exception.
				*
				*	@param [in] name The firewall rule name you want created for the currently running application.
				*/
				static bool set_firewall_allow(const std::wstring &name) {
					// We need to pull our image name for the helper function to get it
					WCHAR filePathName[MAX_PATH];
					GetModuleFileName(NULL, filePathName, MAX_PATH);
					std::wstring modulename(filePathName);

					return security::set_firewall_allow(L"ACRE 2 Plugin", modulename);
				}

				/*!
				*	Performs all the actual logic for adding a provided image file to a firewall exception rule. 
				*	Utilizes the COM API for adding itself as an exception. This does not require administrator to run.
				*
				*	@param [in] name The firewall rule name you want created for the currently running application.
				*	@param [in] imageFileName The binary image to create the firewall exception for.
				*/
				static bool set_firewall_allow(const std::wstring & name, const std::wstring & imageFileName) {
					INetFwProfile*		fwProfile;
					INetFwMgr*			fwMgr;
					INetFwPolicy*		fwPolicy;
					HRESULT				hr;
//					VARIANT_BOOL		fwEnabled;

					hr			= S_OK;
					fwProfile	= NULL;
					fwPolicy	= NULL;
					fwMgr		= NULL;

					// INitialize just in case cause COM is gay
					if ((hr = CoInitialize(NULL))
						== S_OK) {
						if ((hr = CoCreateInstance(__uuidof(NetFwMgr), NULL, CLSCTX_INPROC_SERVER, __uuidof(INetFwMgr), (void**)&fwMgr))
							== S_OK) {
							if ((hr = fwMgr->get_LocalPolicy(&fwPolicy))
								== S_OK) {
								if ((hr = fwPolicy->get_CurrentProfile(&fwProfile))
									== S_OK) {
									//
									// Okay we finally have a damn profile
									//
									INetFwAuthorizedApplication*	fwApp = NULL;
									INetFwAuthorizedApplications*	fwApps = NULL;
									BSTR							fwBstrProcessImageFileName = NULL;
									BSTR							fwBstrName = NULL;

									// Now we need to:
									// 1. Check if we are already enabled. If we are, just continue
									// 2. If we are NOT enabled, we need to add and enable ourselves
									if (!firewall_is_app_enabled(imageFileName, fwProfile)) {

										// App doesnt exist in the policy, we need to add it
										if (!firewall_is_app_in_profile(imageFileName, fwProfile)) {
											firewall_app_add(name, imageFileName, fwProfile);
											// App exists in the policy, we just need to enable it
										}
										else {
											firewall_app_toggle(imageFileName, fwProfile, true);
										}
									}
									// 
									// Unwind and clean up
									//
									fwProfile->Release();
								}
								fwPolicy->Release();
							}
							fwMgr->Release();
						}
					}

					// After unravelling, if we errored, throw our exception
					if (hr != S_OK) {
						_com_error err(hr);	
					
						throw EXCEPT_TEXT(jio::exception, hr, xplatform::wstring_to_string(err.ErrorMessage()).c_str());
					}

					return true;
				}

				/*!
				*	Generates a low integrity integrity DACL.
				*/
				static PSECURITY_DESCRIPTOR get_low_sa(void) {
					PSECURITY_DESCRIPTOR pSD;

					pSD = NULL;
					if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(LOW_INTEGRITY_SDDL_SACL_W, SDDL_REVISION_1, &pSD, NULL)) {
						throw EXCEPT_TEXT(jio::exception, GetLastError(), jio::xplatform::GetLastErrorAsString());
					}

					return pSD;
				}

				/*!
				*	Generates a untrusted integrity DACL.
				*	NOTE: THIS IS THE MAGIC DACL FOR ALLOWING ANYTHING TO CONNECT TO OUR PIPE.
				*/
				static PSECURITY_DESCRIPTOR get_untrusted_sa(void) {
					PSECURITY_DESCRIPTOR pSD;

					pSD = NULL;
					if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(UNTRUSTED_INTEGRITY_SDDL_SACL_W, SDDL_REVISION_1, &pSD, NULL)) {
						throw EXCEPT_TEXT(jio::exception, GetLastError(), jio::xplatform::GetLastErrorAsString());
					}

					return pSD;
				}

				/*!
				*	Gets a SID for the current LOGON. Can be used for pipe security.
				*/
				BOOL GetLogonSID(HANDLE hToken, PSID *ppsid)
				{
					BOOL bSuccess = FALSE;
					DWORD dwIndex;
					DWORD dwLength = 0;
					PTOKEN_GROUPS ptg = NULL;

					// Verify the parameter passed in is not NULL.
					if (NULL == ppsid)
						goto Cleanup;

					// Get required buffer size and allocate the TOKEN_GROUPS buffer.

					if (!GetTokenInformation(
						hToken,         // handle to the access token
						TokenGroups,    // get information about the token's groups 
						(LPVOID)ptg,   // pointer to TOKEN_GROUPS buffer
						0,              // size of buffer
						&dwLength       // receives required buffer size
						)) {
						if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
							goto Cleanup;

						ptg = (PTOKEN_GROUPS)HeapAlloc(GetProcessHeap(),
							HEAP_ZERO_MEMORY, dwLength);

						if (ptg == NULL)
							goto Cleanup;
					}

					// Get the token group information from the access token.

					if (!GetTokenInformation(
						hToken,         // handle to the access token
						TokenGroups,    // get information about the token's groups 
						(LPVOID)ptg,   // pointer to TOKEN_GROUPS buffer
						dwLength,       // size of buffer
						&dwLength       // receives required buffer size
						))
					{
						goto Cleanup;
					}

					// Loop through the groups to find the logon SID.
					for (dwIndex = 0; dwIndex < ptg->GroupCount; dwIndex++)
						if ((ptg->Groups[dwIndex].Attributes & SE_GROUP_LOGON_ID)
							== SE_GROUP_LOGON_ID) {
							// Found the logon SID; make a copy of it.

							dwLength = GetLengthSid(ptg->Groups[dwIndex].Sid);
							*ppsid = (PSID)HeapAlloc(GetProcessHeap(),
								HEAP_ZERO_MEMORY, dwLength);
							if (*ppsid == NULL)
								goto Cleanup;
							if (!CopySid(dwLength, *ppsid, ptg->Groups[dwIndex].Sid)) {
								HeapFree(GetProcessHeap(), 0, (LPVOID)*ppsid);
								goto Cleanup;
							}
							break;
						}

					bSuccess = TRUE;

				Cleanup:
					// Free the buffer for the token groups.
					if (ptg != NULL)
						HeapFree(GetProcessHeap(), 0, (LPVOID)ptg);

					return bSuccess;
				}

			protected:
				/*!
				*	Internal function to add an application to the provided firewall policy object.
				*/
				static bool firewall_app_add(const std::wstring & name, const std::wstring & imageFileName, INetFwProfile* fwProfile) {
					INetFwAuthorizedApplication*	fwApp = NULL;
					INetFwAuthorizedApplications*	fwApps = NULL;
					BSTR							fwBstrProcessImageFileName = NULL, fwBstrName = NULL;
					HRESULT							hr = S_OK;
					bool							result = false;

					// Allocate a BSTR for the process image file name.
					fwBstrProcessImageFileName = SysAllocString(imageFileName.c_str());
					if (fwBstrProcessImageFileName == NULL) {
						hr = E_OUTOFMEMORY;
						throw EXCEPT_TEXT(jio::exception, E_OUTOFMEMORY, "SysAllocString: Out of Memory");
					}
					fwBstrName = SysAllocString(name.c_str());
					if (fwBstrName == NULL) {
						hr = E_OUTOFMEMORY;
						throw EXCEPT_TEXT(jio::exception, E_OUTOFMEMORY, "SysAllocString: Out of Memory");
					}

					if ((hr = fwProfile->get_AuthorizedApplications(&fwApps))
						== S_OK) {
						if ((hr = fwApps->Item(fwBstrProcessImageFileName, &fwApp))
							== S_OK) {

							result = true;
							fwApp->Release();
						}
						else {
							// This is our appropriate handle case, it wasnt in the collection
							if ((hr = CoCreateInstance( __uuidof(NetFwAuthorizedApplication), NULL, CLSCTX_INPROC_SERVER, __uuidof(INetFwAuthorizedApplication), 
								(void**)&fwApp)) 
								== S_OK) {

								fwApp->put_Name(fwBstrName);
								fwApp->put_ProcessImageFileName(fwBstrProcessImageFileName);
								fwApp->put_Enabled(VARIANT_TRUE);
								fwApp->put_Scope(NET_FW_SCOPE_ALL);

								if ((hr = fwApps->Add(fwApp)) == S_OK) {
									result = true;
								} 

								fwApp->Release();
							}

						}
						fwApps->Release();
					}
					SysFreeString(fwBstrProcessImageFileName);

					return result;
				}

				/*!
				*	Internal function to check whether a firewall rule, in a firewall profile, is already enabled for 
				*	the provided image file.
				*/
				static bool firewall_is_app_enabled(const std::wstring & imageFileName, INetFwProfile* fwProfile) {
					INetFwAuthorizedApplication*	fwApp = NULL;
					INetFwAuthorizedApplications*	fwApps = NULL;
					BSTR							fwBstrProcessImageFileName = NULL;
					HRESULT							hr = S_OK;
					bool							result = false;
					VARIANT_BOOL					fwEnabled = VARIANT_FALSE;

					// Allocate a BSTR for the process image file name.
					fwBstrProcessImageFileName = SysAllocString(imageFileName.c_str());
					if (fwBstrProcessImageFileName == NULL) {
						hr = E_OUTOFMEMORY;
						throw EXCEPT_TEXT(jio::exception, E_OUTOFMEMORY, "SysAllocString: Out of Memory");
					}

					if ((hr = fwProfile->get_AuthorizedApplications(&fwApps))
						== S_OK) {
						if ((hr = fwApps->Item(fwBstrProcessImageFileName, &fwApp))
							== S_OK) {
							if ((hr = fwApp->get_Enabled(&fwEnabled))
								== S_OK) {
								if (fwEnabled != VARIANT_FALSE) {
									result = true;
								}
							}
							fwApp->Release();
						}
						fwApps->Release();
					}
					SysFreeString(fwBstrProcessImageFileName);

					return result;
				}

				/*!
				*	Checks whether an application (image file) is already in a provided application firewall profile.
				*/
				static bool firewall_is_app_in_profile(const std::wstring & imageFileName, INetFwProfile* fwProfile) {
					INetFwAuthorizedApplication*	fwApp = NULL;
					INetFwAuthorizedApplications*	fwApps = NULL;
					BSTR							fwBstrProcessImageFileName = NULL;
					HRESULT							hr = S_OK;
					bool							result = false;

					// Allocate a BSTR for the process image file name.
					fwBstrProcessImageFileName = SysAllocString(imageFileName.c_str());
					if (fwBstrProcessImageFileName == NULL) {
						hr = E_OUTOFMEMORY;
						throw EXCEPT_TEXT(jio::exception, E_OUTOFMEMORY, "SysAllocString: Out of Memory");
					}

					if ((hr = fwProfile->get_AuthorizedApplications(&fwApps))
						== S_OK) {
						if ((hr = fwApps->Item(fwBstrProcessImageFileName, &fwApp))
							== S_OK) {
							result = true;
							fwApp->Release();
						}
						fwApps->Release();
					}
					SysFreeString(fwBstrProcessImageFileName);

					return result;
				}

				/*!
				*	Toggles an application firewall exception within the provided firewall profile.
				*/
				static bool firewall_app_toggle(const std::wstring & imageFileName, INetFwProfile* fwProfile, bool enabled) {
					INetFwAuthorizedApplication*	fwApp = NULL;
					INetFwAuthorizedApplications*	fwApps = NULL;
					BSTR							fwBstrProcessImageFileName = NULL;
					HRESULT							hr = S_OK;
					bool							result = false;

					// Allocate a BSTR for the process image file name.
					fwBstrProcessImageFileName = SysAllocString(imageFileName.c_str());
					if (fwBstrProcessImageFileName == NULL) {
						hr = E_OUTOFMEMORY;
						throw EXCEPT_TEXT(jio::exception, E_OUTOFMEMORY, "SysAllocString: Out of Memory");
					}

					if ((hr = fwProfile->get_AuthorizedApplications(&fwApps))
						== S_OK) {
						if ((hr = fwApps->Item(fwBstrProcessImageFileName, &fwApp))
							== S_OK) {

							(enabled) ? fwApp->put_Enabled(VARIANT_TRUE) : fwApp->put_Enabled(VARIANT_FALSE);

							fwApp->Release();
						}
						fwApps->Release();
					}
					SysFreeString(fwBstrProcessImageFileName);

					return result;
				}
			};
		};
	}
}