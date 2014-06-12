#pragma once

#include <errno.h>
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
				basic_udp(lport, jio::xplatform::resolve_host(desthost), dport);
			}
			basic_udp(uint16_t lport, uint32_t desthost, uint16_t dport) : listen_port(lport), dest_address(desthost), dest_port(dport), status(false) {
				status = open();
			}
			~basic_udp() {
				close();
			}

			bool 			open(void) {
				if ((socket_fd = ::socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
					EXCEPT_TEXT(jio::exception, errno, "socket() failed");
				}
				
				memset(&listen_addr, 0x00, sizeof(listen_addr));
				listen_addr.sin_family = AF_INET;
				listen_addr.sin_addr.s_addr = INADDR_ANY;
				listen_addr.sin_port = listen_port;

				if (::bind(socket_fd, (struct sockaddr *)&listen_addr, sizeof(listen_addr)) == -1) {
					xplatform::socket::close(socket_fd);
					EXCEPT_TEXT(jio::exception, -1, "bind() failed");
				}

				return true;
			}
			void 			close(void) {
				if (status) {
					xplatform::socket::close(socket_fd);
				}
			}
			void 			flush(void) {
				THROW_NOT_IMPL();
			}

			message_p  		read(void) {
				uint32_t res;
				unsigned char *data;
				
				// TODO: what do we do on no message case? For now we'll just block...
				while ((res = recv(socket_fd, NULL, 32, MSG_PEEK)) == 0) {}
				if (res < 1) {
					return std::shared_ptr<message>(nullptr);
				}
				
				return std::shared_ptr<message>(nullptr);
			}

			uint32_t		write(const message_p data) {
				uint32_t res, sCount;

				if (dest_address == INADDR_NONE) {
					EXCEPT_TEXT(jio::exception, -1, "No destinations for this transport.");
				}
				if (!status || socket_fd == -1) {
					EXCEPT_TEXT(jio::exception, -1, "Transport not active.");
				}
				// loop sending the data until we send all of it
				for (sCount = 0; sCount < data->length;) {
					res = ::send(socket_fd, (const char *)(data->data + sCount), (data->length - sCount), 0);
					
					if (res == -1) {
						EXCEPT_TEXT(jio::exception, -1, "send() failed");
					}
					sCount += res;
				}

				return data->length;
			}
		protected:
			sockaddr_in listen_addr;
			uint32_t	socket_fd;
			bool		status;

			uint16_t listen_port;
			uint32_t dest_address;
			uint16_t dest_port;
		};
	};
};


#include <jio/transports/exception.hpp>