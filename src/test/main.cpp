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
	const auto MULTIE_THREAD_MIN = 4;
	const auto MULTIE_THREAD_MAX = 12;

	auto exec1 = flowTumn::executor::createExecutor(1, 1);
	auto exec2 = flowTumn::executor::createExecutor(MULTIE_THREAD_MIN, MULTIE_THREAD_MAX);

	::std::atomic <int> counter1{0};
	::std::atomic <int> counter2{0};
	::std::atomic <int> counter3{0};

	assert(exec1->busy() == 0);
	assert(exec1->count() == 1);

	assert(exec2->busy() == 0);
	assert(exec2->count() == MULTIE_THREAD_MIN);

	//cycle 20ms, call count total 100.(single thread)
	exec1->execute([&counter1](){++counter1; return counter1.load(); }, 50, 20);
	exec1->execute([&counter1](){++counter1; return counter1.load(); }, 50, 20);

	//cycle 20ms.call count unlimit.(multie thread)
	for (int i = 0; i < 200; ++i) {
		exec2->execute([&counter2]() {++counter2; return counter2.load(); }, -1, 20);
	}

	for (int i = 1; i <= 10; ++i) {
		exec2->execute([&counter3]() {++counter3; return counter3.load(); }, i, 10);
	}

	flowTumn::sleepFor(5000);

	assert(counter1 == 100);
	assert(exec1->count() == 1);

	assert(counter3 == 55);

	//スレッド上限まで増えている
	assert(exec2->count() == MULTIE_THREAD_MAX);

	exec1->terminate();
	exec2->terminate();
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