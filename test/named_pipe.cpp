#include <stdio.h>
#include <string>
#include <thread>

#include <jio/transports/named_pipe.hpp>

const char *TEST_NAME = "\\\\.\\pipe\\die";
jio::transports::named_pipe *server_pipe;


class NewPlayerMessage {
public:
	NewPlayerMessage(int a, const std::string & name) {}
};

void client_test() {
	bool sent = false;

	printf("* Client Entry\n");
	jio::transports::named_pipe client_pipe(TEST_NAME, jio::transports::named_pipe_settings());
	while (true) {
		if (!sent) {
			unsigned char data[] = "AAAA\x00";
			jio::transports::message msg(data, 5, NULL);
			client_pipe.write(msg);
			client_pipe << msg; 
			client_pipe << std::string("Fuck you");
			sent = true;
		}
		sleep(1);
	}
}

void server_test() {
	printf("* Server Entry\n");
	while (true) {
		jio::transports::message * ptr = server_pipe->read();
		if (ptr != nullptr) {
			printf("Rcpt:[%s]\n", ptr->data);
			delete ptr;
		}
		sleep(1);
	}
}

void named_pipe(void) {
	//NewPlayerMessage newPlayerMsg(12345, "jaynus");
	try {
		server_pipe = new jio::transports::named_pipe(TEST_NAME, jio::transports::named_pipe_settings(), true);
		jio::transports::named_pipe::security::set_firewall_allow(L"Test Firewall Rule");
	}
	catch (jio::exception e) {
		PRINT_EXCEPTION(e);
	}

	// Spawn the client and server threads for testing
	std::thread server(server_test);
	printf("Server started, waiting on client...\n");
	sleep(1000);
	printf("Client starting...\n");
	std::thread client(client_test);

	client.join();
	server.join();

}