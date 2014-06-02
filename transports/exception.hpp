#pragma once

#include <jio/transports/common.hpp>
#include <jio/exception.hpp>

namespace jio {
	namespace transports {
		/*
		*	Exceptions
		*/
		class transport_exception :
			public jio::exception {
		public:
			using jio::exception::exception; // inherit constructors

		protected:
			jio::transports::i_transport & _transport;
		};
	};
};