#pragma once


/*!
* Named pipe implementation class.
*/
#include <jio/xplatform.hpp>
#include <jio/exception.hpp>
#include <jio/transports/common.hpp>

#include <thread>
#include <string>

namespace jio {
	namespace transports {
		
		typedef struct named_pipe_settings {
			int asdf;
		} named_pipe_settings;

		class named_pipe : public base_transport {
		public:
			named_pipe(const std::string & pipename, named_pipe_settings settings, bool is_server = false) {
			}

			~named_pipe(){}
			bool 			open(void) { THROW_NOT_IMPL(); }
			void 			close(void) { THROW_NOT_IMPL(); }

			void 			flush(void) { THROW_NOT_IMPL(); }

			message_p 		read(void) { THROW_NOT_IMPL(); }
			uint32_t		write(const message_p data) { THROW_NOT_IMPL(); }
		};
	};
};