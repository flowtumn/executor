#include <iostream>
#include <atomic>
#include <algorithm>
#include <iterator>
#include <future>
#include <cassert>
#include "concurrent_queue.hpp"
#include "executor.hpp"

namespace {
	template <typename F>
	void repeatCall(F f, int32_t count) {
		for (auto i = INT32_C(0); i < count; ++i) {
			f(i);
		}
	}

	//queue FIFO check test.
	void queueSimpleTest() {
		const auto TEST_COUNT = 0xFFFF;
		flowTumn::concurrent_queue <int> queue;

		repeatCall(
				[&queue](int32_t i) {queue.push(i); }
			,	TEST_COUNT
		);

		repeatCall(
				[&queue](int32_t i) {assert(i == queue.pop(flowTumn::concurrent_queue <int>::popDefaultFunc())); }
			,	TEST_COUNT
		);
	}

	//3秒待ってからqueueにデータを詰めて、popするか
	void queueTestPop() {
		flowTumn::concurrent_queue <int> ints(10);
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

		flowTumn::concurrent_queue <int> ints(10);
		::std::atomic <bool> terminate{ false };

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
		repeatCall(
				[&service, &counter](int32_t i) {
					service.post([&counter, i](){++counter;  assert(i == i); });
				}
			,	POST_COUNT
		);

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

		::std::atomic <int> counter1{ 0 };
		::std::atomic <int> counter2{ 0 };
		::std::atomic <int> counter3{ 0 };

		auto exec1 = flowTumn::executor::createExecutor(1, 1);
		auto exec2 = flowTumn::executor::createExecutor(MULTIE_THREAD_MIN, MULTIE_THREAD_MAX);

		assert(exec1->busy() == 0);
		assert(exec1->count() == 1);

		assert(exec2->busy() == 0);
		assert(exec2->count() == MULTIE_THREAD_MIN);

		//cycle 20ms, call count total 100.(single thread)
		exec1->execute([&counter1](){++counter1; return counter1.load(); }, 50, 20);
		exec1->execute([&counter1](){++counter1; return counter1.load(); }, 50, 20);

		//cycle 20ms.call count unlimit.(multie thread)
		repeatCall(
				[&exec2, &counter2](int32_t) {
					exec2->execute([&counter2]() {++counter2; return counter2.load(); }, -1, 20);
				}
			,	200
		);

		repeatCall(
				[&exec2, &counter3](int32_t i) {
					i += 1;
					exec2->execute([&counter3]() {++counter3; return counter3.load(); }, i, 10);
				}
			,	10
		);

		flowTumn::sleepFor(5000);

		assert(counter1 == 100);
		assert(exec1->count() == 1);

		assert(counter3 == 55);

		//スレッド上限まで増えている
		assert(exec2->count() == MULTIE_THREAD_MAX);
	}

	//busy check.
	void testFunc(flowTumn::executor::executor_ptr& p, uint32_t busy) {
		if (p) {
			assert(p->busy() == busy);
			flowTumn::sleepFor(5000);
		}
		else {
			assert(false && "executor nullptr.");
		}
	}

	auto genBusyTestFunc(flowTumn::executor::executor_ptr& p, int busy) -> decltype(::std::bind(&testFunc, ::std::ref(p), busy)) {
		return ::std::bind(&testFunc, ::std::ref(p), busy);
	}

	void executorSequenceTest() {
		const auto THREAD_MIN = 1;
		const auto THREAD_MAX = 24;

		auto exec = flowTumn::executor::createExecutor(THREAD_MIN, THREAD_MAX);
		::std::atomic <bool> notify{ false };

		repeatCall(
				[&exec](int32_t i) {
					exec->execute(genBusyTestFunc(exec, i + 1));
				}
			,	THREAD_MAX
		);

		flowTumn::sleepFor(3000);
	}

	void taskStopTest() {
		const auto THREAD_MIN = 1;
		const auto THREAD_MAX = 4;
		const auto CREATE_TASK_COUNT = 5;

		auto exec = flowTumn::executor::createExecutor(THREAD_MIN, THREAD_MAX);
		::std::atomic <int32_t> totalCount{ 0 };
		::std::vector <int64_t> taskId;

		auto id1 = exec->execute([](){return;});
		assert(true == exec->stopTask(id1, true));

		for (int i = 0; i < CREATE_TASK_COUNT; ++i) {
			taskId.emplace_back(exec->execute([&totalCount]() {++totalCount; }, 100, 20));
		}

		flowTumn::sleepFor(3000);

		for (auto& v : taskId) {
			assert(true == exec->stopTask(v, true));
		}
	}

	void testAll() {
		queueSimpleTest();
		queueTestPop();
		queueTestPushPop();
		serviceTest();
		executorTest();
		executorSequenceTest();
		//taskStopTest();
	}

};

int main(int, char **) {
	testAll();
	return 0;
}