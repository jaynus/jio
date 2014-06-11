#pragma once

#include <string>
#include <memory>

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
				~base_transport() {}
		};


#if defined(_WINDOWS)
		//
		// Helper functions
		//
		class winsock2 {
		public:
			static bool initialized;
			
			winsock2(void) {
				if (!initialized) {
					WSADATA wsaData;
					int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);

					if (iResult != 0) {
						EXCEPT_TEXT(jio::exception, iResult, "WSAStartup() Failed")
					}
				}

				initialized = true;
			}

			static inline uint32_t resolve_host(const std::string & host) {
				struct addrinfo hints = { 0 };
				struct addrinfo *result = NULL, *ptr = NULL;
				uint32_t errorCode = 0;

				errorCode = inet_addr(host.c_str());
				if (errorCode != INADDR_NONE)
					return errorCode;

				if ((errorCode = ::getaddrinfo(host.c_str(), 0, &hints, &result)) != 0) {
					EXCEPT_TEXT(jio::exception, errorCode, "getaddrinfo() Failed")
				}
				else {

					for (errorCode = INADDR_NONE, ptr = result; ptr != NULL; ptr = ptr->ai_next) {
						if (ptr->ai_family != AF_INET) { // Only handle IPV4 addresses for now
							continue;
						}

						struct sockaddr_in *pptr = (struct sockaddr_in *)ptr;
						memcpy(&errorCode, &pptr->sin_addr, 4);

						break; // if we get to the end of a loop cycle, that means we've got a valid address
					}
					::freeaddrinfo(result);
				}

				if (errorCode = INADDR_ANY)
					EXCEPT_TEXT(jio::exception, errorCode, "Failed to lookup address")

					return errorCode;
			}
		};
		extern std::unique_ptr<jio::transports::winsock2> ws2;
		#define INIT_WINSOCK2() bool jio::transports::winsock2::initialized = false; \
								std::unique_ptr<jio::transports::winsock2> ws2 = std::make_unique<jio::transports::winsock2>();

#endif
	};
};


#include <jio/transports/exception.hpp>