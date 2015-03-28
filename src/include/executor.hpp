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

		uint32_t count() const {
			return this->threadCount_;
		}

		uint32_t busy() const {
			return this->busy_;
		}

		//create.
		static executor_ptr createExecutor(uint32_t minThread, uint32_t maxThread) {
			auto mm = ::std::minmax(minThread, maxThread);
			if (auto p = executor_ptr{ new executor{ mm.first, mm.second } }) {
				p->append(mm.first);
				return p;
			}
			return nullptr;
		}

	private:
		//thread append.
		void append(uint32_t count) {

		}

		service service_;
		uint32_t minThread_;
		uint32_t maxThread_;
		::std::atomic <bool> alive_;
		::std::atomic <uint32_t> busy_;
		::std::atomic <uint32_t> threadCount_;
		::std::thread threadCycle_;
		::std::vector < ::std::thread> threads_;
	};
};

#endif
