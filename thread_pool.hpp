#pragma once

#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <deque>

namespace jio {

	// the actual thread pool
	class thread_pool {
	public:
		thread_pool(const size_t threads);
		~thread_pool();

		template<class F> void enqueue(F f);
		void stop();
		
	private:
		friend class thread_worker;

		std::vector< std::thread > _workers;
		std::deque< std::function<void()> > _tasks;

		std::mutex _queue_mutex;
		std::condition_variable _condition;

		bool _stop;
	};

	class thread_worker {
	public:
		thread_worker(thread_pool &s) : _pool(s) { }
		
		void operator()() {
			std::function<void()> task;

			while (true) {
				{
					std::unique_lock<std::mutex> lock(_pool._queue_mutex);

					while (!_pool._stop && _pool._tasks.empty()) {
						_pool._condition.wait(lock);
					}

					if (_pool._stop) {
						return;
					}

					task = _pool._tasks.front();
					_pool._tasks.pop_front();

				}

				task();
			}
		}
	private:
		thread_pool &_pool;
	};

	// the actual thread pool
	thread_pool::thread_pool(const size_t threads)
		: _stop(false)
	{
		for (size_t i = 0; i < threads; ++i) {
			_workers.push_back(std::thread(thread_worker(*this)));
		}
	}

	thread_pool::~thread_pool() {
		_stop = true;
		_condition.notify_all();

		for (size_t i = 0; i < _workers.size(); ++i) {
			_workers[i].join();
		}
	}

	template<class F> void thread_pool::enqueue(F f) {
		{ 
			std::unique_lock<std::mutex> lock(_queue_mutex);
			_tasks.push_back(std::function<void()>(f));
		}

		_condition.notify_one();
	}

	void thread_pool::stop() { _stop = true; }
}