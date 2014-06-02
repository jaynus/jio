#pragma once

#if defined (__WIN32__) || defined(WIN32) /* WINDOWS */

#define _WINDOWS

#if !defined(_TYPES_)
#define _TYPES_
typedef unsigned char		uint8_t;
typedef unsigned short		uint16_t;
typedef unsigned int		uint32_t;
typedef unsigned long long	uint64_t;

typedef			char		int8_t;
typedef			short		int16_t;
typedef			int			int32_t;
typedef			long long	int64_t;
#endif

#else /* LINUX */

#define _LINUX

#endif

#if defined(_WINDOWS)

#include <WinSock2.h>
#include <Windows.h>
#include <Sddl.h>

namespace jio {
	class xplatform {
	public:
		/*!
		*	cross-platform static function for printing the next of last error.
		*/
		static std::string GetLastErrorAsString (void) {
			//Get the error message, if any.
			DWORD errorMessageID = ::GetLastError();
			LPSTR messageBuffer = nullptr;

			if (errorMessageID == 0) {
				return "No error message has been recorded";
			}

			size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

			std::string message(messageBuffer, size);

			//Free the buffer.
			::LocalFree(messageBuffer);

			return message;
		}
	};
}

#endif