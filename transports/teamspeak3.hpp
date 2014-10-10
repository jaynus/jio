#pragma once

/*!
* Named pipe implementation class.
*/
#include <jio/xplatform.hpp>
#include <jio/exception.hpp>
#include <jio/transports/common.hpp>
#include <jio/base64.hpp>

#include <thread>
#include <string>
#include <queue>

namespace jio {
	namespace transports {
		using namespace jio::messaging;

		class teamspeak3 : public base_transport {
		public:
			teamspeak3() { }
			~teamspeak3(){}
	
			/*!
			 * TS3 function hook for receiving messages. This will queue messages for later reading as they are received.
			 */
			bool hook_onPluginCommandEvent(char *command) {
				message *read_message = nullptr;
				int *pack_check = (int *)command;
			
				// if the first 4 bytes are FF, then we need to unpack it
				if (*pack_check == 0xFFFFFFFF) {
					char * decode_ptr = command + 4;
					unsigned char * bin_command = nullptr;
					char encoded_length = strlen(command);
					size_t decode_length = 0;

					bin_command = _base64.decode(command, encoded_length, &decode_length);
					read_message = new message(bin_command, decode_length);
				} else {
					// data contained no nulls, so we can direct handle it
					read_message = new message((unsigned char *)command, strlen(command));
				}

				if (read_message == nullptr)
					return false;

				this->_queue_in.push(read_message);
			}
			uint32_t impl_sendPluginCommand(const unsigned char *buffer, int target){
				//ts3Functions.sendPluginCommand(serverConnectionHandlerID, pluginID, buffer, target, NULL, NULL);
				
				return 1;
			}
			
			bool 			open(void) { THROW_NOT_IMPL(); }
			void 			close(void) { THROW_NOT_IMPL(); }

			/*!
			* Word of warning. this flushes the INBOUND MESSAGE QUEUE!
			*/
			void 			flush(void) {
				while (!_queue_in.empty()) {
					message *t = _queue_in.front();
					delete t;
					_queue_in.pop();
				}
			}

			message * 		read(void) { 
				if (!_queue_in.empty()) {
					message *ret = _queue_in.front();
					_queue_in.pop();
					return ret;
				}

				return nullptr;
			}
			message *  read(i_message_factory *factory) { THROW_NOT_IMPL(); return nullptr; }
			message *  read(message & msg) { THROW_NOT_IMPL(); return nullptr; }

			uint32_t		write(const message & msg) { 
				char *encoded_message = nullptr;
				unsigned char *search_ptr = nullptr;
				size_t encoded_length = 0;

				// break if we hit a null
				for (search_ptr = msg.data; search_ptr != (msg.data + msg.length); search_ptr++) {
					if (search_ptr == 0x00)
						break;
				}

				// if we hit the end of the stream and there were no nulls, send raw.
				if (search_ptr == (msg.data + msg.length)) {
							
					// No nulls, null terminate it and send it
					encoded_message = new char[msg.length + 1];
					memcpy(encoded_message, msg.data, msg.length);
					encoded_message[msg.length] = 0x00;

				} else { // There were nulls, encode.
					
					//base64 encode it
					encoded_message = _base64.encode(msg.data, msg.length, &encoded_length);
				}
				uint32_t ret = impl_sendPluginCommand(msg.data, ts3_target);
				
				delete encoded_message;

				return msg.length;
			}
		
			int ts3_target;
		private:
			std::queue<message *> _queue_in;
			jio::base64 _base64;
		};
	};
};