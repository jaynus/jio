#pragma once

#include <string>

#include <jio/xplatform.hpp>
#include <jio/transports/common.hpp>

#include "RakPeerInterface.h"
#include "MessageIdentifiers.h"

#ifdef WIN32
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "RakNetDll.lib")
#endif

namespace jio {
	namespace transports {
		using namespace jio::messaging;

		class raknet :
			public i_transport {
		public:
			struct raknet_settings {
				raknet_settings() : server(false), target_host(""), target_port(0), port(0), max_connections(1000) {}
				raknet_settings(const std::string &target_, const uint16_t port_) : server(false), max_connections(1000) {
					target_host = target_;
					target_port = port_;
				}
				raknet_settings(const raknet_settings & in) {
					server = in.server; 
					target_host = in.target_host;
					target_port = in.target_port;
					port = in.port;
					max_connections = in.max_connections;
				}

				bool			server;
				std::string		target_host;
				uint16_t		target_port;
				uint16_t		port;
				uint32_t		max_connections;
			};
			raknet(raknet_settings settings) : _settings(settings) { 
				// Set up our raknet peer based on the settings
				if (!_settings.server) {
					_peer->Startup(1, &_socketDesc, 1);
				} else {
					_peer->Startup(_settings.max_connections, &_socketDesc, 1);
					_peer->SetMaximumIncomingConnections(_settings.max_connections);
				}
			}

			~raknet() { 
				RakNet::RakPeerInterface::DestroyInstance(_peer);
			}

			bool open(void) { 
				if (_settings.server) {
					return true;
				}

				if (_settings.target_host == "" || _settings.target_port == 0) {
					throw EXCEPT_TEXT(jio::exception, E_INVALIDARG, "Invalid target information specified.");
				}

				_peer->Connect(_settings.target_host.c_str(), _settings.target_port, 0, 0);
		
				return true;
			}
			void close(void) { 
				_peer->Shutdown(0);
			}

			void flush(void) { THROW_NOT_IMPL(); }

			jio::messaging::message *  read(i_message_factory *factory) { THROW_NOT_IMPL(); return nullptr; }
			jio::messaging::message *  read(message & msg) { THROW_NOT_IMPL(); return nullptr; }
			jio::messaging::message * read(void) {
				jio::messaging::message *msg;

				RakNet::Packet *packet = _peer->Receive();
				msg = new message(packet->data + 4, packet->length - 4);
				_peer->DeallocatePacket(packet);

				return msg;
			}
			uint32_t write(const jio::messaging::message & data) { THROW_NOT_IMPL(); return 0; }

		protected:
			RakNet::RakPeerInterface *_peer;
			RakNet::SocketDescriptor _socketDesc;
			raknet_settings _settings;
		};
	}
}