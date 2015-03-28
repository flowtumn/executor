#ifndef EXECUTOR_HPP_INCLUDE__
#define EXECUTOR_HPP_INCLUDE__

#include <thread>
#include <atomic>
#include <vector>
#include <memory>
#include "ConcurrentQueue.hpp"
#include "utils.hpp"

namespace flowTumn{

	//service
	struct service {
		using F = ::std::function <void()>;

		service() : terminate_(false){;}

		void post(F f) {
			queue_.push(f);
		}

		void run() {
			try {
				while (auto v = queue_.pop([this](){return !terminate(); })) {
					v();
				}
			} catch (const ::std::runtime_error&) {
			}
		}

		void stop() {
			terminate_ = true;
		}

		bool terminate() {
			return terminate_;
		}

	private:
		flowTumn::ConcurrentQueue <F> queue_;
		::std::atomic <bool> terminate_;
	};

	class executor {
		using executor_ptr = ::std::unique_ptr <executor>;

		executor(uint32_t minThread, uint32_t maxThread)
			: minThread_(minThread)
			, maxThread_(maxThread)
			, alive_(true)
			, busy_(0)
			, threadCount_(0) {
		}

	public:

		~executor() {
			this->terminate();
		}

		uint32_t count() const {
			return this->threadCount_;
		}

		uint32_t busy() const {
			return this->busy_;
		}

		bool terminate() {
			this->alive_ = false;
			this->service_.stop();
			flowTumn::join(this->threads_);
			return true;
		}

		//create.
		static executor_ptr createExecutor(uint32_t minThread, uint32_t maxThread) {
			auto mm = ::std::minmax(minThread, maxThread);
			if (auto p = executor_ptr{ new executor{ mm.first, mm.second } }) {
				//min thread append.
				for (auto i = UINT32_C(0); i < mm.first; ++i) {
					p->append();
				}
				return p;
			}
			return nullptr;
		}

		//exec.
		template <typename F>
		void execute(F f, uint32_t cycleMS = UINT32_C(0), int32_t count = INT32_C(-1)) {
			this->service_.post(
				[this, f, cycleMS, count]() mutable {
					++this->busy_;

					f();

					--count;
					if (INT32_C(0) < count) {
						this->execute(f, cycleMS, count);
					}

					--this->busy_;
				}
			);
		}

	private:
		//thread append.
		void append() {
			auto lock = flowTumn::make_lock_guard(this->mutex_);
			this->threads_.emplace_back(
				::std::thread{
					::std::bind(
							&executor::core
						,	this
					)
				}
			);
		}

		//thread core.
		void core() {
			++this->threadCount_;
			while (this->alive_) {
				this->service_.run();
			}
			--this->threadCount_;
		}

		service service_;
		uint32_t minThread_;
		uint32_t maxThread_;
		::std::mutex mutex_;
		::std::atomic <bool> alive_;
		::std::atomic <uint32_t> busy_;
		::std::atomic <uint32_t> threadCount_;
		::std::thread threadCycle_;
		::std::vector < ::std::thread> threads_;
	};
};

#endif
