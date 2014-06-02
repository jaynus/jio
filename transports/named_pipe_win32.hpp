#pragma once

#define LOW_INTEGRITY_SDDL_SACL_W 			L"S:(ML;;NW;;;LW)"
#define UNTRUSTED_INTEGRITY_SDDL_SACL_W 	L"S:(ML;;NW;;;S-1-16-0)"

/*! 
 * Named pipe implementation class. 
 */
#include <jio/xplatform.hpp>
#include <jio/exception.hpp>
#include <jio/transports/common.hpp>

#include <thread>

namespace jio {
	namespace transports {

		struct named_pipe_settings {
			uint32_t timeout;
			uint32_t max_buffer_in;
			uint32_t max_buffer_out;
			bool use_completion_port;
		};
		named_pipe_settings named_pipe_settings_default = { 1000, 4096, 4096, true };

		class named_pipe :
			public base_pipe {
		public:
			/*!
			*	Initialize named pipe implementation on the pipe of pipename. This will create the pipe and begin listening if its the server.
			*
			* 	@param [in] pipename The fully qualified path of the named pipe to call.
			*	@param [in] is_server Boolean on whether this is a client or server instance of the class.
			**/
			named_pipe(const std::string & pipename, bool is_server = false, named_pipe_settings settings = named_pipe_settings_default) {
				_name = pipename;
				_server = is_server;
				_settings = settings;

				// initialize our handles appropriately
				_hPipe				= INVALID_HANDLE_VALUE;
				_hCompletionPort	= INVALID_HANDLE_VALUE;
				_hListenerThread	= INVALID_HANDLE_VALUE;


				// create the server here and flag us as already opened
				if (_server) {
					_server_create();
				}
			}

			/*!
			*	Destructor for our named pipe win32 implementation. This function will perform the appropriate disconnections and cleanup
			*/
			~named_pipe() {
				close();
			}

			bool open(void) { 
				
				// Bail out if we are the server and just pretend we are always open
				if (_server) {
					return true;
				}

				

			}
			void close(void) {
				
				DisconnectNamedPipe(_hPipe);

				CloseHandle(_hCompletionPort);
				CloseHandle(_hPipe);
			}

			/*!
			*	Reads the next full message off the named pipe buffer
			*/
			message & read(void) {
				THROW_NOT_IMPL();
			}
			/*!
			*	Writes the next message to the named pipe buffer.
			*/
			uint32_t write(const message & data) {
				THROW_NOT_IMPL();
			}

			/*!
			*	Flush the pipe where applicable.
			*/
			void flush(void) {
				THROW_NOT_IMPL();
			}
		
			/*!
			*	Returns the settings used to create this pipe.
			*/
			const named_pipe_settings & named_pipe_settings() const { return _settings; }
		

		protected:
			// Internal win32-specific properties are here.
			jio::transports::named_pipe_settings _settings;
			HANDLE _hPipe;
			HANDLE _hCompletionPort;
			HANDLE _hListenerThread;

		protected:
			/*!
			*	Internal function for creating a named pipe in win32 land
			*/
			bool _server_create(void) {

				// This SA creates a untrusted IL named pipe allow all
				SECURITY_ATTRIBUTES sa;
				sa.nLength = sizeof(SECURITY_ATTRIBUTES);
				sa.lpSecurityDescriptor = jio::transports::named_pipe::security::get_untrusted_sa();
				sa.bInheritHandle = FALSE;

				// Try to create the named pipe. On error, throw an except with the detailed error.
				if ((_hPipe = ::CreateNamedPipeA(
					_name.c_str(),
					PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
					PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
					PIPE_UNLIMITED_INSTANCES, 
					_settings.max_buffer_out,
					_settings.max_buffer_in,
					_settings.timeout,
					&sa)) == INVALID_HANDLE_VALUE) { 
					
					// free the security descriptor on error
					::LocalFree(sa.lpSecurityDescriptor);
					throw EXCEPT_TEXT(jio::exception, GetLastError(), jio::xplatform::GetLastErrorAsString());
				}

				// free the security desc. anyways
				::LocalFree(sa.lpSecurityDescriptor);

				// 
				// if enabled, create the completion port and utilize threaded listener methodology
				//
				if (_settings.use_completion_port) {
					
					if ((_hCompletionPort = ::CreateIoCompletionPort(
						_hPipe,
						NULL,
						0,
						0)) == INVALID_HANDLE_VALUE) {
						close();
						throw EXCEPT_TEXT(jio::exception, GetLastError(), jio::xplatform::GetLastErrorAsString());
					}
					

				}

				// return true, we are done
				return true;
			}

		public:
			/*!
			*	This inner-class handles security for the pipes including firewall, registry and security descriptor creation.
			*	This is intended to work around any possible permissions humbuggery involved in named pipe access in windows.
			*/
			class security {
			public:
				static void set_firewall_current_allow(void) {

				}
				static void set_firewall_current_delete(void) {

				}

				static PSECURITY_DESCRIPTOR get_untrusted_sa(void) {
					SECURITY_ATTRIBUTES sa;
					PSECURITY_DESCRIPTOR pSD;
					
					pSD = NULL;
					if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(UNTRUSTED_INTEGRITY_SDDL_SACL_W, SDDL_REVISION_1, &pSD, NULL)) {
						throw EXCEPT_TEXT(jio::exception, GetLastError(), jio::xplatform::GetLastErrorAsString());
					}

					return pSD;
				}
			};
		};
	}
}