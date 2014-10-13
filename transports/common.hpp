#pragma once

#include <string>
#include <memory>

#include <jio/xplatform.hpp>
#include <jio/exception.hpp>
#include <jio/messaging/message.hpp>

namespace jio {
	namespace transports {
		using namespace jio::messaging;

		class i_transport;
		/*!
		*	Interface for a transport
		*/
		class i_transport {
		public:
			virtual bool 			open(void) = 0;
			virtual void 			close(void) = 0;
			
			virtual void 			flush(void) = 0;
			
			virtual jio::messaging::message *  read(i_message_factory *factory) = 0;
			virtual jio::messaging::message *  read(message & msg) = 0;
			virtual jio::messaging::message  * read(void) = 0;
			virtual uint32_t write(const jio::messaging::message & data) = 0;

		};
		
		/*!
		*	Base class implementation of blank transport interface
		*/
		class base_transport : 
			public i_transport {
		public:
				~base_transport() {}
		};
	};
};


#include <jio/transports/exception.hpp>