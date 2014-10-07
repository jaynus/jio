#include <stdio.h>

#include <jio/test/functions.h>

#if defined(_WINDOWS)
INIT_WINSOCK2();
#endif

int main(int argc, char **argv) {

	//BEGIN_TEST(dispatcher);

	BEGIN_TEST(named_pipe);
	//BEGIN_TEST(basic_udp);

	getchar();
	
	return 0;
}