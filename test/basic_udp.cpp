#include <stdio.h>
#include <string>

#include <jio/transports/basic_udp.hpp>

void basic_udp(void) {
	try {
		printf("* Beginning UDP listener...\n");
		auto server = jio::transports::basic_udp(12345);
		printf("* Press enter to stop listening\n");
		while (true) {
			try {
				jio::transports::message  * msg = server.read();
				if (msg != nullptr) {
					printf("%d:%s", msg->length, msg->data);
					break;
				}
			} catch (jio::exception e) {
				PRINT_EXCEPTION(e);
			}
			sleep(1);
		}

		printf("UDP listening ended\n");
	}
	catch (jio::exception e) {
		PRINT_EXCEPTION(e);
	}
}