#include <stdio.h>
#include <string>
#include <memory>
#include <jio/messaging/dispatcher.hpp>

jio::messaging::dispatch * callback(jio::messaging::dispatcher *d, const jio::messaging::dispatch in) {
	printf("\t! Hello World Callback!\n");
	return nullptr;
}
void dispatcher(void) {
	try {
	/*	printf("Dispatcher test started\n");
		jio::messaging::dispatcher d;
		printf("* Adding handler\n");
		d.add(1, "fuck", jio::messaging::dispatch_func_t(&callback));
		printf("* Calling by ID\n");
		auto test_data = std::make_shared<char>(new char[255]);
		d.call(std::make_shared<dispatch>(jio::messaging::dispatch(1, test_data)));
		printf("* Calling by Name\n");
		//d.call("fuck", jio::messaging::dispatch_data("jkl;", 5));
		printf("Dispatcher test ended\n");
	*/}
	catch (jio::exception e) {
		PRINT_EXCEPTION(e);
	}
}