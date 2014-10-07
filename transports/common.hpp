#pragma once

#include <string>
#include <memory>

#include <jio/xplatform.hpp>
#include <jio/exception.hpp>
#include <jio/transports/common.hpp>

namespace jio {
	namespace transports {
		class i_transport;
		/*
		* Generic data class
		*/
		template<typename T> class message_t {
		public:
			message_t(T *_data, const uint32_t _length) {
				data = new T[_length];  memcpy(data, _data, _length * sizeof(T));
				length = _length; src = NULL; }
			message_t(T *_data, const uint32_t _length, i_transport *transport) {
				data = new T[_length];  memcpy(data, _data, _length * sizeof(T));
				length = _length; src = transport; }
			
			message_t(const message_t & o) { data = o.data; length = o.length; src = o.src; }
			void operator = (const message_t & o) { data = o.data; length = o.length; src = o.src; }
			~message_t() {
				if(data)
					delete data;
			}
			//message_t(const message_t<T> & o ) {  data = o.data; length = o.length; } 
			//void operator = (const message_t<T> & o) { data = o.data; length = o.length; }
			
			
			T *data;
			i_transport * src;
			uint32_t length;
		};
		typedef message_t<unsigned char> message;
		typedef std::shared_ptr< message_t<unsigned char> > message_p;
		/*
		*	Transport Interface
		*/
		class i_transport {
		public:
			virtual bool 			open(void) = 0;
			virtual void 			close(void) = 0;
			
			virtual void 			flush(void) = 0;
			
			virtual message  * read(void) = 0;
			virtual uint32_t write(const message & data) = 0;

		};
		
		/*
		* 	Base Transport Class
		*/
		class base_transport : 
			public i_transport {
		public:
				~base_transport() {}
		};
	};
};


#include <jio/transports/exception.hpp>