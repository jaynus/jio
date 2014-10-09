#pragma once

#include <memory>
#include <string>

namespace jio {
	namespace transports {
		class i_transport;
	}
	namespace messaging {
		using namespace jio::transports;
		/*!
		*	Generic message data class
		*/
		template<typename T> class message_t {
		public:
			message_t(T *_data, const uint32_t _length) {
				data = new T[_length];  memcpy(data, _data, _length * sizeof(T));
				length = _length; src = NULL;
			}
			message_t(T *_data, const uint32_t _length, i_transport *transport) {
				data = new T[_length];  memcpy(data, _data, _length * sizeof(T));
				length = _length; src = transport;
			}

			message_t(const message_t & o) { data = o.data; length = o.length; src = o.src; }
			void operator = (const message_t & o) { data = o.data; length = o.length; src = o.src; }
			~message_t() {
				if (data)
					delete data;
			}
			//message_t(const message_t<T> & o ) {  data = o.data; length = o.length; } 
			//void operator = (const message_t<T> & o) { data = o.data; length = o.length; }

			std::string to_string() {
				std::string ret = "";

				if (length > 0) {
					if (this->data[length - 1] != 0x00) {
						T buffer[this->length];
						buffer[length - 1] = 0x00;
						ret = buffer;
					} else {
						ret = (char *)this->data;
					}
				}

				return ret;
			}

			T *data;
			i_transport * src;
			uint32_t length;
		};
		typedef message_t<unsigned char> message;
		typedef std::shared_ptr< message_t<unsigned char> > message_p;

		/*!
		*	Interface for message allocation factory. Use this if you want to use different buffers for messages
		*/
		class i_message_factory {
		public:
			virtual message		*createMessage(unsigned char *, size_t, i_transport *) = 0;
		};
	}
}