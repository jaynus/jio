CC=g++
INCLUDE=-I../
DFLAGS=-D_LINUX
CFLAGS=-c -Wall $(DFLAGS) $(INCLUDE) -std=c++11



all: linux/test

linux/test: linux/named_pipe.o linux/basic_udp.o linux/test.o
	g++ linux/named_pipe.o linux/basic_udp.o linux/test.o -o linux/test

linux/named_pipe.o: test/named_pipe.cpp
	g++ $(CFLAGS) test/named_pipe.cpp -o linux/named_pipe.o

linux/basic_udp.o: test/basic_udp.cpp
	g++ $(CFLAGS) test/basic_udp.cpp -o linux/basic_udp.o

linux/test.o: test/test.cpp
	g++ $(CFLAGS) test/test.cpp -o linux/test.o

clean:
	rm -rf linux/*