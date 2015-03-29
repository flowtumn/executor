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
		executor(uint32_t minThread, uint32_t maxThread)
			: minThread_(minThread)
			, maxThread_(maxThread)
			, alive_(true)
			, busy_(0)
			, threadCount_(0) {
		}

	public:
		using executor_ptr = ::std::unique_ptr <executor>;

		~executor() {
			this->terminate();
		}

		uint32_t count() const {
			return this->threadCount_;
		}

		uint32_t busy() const {
			return this->busy_;
		}

		void terminate() {
			auto lock = flowTumn::make_lock_guard(this->mutex_);
			this->alive_ = false;
			this->service_.stop();
			flowTumn::join(this->threads_);
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
		void execute(F f, int32_t count = INT32_C(0), uint32_t cycleMS = UINT32_C(0)) {
			//0 => 1
			count = (INT32_C(0) == count) ? 1 : count;
			this->execute(f, ::std::chrono::high_resolution_clock::now(), count, cycleMS);
		}

	private:
		//thread append.
		void append() {
			auto lock = flowTumn::make_lock_guard(this->mutex_);

			if (this->alive_ && (this->threadCount_ < this->maxThread_)) {
				::std::promise <void> promise;
				++this->threadCount_;
				this->threads_.emplace_back(
					::std::thread{
						::std::bind(
								&executor::core
							,	this
							,	::std::ref(promise)
						)
					}
				);

				promise.get_future().get();
			}
		}

		//thread core.
		void core(::std::promise <void>& notify) {
			notify.set_value();
			while (this->alive_) {
				this->service_.run();
			}
			--this->threadCount_;
		}

		template <typename F>
		void execute(F f, decltype(::std::chrono::high_resolution_clock::now()) now, int32_t count, uint32_t cycleMS) {

			if (this->busy() + 1 <= this->threadCount_) {
				if (this->threadCount_ < this->maxThread_) {
					//busy all.. append.
					this->append();
				}
			}

			this->service_.post(
				[this, f, now, cycleMS, count]() mutable {
					++this->busy_;

					if (::std::chrono::milliseconds(cycleMS) < ::std::chrono::high_resolution_clock::now() - now) {
						f();
						--count;

						//now update.
						now = ::std::chrono::high_resolution_clock::now();
					} else {
						flowTumn::sleepFor(1);
					}

					if (INT32_C(0) > count || INT32_C(0) < count) {
						this->execute(f, now, count, cycleMS);
					}

					--this->busy_;
				}
			);
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
