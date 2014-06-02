#include <stdio.h>

#include <string>

#include <jio/transports/named_pipe.hpp>

class NewPlayerMessage {
public:
	NewPlayerMessage(int a, const std::string & name) {}
};

int main(int argc, char **argv) {
	//NewPlayerMessage newPlayerMsg(12345, "jaynus");
	try {
		jio::transports::named_pipe p("\\\\.\\pipe\\die", true);
		//p << newPlayerMsg;
	} 
	catch (jio::exception e) { 
		PRINT_EXCEPTION(e);
		getchar();
		exit(-1);
	}

	printf("Wee\n");

	getchar();
	
	return 0;
}