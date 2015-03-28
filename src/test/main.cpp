#include <iostream>
#include "ConcurrentQueue.hpp"
#include <atomic>
#include <algorithm>
#include <iterator>

namespace {
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

//3秒待ってからqueueにデータを詰めて、popするか
void queueTestPop() {
	flowTumn::ConcurrentQueue <int> ints(10);
	::std::thread thr(
		[&ints]() {
			auto v = ints.pop([](){return true; });
			assert(v == 1000);
		}
	);

	// check 3sec. pop blocking.
	flowTumn::sleepFor(3000);

	// 1000 push.
	ints.push(1000);

	thr.join();
	
}

//push/popを繰り返し、データ数に問題がないか
void queueTestPushPop() {
	const auto POP_THREAD_COUNT = 5;
	const auto PUSH_THREAD_COUNT = 10;
	const auto POP_MAX_COUNT = 1000;
	const auto PUSH_MAX_COUNT = 2000;

	flowTumn::ConcurrentQueue <int> ints(10);
	::std::atomic <bool> terminate{false};

	::std::vector < ::std::thread> popThreads;
	::std::vector < ::std::thread> pushThreads;

	::std::generate_n(
			::std::back_inserter(popThreads)
		,	POP_THREAD_COUNT
		,	[&ints, POP_MAX_COUNT]() {
				return ::std::thread{
					[&ints, POP_MAX_COUNT]() {
						auto count = POP_MAX_COUNT;
						while (count) {
							ints.pop([](){return true; });
							--count;
						}
					}
				};
			}
	);

	::std::generate_n(
			::std::back_inserter(pushThreads)
		,	PUSH_THREAD_COUNT
		,	[&ints, PUSH_MAX_COUNT]() {
				return ::std::thread{
					[&ints, PUSH_MAX_COUNT]() {
						auto count = PUSH_MAX_COUNT;
						while (count) {
							ints.push(100);
							--count;
						}
					}
				};
			}
	);

	join(popThreads);
	join(pushThreads);

	assert((PUSH_THREAD_COUNT * PUSH_MAX_COUNT) - (POP_THREAD_COUNT * POP_MAX_COUNT) == ints.size());

}

void executorTest() {

}

void testAll() {
	queueTestPop();
	queueTestPushPop();
	executorTest();
}

int main(int argc, char **argv) {
	testAll();
	[](){::std::cout << "HelloWorld" << ::std::endl; }();
}