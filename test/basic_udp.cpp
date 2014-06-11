#include <stdio.h>
#include <string>

#include <jio/transports/basic_udp.hpp>

void basic_udp(void) {
	try {
		jio::transports::basic_udp(12345);

	}
	catch (jio::exception e) {
		PRINT_EXCEPTION(e);
	}
}