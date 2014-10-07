#pragma once

#include <stdint.h>
#include <string>

#if defined (__WIN32__) || defined(WIN32) /* WINDOWS */

#define _WINDOWS

#define sleep Sleep

#else

#if !defined(_LINUX)
#define _LINUX
#endif

#endif

#if defined(_WINDOWS)

#include <WinSock2.h>
#include <ws2tcpip.h>
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
#pragma comment( lib, "Ws2_32.lib" )

#endif


#if defined(_LINUX)
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <cstring>
#include <unistd.h>
#endif

#include <stdio.h>
#include <memory>
#include <jio/exception.hpp>

namespace jio {
	namespace xplatform {

		static uint32_t resolve_host(const std::string & host) {
			struct addrinfo hints = { 0 };
			struct addrinfo *result = NULL, *ptr = NULL;
			uint32_t errorCode = 0;

			errorCode = inet_addr(host.c_str());
			if (errorCode != INADDR_NONE)
				return errorCode;

			if ((errorCode = ::getaddrinfo(host.c_str(), 0, &hints, &result)) != 0) {
				throw EXCEPT_TEXT(jio::exception, errorCode, "getaddrinfo() Failed")
			}
			else {

				for (errorCode = INADDR_NONE, ptr = result; ptr != NULL; ptr = ptr->ai_next) {
					if (ptr->ai_family != AF_INET) { // Only handle IPV4 addresses for now
						continue;
					}

					struct sockaddr_in *pptr = (struct sockaddr_in *)ptr;
					std::memcpy(&errorCode, &pptr->sin_addr, 4);

					break; // if we get to the end of a loop cycle, that means we've got a valid address
				}
				::freeaddrinfo(result);
			}

			if (errorCode == INADDR_ANY)
				throw EXCEPT_TEXT(jio::exception, errorCode, "Failed to lookup address")

				return errorCode;
		}

		class socket {
		public:

			static void close(uint32_t socket) {
#ifdef _WINDOWS
				::closesocket(socket);
#else
				::close(socket);
#endif
			}
		};

#if defined(_WINDOWS)
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

		static std::string wstring_to_string(const std::wstring & in) {
			std::string ret;
			ret.assign(in.begin(), in.end());
			return ret;
		}

		//
		// Helper functions
		//
		class winsock2 {
		public:
			static bool initialized;

			winsock2(void) {
				if (!initialized) {
					WSADATA wsaData;
					int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);

					if (iResult != 0) {
						throw EXCEPT_TEXT(jio::exception, iResult, "WSAStartup() Failed")
					}
				}

				initialized = true;
			}
		};
		extern std::unique_ptr<jio::xplatform::winsock2> ws2;
		#define INIT_WINSOCK2() bool jio::xplatform::winsock2::initialized = false; \
								std::unique_ptr<jio::xplatform::winsock2> ws2 = std::make_unique<jio::xplatform::winsock2>();
#endif
	};
};
