#include "FreqUsedSymbolsOfTesting.h"
#include "MT/ThreadPool.h"

TEST(AFcfsTaskScheduler, CanVerifyEmptiness)
{
	FcfsTaskScheduler<int()> taskScheduler;

	ASSERT_TRUE(taskScheduler.IsEmpty());
}

TEST(AFcfsTaskScheduler, SchedulesTaskInOrderTheyGotPushed)
{
	FcfsTaskScheduler<int()> taskScheduler;
	
	for (int i = 0; i < 10; ++i)
	{
		taskScheduler.PushTask([i]()->int {return i; });
	}

	for (int i = 0; i < 10; ++i)
	{
		auto result = taskScheduler.GetNextTask()();
		ASSERT_EQ(result, i);
	}
}

template<typename F>
class MockTaskScheduler;

template<typename R, typename... ARGS>
class MockTaskScheduler<R(ARGS...)> : public TaskScheduler<R(ARGS...)>
{
public:
	MOCK_METHOD(std::function<R(ARGS...)>, GetNextTask, (), (override));
    MOCK_METHOD(void, PushTask, (std::function<R(ARGS...)> task), (noexcept, override));
	MOCK_METHOD(bool, IsEmpty, (), (const , noexcept, override));

	std::queue<std::function<R(ARGS...)>> m_Tasks;
	std::mutex m_Mtx;
};

using SchedulerMockAlias = testing::NiceMock<MockTaskScheduler<void()>>;

class ThreadPoolTest : public ::testing::Test
{
protected:

	ThreadPoolTest()
	{
		scheduler = std::make_unique<SchedulerMockAlias>();
		schedulerPtr = scheduler.get();
	}

	virtual ~ThreadPoolTest()
	{

	}

	void SetUp() override
	{
		ON_CALL(*schedulerPtr, PushTask(testing::_)).WillByDefault(testing::Invoke(
			[&](std::function<void()> task)->void
			{
				std::lock_guard lock(schedulerPtr->m_Mtx);
				schedulerPtr->m_Tasks.push(task);
			}));

		ON_CALL(*schedulerPtr, GetNextTask()).WillByDefault(testing::Invoke(
			[&]()->std::function<void()>
			{
				std::lock_guard lock(schedulerPtr->m_Mtx);

				if (schedulerPtr->m_Tasks.empty())
					return nullptr;
				auto task = schedulerPtr->m_Tasks.front();
				schedulerPtr->m_Tasks.pop();

				return task;
			}));

		ON_CALL(*schedulerPtr, IsEmpty()).WillByDefault(testing::Invoke(
			[&]()->bool
			{
				std::lock_guard lock(schedulerPtr->m_Mtx);

				return schedulerPtr->m_Tasks.empty();
			}));
	}

	void TearDown() override
	{
		if (pool)
		{
			pool.reset();
		}
	}

	
	std::unique_ptr<ThreadPool> pool;
	std::unique_ptr<SchedulerMockAlias> scheduler;
	SchedulerMockAlias* schedulerPtr;
};

TEST(AThreadPool, CantHaveZeroThreads)
{
	EXPECT_THROW(ThreadPool(nullptr, 0), std::runtime_error);
}

TEST_F(ThreadPoolTest, UsesTaskSchedulerAndCanPauseTaskExecution)
{
	std::atomic_bool invokedTasks{false};

	std::barrier barrier{ 2 };

	pool = std::make_unique<ThreadPool>(std::move(scheduler) , 1);

	pool->Pause();

	pool->Execute([&]()
		{
			invokedTasks.store(true, std::memory_order_relaxed);

			barrier.arrive_and_wait();
		}
			);

	ASSERT_FALSE(invokedTasks.load(std::memory_order_seq_cst));

	pool->Resume();

	barrier.arrive_and_wait();

	ASSERT_TRUE(invokedTasks.load(std::memory_order_relaxed));
}

TEST_F(ThreadPoolTest, AdheresToItsScheduler)
{
	std::vector<int> invokedTasks;

	std::barrier barrier{ 6 };

	pool = std::make_unique<ThreadPool>(std::move(scheduler), 1);

	//We are pausing to prove that every task is running
	//with the scheduler order and it's not random behaviour
	pool->Pause();

	for (int i = 0; i < 5; ++i)
	{
		pool->Execute([& , i]()
			{
				invokedTasks.emplace_back(i + 1);

				barrier.arrive();
			});
	}

	pool->Resume();

	barrier.arrive_and_wait();

	for (int i = 0; i < 5; ++i)
		ASSERT_EQ(invokedTasks[i] , i + 1);
}

