#include <iostream>
#include <atomic>
#include <algorithm>
#include <iterator>
#include <future>
#include "ConcurrentQueue.hpp"
#include "executor.hpp"

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

	flowTumn::join(popThreads);
	flowTumn::join(pushThreads);

	assert((PUSH_THREAD_COUNT * PUSH_MAX_COUNT) - (POP_THREAD_COUNT * POP_MAX_COUNT) == ints.size());

}

void serviceTest() {
	const auto POST_COUNT = 0xFFFF;
	flowTumn::service service;
	::std::atomic <int> counter{ 0 };
	auto thread = ::std::thread{ [&service](){service.run(); } };

	//別スレッドで実行する状態になっているのでpostし続ける
	for (int i = 0; i < POST_COUNT; ++i) {
		service.post([&counter, i](){++counter;  assert(i == i); });
	}

	//全部流れたことを知るまで待機
	::std::promise <void> future;

	service.post([&future](){future.set_value(); });
	future.get_future().get();
	service.stop();
	thread.join();

	assert(counter == POST_COUNT);
}

void executorTest() {
	auto exec1 = flowTumn::executor::createExecutor(4, 8);

	assert(exec1->busy() == 0);
	//assert(exec1->count() == 4);



	//assert(exec1->busy() == 8);

}

void testAll() {
	queueTestPop();
	queueTestPushPop();
	serviceTest();
	executorTest();
}

int main(int argc, char **argv) {
	testAll();
}