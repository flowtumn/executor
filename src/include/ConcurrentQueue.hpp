#ifndef CONCURRENTQUEUE_HPP_INCLUDE__
#define CONCURRENTQUEUE_HPP_INCLUDE__

#include <mutex>
#include <queue>
#include <chrono>
#include <thread>
#include <functional>
#include "utils.hpp"

namespace {
	int64_t DEFAULT_SLEEP_MS = INT64_C(10);
};

namespace flowTumn {

//ConccurentQueue.
template <typename T>
class ConcurrentQueue {
public:
	ConcurrentQueue(int64_t popCycle = DEFAULT_SLEEP_MS)
		: popCycle_(popCycle) {;}

	void push(const T& v) {
		auto lock = make_lock_guard(this->mutex_);
		this->queue_.push(v);
	}

	T pop(::std::function <bool ()> f) {
		while (f()) {
			{
				auto lock = make_lock_guard(this->mutex_);
				if (0 < this->queue_.size()) {
					auto v = this->queue_.front();
					this->queue_.pop();
					return v;
				}
			}
			sleepFor(popCycle_);
		}

		throw ::std::runtime_error("value not found,");
	}

	int64_t size() const {
		auto lock = make_lock_guard(this->mutex_);
		return this->queue_.size();
	}

	::std::queue <T> queue_;
	int64_t popCycle_;
	mutable ::std::mutex mutex_;
};

};

#endif
