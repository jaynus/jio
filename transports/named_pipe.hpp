#pragma once

#include <string>

#include <jio/xplatform.hpp>
#include <jio/transports/common.hpp>

namespace jio {
	namespace transports {
		class named_pipe : 
			public base_transport {
		public:
			named_pipe(const std::string & pipe_name) {
			}
			~named_pipe() {}

			void open(void) {}
			void close(void) {}
			
			packet_buf & read(void) {
				throw 1;
			}
			uint32_t write(const packet_buf & data, uint32_t length ) {
				return 0;
			}
			
			void flush(void) {}
			
		};
	};
};