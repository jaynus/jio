#pragma once

#if defined (__WIN32__)
#define _WINDOWS
#else
#define _LINUX
#endif

#include <cstdint>

namespace jio {
	class ITransport {
	public:
		virtual void read(void) = 0;
		virtual void write(void) = 0;
		virtual void open(void) = 0;
		virtual void create(void) = 0;
		virtual void close(void) = 0;
		virtual void flush(void) = 0;
	};

	class NamedPipeTransport : 
		public ITransport {
	public:
		NamedPipeTransport() {}
		~NamedPipeTransport() {}
		
		void read(void) {}
		void write(void) {}
		void open(void) {}
		void create(void) {}
		void close(void) {}
		void flush(void) {}
		
	};

};