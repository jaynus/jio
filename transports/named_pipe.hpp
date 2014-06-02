#pragma once

#include <string>

#include <jio/xplatform.hpp>
#include <jio/transports/common.hpp>

namespace jio {
	namespace transports {	 
		/*! 
		 * Base class for pipe-based transport implementations (such as named pipes, FIFO, loopback, etc)
		 */
		class base_pipe : 
			public base_transport {	
		public:
			/*!
			 *	Returns true or false of whether the handle has read-access.
			 */
			bool readable() const { return true ; }
			/*!
			 *	Returns true or false of whether the handle has write-access.
			 */
			bool writable() const { return true; }
			/*!
			 *	Permissions of the currently active pipe.
			 */
			uint32_t permissions() const { return _permissions; }
			/*!
			 *	The fully qualified path of the pipe.
			 */
			const std::string name() const { return _name; }
			const bool is_server() const { return _server; }
		protected:
			std::string _name;
			uint32_t _permissions;
			bool _server;
			
		};
	};
};

#ifdef _WINDOWS
#include "named_pipe_win32.hpp"
#else 
// TODO: named pipe for linux goes here
// #include "named_pipe_linux.hpp"
#endif