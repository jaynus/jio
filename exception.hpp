#pragma once

#include <exception>
#include <iostream>
#include <jio/xplatform.hpp>

#if !defined(EXCEPT)
#define EXCEPT(exception_obj, code)						exception_obj(code, std::string(__FILE__), __LINE__); 
#define EXCEPT_TEXT(exception_obj, code, desc)    		exception_obj(code, std::string(__FILE__), __LINE__, desc); 

#define THROW_NOT_IMPL()								throw EXCEPT_TEXT(jio::exception, -1, std::string("Not Implemented"));

#define PRINT_EXCEPTION(e) { \
	std::cout << "Exception Caught"; \
	std::cout << "\nCode:" << e.code; \
	std::cout << "\nSource: " << e.filename.c_str() << ":" << e.linenumber; \
	std::cout << "\nDesc: " << e.description.c_str() << "\n\n"; \
}

#endif

namespace jio {
	/*! 
	* 
	*/
	class exception : 
	public std::exception {
	public:
		exception() : description(""), filename(""), linenumber(-1), code(0) {}
		exception(const uint32_t errorcode) : description(""), filename(""), linenumber(-1), code(errorcode) {}
		exception(const std::string & file, uint32_t line) : description(""), filename(file), linenumber(line), code(-1) {}
		exception(const uint32_t errorcode, const std::string & file, uint32_t line) : description(""), filename(file), linenumber(line), code(errorcode) {}
		exception(const uint32_t errorcode, const std::string & file, uint32_t line, const std::string & desc) : description(desc), filename(file), linenumber(line), code(errorcode) {}
		exception(const uint32_t errorcode, const std::string & desc) : description(desc), filename(""), linenumber(-1), code(errorcode) { }

	public:	// Set as public since its just an exception that can be built
		std::string description;
		std::string filename;
		uint32_t 	linenumber;
		uint64_t 	code;
	};

	class not_impl : 
		public jio::exception {
	public:
		using jio::exception::exception;
		not_impl() { jio::exception::exception(-1, "Functionality not implemented."); }
	};

	class invalid_argument :
		public jio::exception {
	public:
		using jio::exception::exception;
		invalid_argument() { jio::exception::exception(-1, "Functionality not implemented."); }
	};
};