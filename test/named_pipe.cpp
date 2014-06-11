#include <stdio.h>
#include <string>

#include <jio/transports/named_pipe.hpp>

class NewPlayerMessage {
public:
	NewPlayerMessage(int a, const std::string & name) {}
};

void named_pipe(void) {
	//NewPlayerMessage newPlayerMsg(12345, "jaynus");
	try {
		jio::transports::named_pipe p("\\\\.\\pipe\\die", jio::transports::named_pipe_settings(), true);
		//p << newPlayerMsg;
	}
	catch (jio::exception e) {
		PRINT_EXCEPTION(e);
	}
}