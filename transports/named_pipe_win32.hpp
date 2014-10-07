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
#include <string>

namespace jio {
	namespace transports {

		typedef struct named_pipe_settings {
			uint32_t timeout;
			uint32_t max_buffer_in;
			uint32_t max_buffer_out;
			bool use_completion_port;
			std::wstring application_name;
			named_pipe_settings() {
				timeout = 1000;
				max_buffer_in = 4096;
				max_buffer_out = 4096;
				use_completion_port = true;
				application_name = L"";
			}
			named_pipe_settings(const named_pipe_settings & in) {
				timeout = in.timeout;
				max_buffer_in = in.max_buffer_in;
				max_buffer_out = in.max_buffer_out;
				use_completion_port = in.use_completion_port;
				application_name = in.application_name;
			}
		} named_pipe_settings;
		

		class named_pipe :
			public base_pipe {
		public:
			/*!
			*	Initialize named pipe implementation on the pipe of pipename. This will create the pipe and begin listening if its the server.
			*
			* 	@param [in] pipename The fully qualified path of the named pipe to call.
			*	@param [in] is_server Boolean on whether this is a client or server instance of the class.
			**/

			named_pipe(const std::string & pipename, named_pipe_settings settings, bool is_server = false) {
				_name = pipename;
				_server = is_server;
				_settings = settings;

				// initialize our handles appropriately
				_hPipe				= INVALID_HANDLE_VALUE;
				_hCompletionPort	= INVALID_HANDLE_VALUE;

				// create the server here and flag us as already opened
				if (_server) {
					_server_create();
				} else {
					open();
				}

				// finally, allocate a generic read and write buffer
				_inputBuffer = new unsigned char[_settings.max_buffer_in];
				_outputBuffer = new unsigned char[_settings.max_buffer_in];
			}

			/*!
			*	Destructor for our named pipe win32 implementation. This function will perform the appropriate disconnections and cleanup
			*/
			~named_pipe() {
				close();
				delete _inputBuffer;
				delete _outputBuffer;
			}

			bool open(void) { 
				
				// Bail out if we are the server and just pretend we are always open
				if (_server) {
					return true;
				}
				// Client functionality

				// Connect to the pipe
				_hPipe = CreateFileA(
					_name.c_str(),			// pipe name 
					GENERIC_READ |			// read and write access 
					GENERIC_WRITE,
					0,						// no sharing 
					NULL,					// default security attributes
					OPEN_EXISTING,			// opens existing pipe 
					FILE_ATTRIBUTE_NORMAL,	// default attributes 
					NULL);					// no template file 
				
				if (_hPipe == INVALID_HANDLE_VALUE) {
					if (GetLastError() == ERROR_PIPE_BUSY) {
						// TODO: Eventually handle reconnects here
						throw EXCEPT_TEXT(jio::exception, GetLastError(), jio::xplatform::GetLastErrorAsString());
					}
					else {
						// real error
						throw EXCEPT_TEXT(jio::exception, GetLastError(), jio::xplatform::GetLastErrorAsString());
					}
				}


			}
			void close(void) {
				
				if (_hPipe != INVALID_HANDLE_VALUE) {
					if (is_server()) {
						DisconnectNamedPipe(_hPipe);
						if (_settings.use_completion_port) {
							CloseHandle(_hCompletionPort);
						}
					}
				
					CloseHandle(_hPipe);
				}

				_hCompletionPort = INVALID_HANDLE_VALUE;
				_hPipe = INVALID_HANDLE_VALUE;
			}

			/*!
			*	Reads the next full message off the named pipe buffer
			*/
			message_p  read(void) {
				// Read a message off the buffer
				BOOL fSuccess = FALSE;
				DWORD cbRead = -1;
				message *returnMessage;

				fSuccess = ReadFile(_hPipe, _inputBuffer, _settings.max_buffer_in, &cbRead, NULL);
				if (!fSuccess) {
					return nullptr;
				}

				return std::shared_ptr<message_t <unsigned char *>>(new message(_inputBuffer, cbRead));
			}
			/*!
			*	Writes the next message to the named pipe buffer.
			*/
			uint32_t write(const message_p data) {
				BOOL fSuccess = FALSE;
				DWORD cbWritten = 0;

				// write the full packet
				while (cbWritten < data->length) {
					fSuccess = WriteFile(
						_hPipe,                  // pipe handle 
						data->data,             // message 
						data->length,              // message length 
						&cbWritten,             // bytes written 
						NULL);                  // not overlapped 
					if (!fSuccess) {
						throw EXCEPT_TEXT(jio::exception, GetLastError(), jio::xplatform::GetLastErrorAsString());
					}
				}
			}

			/*!
			*	Flush the pipe where applicable.
			*/
			void flush(void) {
				FlushFileBuffers(_hPipe);
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
			std::thread _listenerThread;
			unsigned char *_inputBuffer;
			unsigned char *_outputBuffer;

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
			
				return true;
			}

		public:
			/*!
			*	This inner-class handles security for the pipes including firewall, registry and security descriptor creation.
			*	This is intended to work around any possible permissions humbuggery involved in named pipe access in windows.
			*	from: http://msdn.microsoft.com/en-us/library/aa364726%28v=VS.85%29.aspx
			*/
			class security {
			public:
				// wrapper to provide module name
				static bool set_firewall_allow(const std::wstring &name) {
					// We need to pull our image name for the helper function to get it
					WCHAR filePathName[MAX_PATH];
					GetModuleFileName(NULL, filePathName, MAX_PATH);
					std::wstring modulename(filePathName);

					return security::set_firewall_allow(L"ACRE 2 Plugin", modulename);
				}

				static bool set_firewall_allow(const std::wstring & name, const std::wstring & imageFileName) {
					INetFwProfile*		fwProfile;
					INetFwMgr*			fwMgr;
					INetFwPolicy*		fwPolicy;
					HRESULT				hr;
//					VARIANT_BOOL		fwEnabled;

					hr			= S_OK;
					fwProfile	= NULL;
					fwPolicy	= NULL;
					fwMgr		= NULL;

					if ((hr = CoCreateInstance(__uuidof(NetFwMgr), NULL, CLSCTX_INPROC_SERVER, __uuidof(INetFwMgr), (void**)&fwMgr)) 
						== S_OK) {
						if ((hr = fwMgr->get_LocalPolicy(&fwPolicy)) 
							== S_OK) {
							if ((hr = fwPolicy->get_CurrentProfile(&fwProfile)) 
								== S_OK) {
								//
								// Okay we finally have a damn profile
								//
								INetFwAuthorizedApplication*	fwApp = NULL;
								INetFwAuthorizedApplications*	fwApps = NULL;
								BSTR							fwBstrProcessImageFileName = NULL;
								BSTR							fwBstrName = NULL;

								// Now we need to:
								// 1. Check if we are already enabled. If we are, just continue
								// 2. If we are NOT enabled, we need to add and enable ourselves
								if (!firewall_is_app_enabled(imageFileName, fwProfile)) {
									
									// App doesnt exist in the policy, we need to add it
									if (!firewall_is_app_in_profile(imageFileName, fwProfile)) {
										firewall_app_add(name, imageFileName, fwProfile);
									// App exists in the policy, we just need to enable it
									} else {
										firewall_app_toggle(imageFileName, fwProfile, true);
									}
								}
								// 
								// Unwind and clean up
								//
								fwProfile->Release();
							}
							fwPolicy->Release();
						} 
						fwMgr->Release();
					}

					// After unravelling, if we errored, throw our exception
					if (hr != S_OK) {
						throw EXCEPT_TEXT(jio::exception, GetLastError(), jio::xplatform::GetLastErrorAsString());
					}

					return true;
				}

				static PSECURITY_DESCRIPTOR get_untrusted_sa(void) {
	//				SECURITY_ATTRIBUTES sa;
					PSECURITY_DESCRIPTOR pSD;
					
					pSD = NULL;
					if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(UNTRUSTED_INTEGRITY_SDDL_SACL_W, SDDL_REVISION_1, &pSD, NULL)) {
						throw EXCEPT_TEXT(jio::exception, GetLastError(), jio::xplatform::GetLastErrorAsString());
					}

					return pSD;
				}

			protected:
				static bool firewall_app_add(const std::wstring & name, const std::wstring & imageFileName, INetFwProfile* fwProfile) {
					INetFwAuthorizedApplication*	fwApp = NULL;
					INetFwAuthorizedApplications*	fwApps = NULL;
					BSTR							fwBstrProcessImageFileName = NULL, fwBstrName = NULL;
					HRESULT							hr = S_OK;
					bool							result = false;

					// Allocate a BSTR for the process image file name.
					fwBstrProcessImageFileName = SysAllocString(imageFileName.c_str());
					if (fwBstrProcessImageFileName == NULL) {
						hr = E_OUTOFMEMORY;
						throw EXCEPT_TEXT(jio::exception, E_OUTOFMEMORY, "SysAllocString: Out of Memory");
					}
					fwBstrName = SysAllocString(name.c_str());
					if (fwBstrName == NULL) {
						hr = E_OUTOFMEMORY;
						throw EXCEPT_TEXT(jio::exception, E_OUTOFMEMORY, "SysAllocString: Out of Memory");
					}

					if ((hr = fwProfile->get_AuthorizedApplications(&fwApps))
						== S_OK) {
						if ((hr = fwApps->Item(fwBstrProcessImageFileName, &fwApp))
							== S_OK) {

							result = true;
							fwApp->Release();
						}
						else {
							// This is our appropriate handle case, it wasnt in the collection
							if ((hr = CoCreateInstance( __uuidof(NetFwAuthorizedApplication), NULL, CLSCTX_INPROC_SERVER, __uuidof(INetFwAuthorizedApplication), 
								(void**)&fwApp)) 
								== S_OK) {

								fwApp->put_Name(fwBstrName);
								fwApp->put_ProcessImageFileName(fwBstrProcessImageFileName);
								fwApp->put_Enabled(VARIANT_TRUE);
								fwApp->put_Scope(NET_FW_SCOPE_ALL);

								if ((hr = fwApps->Add(fwApp)) == S_OK) {
									result = true;
								} 

								fwApp->Release();
							}

						}
						fwApps->Release();
					}
					SysFreeString(fwBstrProcessImageFileName);

					return result;
				}
				static bool firewall_is_app_enabled(const std::wstring & imageFileName, INetFwProfile* fwProfile) {
					INetFwAuthorizedApplication*	fwApp = NULL;
					INetFwAuthorizedApplications*	fwApps = NULL;
					BSTR							fwBstrProcessImageFileName = NULL;
					HRESULT							hr = S_OK;
					bool							result = false;
					VARIANT_BOOL					fwEnabled = VARIANT_FALSE;

					// Allocate a BSTR for the process image file name.
					fwBstrProcessImageFileName = SysAllocString(imageFileName.c_str());
					if (fwBstrProcessImageFileName == NULL) {
						hr = E_OUTOFMEMORY;
						throw EXCEPT_TEXT(jio::exception, E_OUTOFMEMORY, "SysAllocString: Out of Memory");
					}

					if ((hr = fwProfile->get_AuthorizedApplications(&fwApps))
						== S_OK) {
						if ((hr = fwApps->Item(fwBstrProcessImageFileName, &fwApp))
							== S_OK) {
							if ((hr = fwApp->get_Enabled(&fwEnabled))
								== S_OK) {
								if (fwEnabled != VARIANT_FALSE) {
									result = true;
								}
							}
							fwApp->Release();
						}
						fwApps->Release();
					}
					SysFreeString(fwBstrProcessImageFileName);

					return result;
				}

				static bool firewall_is_app_in_profile(const std::wstring & imageFileName, INetFwProfile* fwProfile) {
					INetFwAuthorizedApplication*	fwApp = NULL;
					INetFwAuthorizedApplications*	fwApps = NULL;
					BSTR							fwBstrProcessImageFileName = NULL;
					HRESULT							hr = S_OK;
					bool							result = false;

					// Allocate a BSTR for the process image file name.
					fwBstrProcessImageFileName = SysAllocString(imageFileName.c_str());
					if (fwBstrProcessImageFileName == NULL) {
						hr = E_OUTOFMEMORY;
						throw EXCEPT_TEXT(jio::exception, E_OUTOFMEMORY, "SysAllocString: Out of Memory");
					}

					if ((hr = fwProfile->get_AuthorizedApplications(&fwApps))
						== S_OK) {
						if ((hr = fwApps->Item(fwBstrProcessImageFileName, &fwApp))
							== S_OK) {
							result = true;
							fwApp->Release();
						}
						fwApps->Release();
					}
					SysFreeString(fwBstrProcessImageFileName);

					return result;
				}

				static bool firewall_app_toggle(const std::wstring & imageFileName, INetFwProfile* fwProfile, bool enabled) {
					INetFwAuthorizedApplication*	fwApp = NULL;
					INetFwAuthorizedApplications*	fwApps = NULL;
					BSTR							fwBstrProcessImageFileName = NULL;
					HRESULT							hr = S_OK;
					bool							result = false;

					// Allocate a BSTR for the process image file name.
					fwBstrProcessImageFileName = SysAllocString(imageFileName.c_str());
					if (fwBstrProcessImageFileName == NULL) {
						hr = E_OUTOFMEMORY;
						throw EXCEPT_TEXT(jio::exception, E_OUTOFMEMORY, "SysAllocString: Out of Memory");
					}

					if ((hr = fwProfile->get_AuthorizedApplications(&fwApps))
						== S_OK) {
						if ((hr = fwApps->Item(fwBstrProcessImageFileName, &fwApp))
							== S_OK) {

							(enabled) ? fwApp->put_Enabled(VARIANT_TRUE) : fwApp->put_Enabled(VARIANT_FALSE);

							fwApp->Release();
						}
						fwApps->Release();
					}
					SysFreeString(fwBstrProcessImageFileName);

					return result;
				}
			};
		};
	}
}