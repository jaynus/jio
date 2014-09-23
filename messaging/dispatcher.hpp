#pragma once

#include <jio/xplatform.hpp>
#include <jio/exception.hpp>
#include <jio/thread_pool.hpp>

#include <map>
#include <memory>
#include <future>

namespace jio {
	namespace messaging {
		class dispatcher;
		
		class dispatch;
		typedef std::shared_ptr<dispatch> dispatch_p;

		typedef std::function<dispatch *(dispatcher *, dispatch_p)> dispatch_func_t;

		class dispatch_settings {
		public:
			dispatch_settings(bool _isolate = false, bool _async = false, dispatch_func_t _callback = nullptr)
				: isolate(_isolate), async(_async), callback(callback) {}
			~dispatch_settings(){}

			dispatch_settings(const dispatch_settings & in) : isolate(in.isolate), async(in.async), callback(in.callback) {  }
			dispatch_settings & operator=(const dispatch_settings & in) { isolate = in.isolate; async = in.async; callback = in.callback; return *this; }

			bool operator==(const dispatch_settings & in) const { return (isolate == in.isolate && async == in.async && id == in.id ? true : false); }

			bool isolate;
			bool async;

			size_t id;

			dispatch_func_t callback;
		};

		class dispatch {
		public:
			dispatch(uint32_t call_id, std::shared_ptr<char> _data, dispatch_settings _settings = dispatch_settings())
				: id(call_id), settings(_settings) {}
			~dispatch() {}

			dispatch(const dispatch & in)
				: id(in.id), settings(in.settings) {}
			dispatch & operator=(const dispatch & in) { id = in.id; settings = in.settings; data = in.data; return *this; }

			bool operator==(const dispatch & in) const { return (id == in.id && settings == in.settings ? true : false); }

			size_t id;
			dispatch_settings settings;
			std::shared_ptr<char> data;
		};

		class dispatcher {
		public:
			dispatcher(bool isolate = false, size_t workerCount = 5) : _isolated(isolate), _pool(jio::thread_pool(workerCount)) {}
			~dispatcher() {}

			bool add(size_t id, const std::string & name, const dispatch_func_t & handler) {
				_id_to_dispatch[id] = handler;
				_name_to_dispatch[name] = handler;

				return true;
			}

			dispatch call(dispatch_p disp) {
				
				if (!_id_to_dispatch[disp->id])
					THROW_NOT_IMPL();

				_id_to_dispatch[disp->id](this, disp);

			}

			bool isolated() const { return _isolated; }
		protected:
			bool _isolated;

			jio::thread_pool & _pool;
			
			std::map<size_t, dispatch_func_t> _id_to_dispatch;
			std::map<std::string, dispatch_func_t> _name_to_dispatch;
		};

		
		

	}
}