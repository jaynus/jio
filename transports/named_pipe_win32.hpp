#pragma once

#define LOW_INTEGRITY_SDDL_SACL_W 			L"S:(ML;;NW;;;LW)"
#define UNTRUSTED_INTEGRITY_SDDL_SACL_W 	L"S:(ML;;NW;;;S-1-16-0)"

/*! 
 * Named pipe implementation class. 
 */
#include <jio/xplatform.hpp>
#include <jio/exception.hpp>
#include <jio/transports/common.hpp>


namespace jio {
	namespace transports {
		class named_pipe :
			public base_pipe {
		public:
			/*!
			*	Initialize named pipe implementation on the pipe of pipename
			*
			* 	@param [in] pipename The fully qualified path of the named pipe to call.
			**/
			named_pipe(const std::string & pipename) {
				_name = pipename;
			}
			~named_pipe() {}

			void open(void) {}
			void close(void) {}

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

			void flush(void) {}


		public:
			/*!
			*	This inner-class handles security for the pipes including firewall, registry and security descriptor creation.
			*	This is intended to work around any possible permissions humbuggery involved in named pipe access in windows.
			*/
			class security {
				static void set_firewall_current_allow(void) {

				}
				static void set_firewall_current_delete(void) {

				}

				static PSECURITY_DESCRIPTOR get_untrusted_sa(void) {
					PSECURITY_DESCRIPTOR pSD;
					
					pSD = NULL;
					ConvertStringSecurityDescriptorToSecurityDescriptorW(UNTRUSTED_INTEGRITY_SDDL_SACL_W, SDDL_REVISION_1, &pSD, NULL);

					return pSD;
				}
			};
		};
	}
}