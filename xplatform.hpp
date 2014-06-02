#pragma once

#include <stdint.h>

#if defined (__WIN32__) || defined(WIN32) /* WINDOWS */

#define _WINDOWS

#else

#define _LINUX

#endif

#if defined(_WINDOWS)

#include <WinSock2.h>
#include <Windows.h>
#include <Sddl.h>
#include <netfw.h>
#include <objbase.h>
#include <oleauto.h>

// library requirements
#pragma comment( lib, "ole32.lib" )
#pragma comment( lib, "oleaut32.lib" )
#pragma comment( lib, "advapi32.lib" )
#pragma comment( lib, "kernel32.lib" )

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