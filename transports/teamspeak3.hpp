#pragma once


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

		/*!
		* Teamspeak 3 transport implementation class for ACRE.
		*/
		class teamspeak3 : public base_transport {
		public:
			teamspeak3() { }
			~teamspeak3() { 
				// clear the internal queue on destruction
				flush();
			}
	
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
			/*!
			* TS3 implementation middle-man function for calling into API with sendPLuginCommand
			*/
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

			/*!
			*	Sends a message object via TS3. If they message data contains nulls, it is base64 encoded for 
			*	TS3 string handling. If it does not, its sent plain over the wire. Overhead of a NULL containing
			*	message is an extra allocation, base64 encode, and deallocation. Sending a non-null message
			*	will just temporarily null-terminate the buffer (and off-by-one on purpose) temporarily for performance.
			*/
			uint32_t		write(const message & msg) { 
				char *encoded_message = nullptr;
				unsigned char *search_ptr = nullptr;
				unsigned char save_char;
				size_t encoded_length = 0;
			
				// break if we hit a null
				for (search_ptr = msg.data; search_ptr != (msg.data + msg.length); search_ptr++) {
					if (*search_ptr == 0x00)
						break;
				}

				// if we hit the end of the stream and there were no nulls, send raw.
				if (search_ptr == (msg.data + msg.length)) {
					// null-terminate and save the data object, temporary for sending
					if (*search_ptr != 0x00) {
						save_char = *search_ptr;
						*search_ptr = 0x00;
					}
					impl_sendPluginCommand(msg.data, ts3_target);
					*search_ptr = save_char;
				} else { // There were nulls, encode.
					//base64 encode it
					encoded_message = _base64.encode(msg.data, msg.length, &encoded_length);
					impl_sendPluginCommand(msg.data, ts3_target);
					delete encoded_message;
				}

				return msg.length;
			}
		
			int ts3_target;
		private:
			std::queue<message *> _queue_in;
			jio::base64 _base64;
		};
	};
};