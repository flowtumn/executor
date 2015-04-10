#ifndef EXECUTOR_HPP_INCLUDE__
#define EXECUTOR_HPP_INCLUDE__

#include <thread>
#include <atomic>
#include <vector>
#include <memory>
#include <unordered_map>
#include "concurrent_queue.hpp"
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
		flowTumn::concurrent_queue <F> queue_;
		::std::atomic <bool> terminate_;
	};

	class executor {
		enum class TaskState {
			TaskStart,
			TaskSuspend,	//reserve
			TaskFinish,
			TaskStop,
			TaskStateNone,
		};

		executor(uint32_t minThread, uint32_t maxThread)
			:	minThread_(minThread)
			,	maxThread_(maxThread)
			,	alive_(true)
			,	busy_(0)
			,	threadCount_(0)
			,	id_(0) {
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

		//stop task.
		bool stopTask(int64_t id, bool blocking = false) {
			if (TaskState::TaskFinish == this->getAndSetTaskState(id, TaskState::TaskStop)) {
				return true;
			}

			this->setTaskState(id, TaskState::TaskStop);

			while (blocking) {
				if (TaskState::TaskFinish == this->getTaskState(id)) {
					return true;
				}
				flowTumn::sleepFor(20);
			}

			return (TaskState::TaskFinish == this->getTaskState(id));
		}

		//exec.
		template <typename F>
		int64_t execute(F f, int32_t count = INT32_C(0), uint32_t cycleMS = UINT32_C(0)) {
			//0 => 1
			count = (INT32_C(0) == count) ? 1 : count;
			this->execute(f, ::std::chrono::high_resolution_clock::now(), count, cycleMS, ++this->id_);
			return this->id_;
		}

	private:
		//task start.
		void setTaskState(int64_t id, TaskState state) {
			auto lock = flowTumn::make_lock_guard(this->taskMutex_);
			this->taskMap_[id] = state;
		}

		//task state get and set.
		TaskState getAndSetTaskState(int64_t id, TaskState state) {
			auto result = this->getTaskState(id);

			this->setTaskState(id, state);
			return result;
		}

		//task state.
		TaskState getTaskState(int64_t id) {
			auto lock = flowTumn::make_lock_guard(this->taskMutex_);

			if (this->taskMap_.end() != this->taskMap_.find(id)) {
				return this->taskMap_[id];
			}

			return TaskState::TaskStateNone;
		}

		//thread append.
		void append() {
			auto lock = flowTumn::make_lock_guard(this->mutex_);

			if (this->alive_ && (this->threadCount_ < this->maxThread_)) {
				//thread callback.
				::std::promise <void> promise;
				auto f = [&promise]() {
					flowTumn::sleepFor(1);
					promise.set_value();
				};

				this->threads_.emplace_back(
					::std::thread{
						[this, &f]() {
							++this->threadCount_;
							this->core(f);
							--this->threadCount_;
						}
					}
				);

				promise.get_future().get();
			}
		}

		//thread core.
		template <typename F>
		void core(F f) {
			// f execute to another thread.
			this->service_.post(f);

			while (this->alive_) {
				this->service_.run();
			}
		}

		template <typename F>
		void execute(F f, decltype(::std::chrono::high_resolution_clock::now()) now, int32_t count, uint32_t cycleMS, int64_t id) {

			if (!this->alive_) {
				return;
			}

			//task check.
			if (TaskState::TaskStop == this->getTaskState(id)) {
				//task end.
				this->setTaskState(id, TaskState::TaskFinish);
				return;
			}

			if (this->busy() + 1 <= this->threadCount_) {
				if (this->threadCount_ < this->maxThread_) {
					//busy all.. append.
					this->append();
				}
			}

			this->service_.post(
				[this, f, now, cycleMS, count, id]() mutable {
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
						this->execute(f, now, count, cycleMS, id);
					} else {
						//end task.
						this->setTaskState(id, TaskState::TaskFinish);
					}

					--this->busy_;
				}
			);
		}

		service service_;
		uint32_t minThread_;
		uint32_t maxThread_;
		::std::mutex mutex_;
		::std::mutex taskMutex_;
		::std::atomic <bool> alive_;
		::std::atomic <uint32_t> busy_;
		::std::atomic <uint32_t> threadCount_;
		::std::atomic <int64_t> id_;
		::std::unordered_map < int64_t, TaskState> taskMap_;
		::std::thread threadCycle_;
		::std::vector < ::std::thread> threads_;
	};
};

#endif
