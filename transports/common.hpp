#pragma once

#include <jio/xplatform.hpp>
#include <jio/transports/common.hpp>

namespace jio {
	namespace transports {
	
		/*
		* Generic data class
		*/
		template<typename T> class message {
		public:
			message(const T _data, const uint32_t _length) { data = _data; length = _length; }
			message(const message& o ) {  data = o.data; length = o.length; } 
			void operator = (const message & o) { data = o.data; length = o.length; }
			
			T data;
			uint32_t length;
		};
		typedef message<unsigned char *> packet_buf;
		
		/*
		*	Transport Interface
		*/
		class i_transport {
		public:
			virtual void 			open(void) = 0;
			virtual void 			close(void) = 0;
			
			virtual void 			flush(void) = 0;
			
			virtual packet_buf & 	read(void) = 0;
			virtual uint32_t		write(const packet_buf & data, uint32_t length ) = 0;
			
			
		};
		
		
		/*
		* 	Base Transport Class
		*/
		class base_transport : 
			public i_transport {
		public:
				base_transport() {}
				~base_transport() {}
		};
	};
};
