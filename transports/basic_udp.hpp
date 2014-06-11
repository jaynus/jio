#pragma once

#if defined(_LINUX)
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#include <jio/xplatform.hpp>
#include <jio/exception.hpp>
#include <jio/transports/common.hpp>


namespace jio {
	namespace transports {
		/*
		* UDP Interface
		*	Because UDP doesn't have a true server/client infastructure, each stream actually just has a destination host and a receiving port.
		*/
		class basic_udp : 
			public jio::transports::base_transport {
		public:

			basic_udp(uint16_t lport)  {
				// Resolvable hostname that then calls our real constructor
				basic_udp(lport, INADDR_NONE, 0);
			}
			basic_udp(uint16_t lport, std::string desthost, uint16_t dport)  {
				// Resolvable hostname that then calls our real constructor
#ifdef _WINDOWS
				basic_udp(lport, jio::transports::winsock2::resolve_host(desthost), dport);
#endif
			}
			basic_udp(uint16_t lport, uint32_t desthost, uint16_t dport) : listen_port(lport), dest_address(desthost), dest_port(dport) {
				open();
			}

			bool 			open(void) {
				THROW_NOT_IMPL();
			}
			void 			close(void) {
				THROW_NOT_IMPL();
			}
			void 			flush(void) {
				THROW_NOT_IMPL();
			}

			message & 		read(void) { 
				THROW_NOT_IMPL(); 
			}
			uint32_t		write(const message & data) {
				THROW_NOT_IMPL();
			}

		public:
			uint16_t listen_port;
			uint32_t dest_address;
			uint16_t dest_port;

		};
	};
};


#include <jio/transports/exception.hpp>