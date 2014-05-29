#include <stdio.h>

#include <string>

#include <jio/transports/named_pipe.hpp>

class NewPlayerMessage {
public:
	NewPlayerMessage(int a, const std::string & name) {}
};

int main(int argc, char **argv) {
	NewPlayerMessage newPlayerMsg(12345, "jaynus");
	
	jio::transports::named_pipe p("asdf");
	p << newPlayerMsg;
	p.close();
	
	
	return 0;
}