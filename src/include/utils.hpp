#ifndef UTILS_HPP_INCLUDE__
#define UTILS_HPP_INCLUDE__

#include <thread>
#include <vector>
#include <mutex>
#include <chrono>
#include <algorithm>
#include <memory>

namespace flowTumn{

template <
		typename T
	,	typename Guard = ::std::lock_guard <T>
	,	typename Ptr = ::std::unique_ptr <Guard>
>
inline auto make_lock_guard(T& mutex) -> Ptr {
	return Ptr{new Guard(mutex)};
}

inline void sleepFor(int64_t ms) {
	::std::this_thread::sleep_for(::std::chrono::milliseconds(ms));
}

template <typename T>
inline void join(T& threads) {
	static_assert(
			::std::is_base_of <typename T::value_type, ::std::thread>::value
		,	"T must be a descendant of std::thread."
	);

	::std::for_each(
			::std::begin(threads)
		,	::std::end(threads)
		,	[](::std::thread& v) {
				if (v.joinable()) {
					v.join();
				}
			}
	);
}

};

#endif
