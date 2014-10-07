#include <stdio.h>
#include <string>
#include <memory>

#include <jio/messaging/dispatcher.hpp>

jio::messaging::dispatch * callback(jio::messaging::dispatcher *d, const jio::messaging::dispatch * in) {
	printf("\t! Hello World Callback!\n");
	delete in;
	return nullptr;
}
void dispatcher(void) {
	try {
		printf("Dispatcher test started\n");
		jio::messaging::dispatcher d;
		printf("* Adding handler\n");
		d.add(1, "fuck", jio::messaging::dispatch_func_t(&callback));
		printf("* Calling by ID\n");
		auto test_data = new char[255];
		d.call(new jio::messaging::dispatch(1, test_data));
		delete test_data;

		printf("* Calling by Name\n");
		d.call(std::string("fuck"), new jio::messaging::dispatch(1, test_data));
		printf("Dispatcher test ended\n");
		
	}
	catch (jio::exception e) {
		PRINT_EXCEPTION(e);
	}
}