TEST_F(ThreadPoolTest, RunsTasksInParallelIfPossible)
{
	std::array<std::atomic<std::shared_ptr<std::thread::id>>,5> threadIDs;

	std::barrier barrier{ 6 };

	pool = std::make_unique<ThreadPool>(std::move(scheduler), 5);

	pool->Pause();

	for (int i = 0; i < 5; ++i)
	{
		pool->Execute([&, i]()
			{
				threadIDs[i].store(std::make_shared<std::thread::id>(std::this_thread::get_id()), std::memory_order_relaxed);

				barrier.arrive_and_wait();
			});
	}

	pool->Resume();

	barrier.arrive_and_wait();

	std::unordered_set<std::thread::id> uniqueThreadIDs;

	for (int i = 0; i < 5; ++i)
		uniqueThreadIDs.insert(*threadIDs[i].load(std::memory_order_relaxed));

	ASSERT_EQ(uniqueThreadIDs.size() , 5);
}

class ThreadPoolThiefTest : public ThreadPoolTest
{
protected:

	ThreadPoolThiefTest()
	{
		theifScheduler = std::make_unique<SchedulerMockAlias>();
		thiefSchedulerPtr = theifScheduler.get();
	}

	~ThreadPoolThiefTest()
	{

	}

	void SetUp() override
	{
		ThreadPoolTest::SetUp();

		ON_CALL(*thiefSchedulerPtr, PushTask(testing::_)).WillByDefault(testing::Invoke(
			[&](std::function<void()> task)->void
			{
				std::lock_guard lock(thiefSchedulerPtr->m_Mtx);
				thiefSchedulerPtr->m_Tasks.push(task);
			}));

		ON_CALL(*thiefSchedulerPtr, GetNextTask()).WillByDefault(testing::Invoke(
			[&]()->std::function<void()>
			{
				std::lock_guard lock(thiefSchedulerPtr->m_Mtx);

				if (thiefSchedulerPtr->m_Tasks.empty())
					return nullptr;
				auto task = thiefSchedulerPtr->m_Tasks.front();
				thiefSchedulerPtr->m_Tasks.pop();

				return task;
			}));

		ON_CALL(*thiefSchedulerPtr, IsEmpty()).WillByDefault(testing::Invoke(
			[&]()->bool
			{
				std::lock_guard lock(thiefSchedulerPtr->m_Mtx);

				return thiefSchedulerPtr->m_Tasks.empty();
			}));
	}

	void TearDown() override
	{
		ThreadPoolTest::TearDown();

		if (thiefPool)
		{
			thiefPool->Shutdown();
			thiefPool.reset();
		}
	}


	std::unique_ptr<ThreadPool> thiefPool;
	std::unique_ptr<SchedulerMockAlias> theifScheduler;
	SchedulerMockAlias* thiefSchedulerPtr;
};

TEST_F(ThreadPoolThiefTest, SupportsWorkStealing)
{
	pool = std::make_unique<ThreadPool>(std::move(scheduler), 1);
	thiefPool = std::make_unique<ThreadPool>(std::move(theifScheduler), 1);

	std::array<std::unique_ptr<std::thread::id>,2> threadIDs;

	std::latch latch { 2 };

	std::condition_variable cond;
	std::mutex mtx;

	pool->Execute([&]()
		{
			threadIDs[0] = std::make_unique<std::thread::id>(std::this_thread::get_id());

			latch.arrive_and_wait();
		}
	);

	pool->Execute([&]()
		{
			std::unique_lock lock(mtx);

			threadIDs[1] = std::make_unique<std::thread::id>(std::this_thread::get_id());

			lock.unlock();
			cond.notify_one();
		}
	);

	thiefPool->ForceWakeUp();

	{
		std::unique_lock lock(mtx);
		cond.wait_for(lock, std::chrono::seconds{ 5 }, [&]()->bool { return threadIDs[1] != nullptr; });
		
	}

	latch.count_down();

	{
		std::unique_lock lock(mtx);
		cond.wait(lock, [&]()->bool { return threadIDs[1] != nullptr; });
	}

	ASSERT_NE(*threadIDs[0], *threadIDs[1]);
	ASSERT_NE(*threadIDs[0], std::this_thread::get_id());
	ASSERT_NE(*threadIDs[1], std::this_thread::get_id());
}