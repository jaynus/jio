#pragma once

#include <jio/xplatform.hpp>
#include <jio/exception.hpp>
#include <jio/transports/common.hpp>

namespace jio {
	namespace transports {
		/*
		* Generic data class
		*/
		template<typename T> class message_t {
		public:
			message_t(const T _data, const uint32_t _length) { data = _data; length = _length; }
			message_t(const message_t& o ) {  data = o.data; length = o.length; } 
			void operator = (const message_t & o) { data = o.data; length = o.length; }
			
			//message_t(const message_t<T> & o ) {  data = o.data; length = o.length; } 
			//void operator = (const message_t<T> & o) { data = o.data; length = o.length; }
			
			
			T data;
			uint32_t length;
		};
		typedef message_t<unsigned char *> message;
		
		/*
		*	Transport Interface
		*/
		class i_transport {
		public:
			virtual bool 			open(void) = 0;
			virtual void 			close(void) = 0;
			
			virtual void 			flush(void) = 0;
			
			virtual message & 		read(void) = 0;
			virtual uint32_t		write(const message & data ) = 0;
			
			
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


#include <jio/transports/exception.hpp>