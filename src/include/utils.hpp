#ifndef UTILS_HPP_INCLUDE__
#define UTILS_HPP_INCLUDE__

#include <thread>
#include <vector>
#include <mutex>
#include <chrono>
#include <algorithm>

namespace flowTumn{

template <typename T>
inline
::std::lock_guard <T> make_lock_guard(T& mutex) {
	return ::std::lock_guard <T>(mutex);
}

inline void sleepFor(int64_t ms) {
	::std::this_thread::sleep_for(::std::chrono::milliseconds(ms));
}

inline void join(::std::vector < ::std::thread>& threads) {
	::std::for_each(
			::std::begin(threads)
		,	::std::end(threads)
		,	[](::std::thread& v) {
				v.join();
			}
	);
}

};

#endif
