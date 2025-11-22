#include "FreqUsedSymbolsOfTesting.h"
#include "MT/JobSystem.h"
#include "MT/JobHandle.h"
#include "MT/Job.h"
#include <atomic>
#include <barrier>
#include <latch>
#include <chrono>
#include <thread>
#include <vector>

// ============================================================================
// JobHandle Tests
// ============================================================================

TEST(JobHandle, InvalidHandleIsAlwaysComplete)
{
    JobHandle handle;
    ASSERT_TRUE(handle.IsComplete());
    ASSERT_FALSE(handle.IsValid());

    // Waiting on invalid handle should not block
    handle.Wait(); // Should return immediately
}

TEST(JobHandle, ValidHandleStartsIncomplete)
{
    auto flag = std::make_shared<std::atomic<bool>>(false);
    JobHandle handle(flag);

    ASSERT_TRUE(handle.IsValid());
    ASSERT_FALSE(handle.IsComplete());
}

TEST(JobHandle, BecomesCompleteWhenFlagIsSet)
{
    auto flag = std::make_shared<std::atomic<bool>>(false);
    JobHandle handle(flag);

    ASSERT_FALSE(handle.IsComplete());

    flag->store(true, std::memory_order_release);

    ASSERT_TRUE(handle.IsComplete());
}

TEST(JobHandle, WaitBlocksUntilComplete)
{
    auto flag = std::make_shared<std::atomic<bool>>(false);
    JobHandle handle(flag);

    std::atomic<bool> waitReturned{ false };

    std::thread waiter([&]() {
        handle.Wait();
        waitReturned.store(true, std::memory_order_release);
        });

    // Give the waiter thread time to start waiting
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    ASSERT_FALSE(waitReturned.load(std::memory_order_acquire));

    // Complete the job
    flag->store(true, std::memory_order_release);

    waiter.join();
    ASSERT_TRUE(waitReturned.load(std::memory_order_acquire));
}

// ============================================================================
// JobSystem Basic Tests
// ============================================================================

TEST(JobSystem, CanBeConstructedWithDefaultThreadCount)
{
    ASSERT_NO_THROW({
        JobSystem jobSystem(0); // Should use hardware_concurrency - 1
        });
}

TEST(JobSystem, CanBeConstructedWithSpecificThreadCount)
{
    ASSERT_NO_THROW({
        JobSystem jobSystem(4);
        });
}

TEST(JobSystem, ReportsCorrectWorkerCount)
{
    JobSystem jobSystem(4);
    ASSERT_EQ(jobSystem.GetWorkerCount(), 4);
}

TEST(JobSystem, ExecutesSingleTask)
{
    JobSystem jobSystem(2);
    std::atomic<bool> executed{ false };

    auto handle = jobSystem.Schedule([&]() {
        executed.store(true, std::memory_order_release);
        });

    handle.Wait();

    ASSERT_TRUE(executed.load(std::memory_order_acquire));
    ASSERT_TRUE(handle.IsComplete());
}

TEST(JobSystem, ExecutesMultipleTasks)
{
    JobSystem jobSystem(4);
    std::atomic<int> counter{ 0 };

    std::vector<JobHandle> handles;
    for (int i = 0; i < 10; ++i) {
        handles.push_back(jobSystem.Schedule([&]() {
            counter.fetch_add(1, std::memory_order_relaxed);
            }));
    }

    jobSystem.WaitAll(handles);

    ASSERT_EQ(counter.load(std::memory_order_acquire), 10);
}

TEST(JobSystem, TasksExecuteInParallel)
{
    JobSystem jobSystem(4);

    std::array<std::atomic<std::thread::id>, 4> threadIds;
    std::barrier barrier{ 5 }; // 4 workers + 1 main thread

    std::vector<JobHandle> handles;
    for (int i = 0; i < 4; ++i) {
        handles.push_back(jobSystem.Schedule([&, i]() {
            threadIds[i].store(std::this_thread::get_id(), std::memory_order_relaxed);
            barrier.arrive_and_wait();
            }));
    }

    barrier.arrive_and_wait();
    jobSystem.WaitAll(handles);

    // Verify we got 4 different thread IDs
    std::unordered_set<std::thread::id> uniqueIds;
    for (int i = 0; i < 4; ++i) {
        uniqueIds.insert(threadIds[i].load(std::memory_order_relaxed));
    }

    ASSERT_EQ(uniqueIds.size(), 4);
}// ============================================================================
// Dependency Tests
// ============================================================================

TEST(JobSystem, ExecutesJobWithDependency)
{
    JobSystem jobSystem(2);

    std::atomic<int> value{ 0 };

    // First job sets value to 1
    auto job1 = jobSystem.Schedule([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        value.store(1, std::memory_order_release);
        }, {}, "Job1");

    // Second job depends on first, multiplies by 2
    auto job2 = jobSystem.Schedule([&]() {
        int current = value.load(std::memory_order_acquire);
        value.store(current * 2, std::memory_order_release);
        }, { job1 }, "Job2");

    job2.Wait();

    ASSERT_EQ(value.load(std::memory_order_acquire), 2);
}

TEST(JobSystem, HandlesMultipleDependencies)
{
    JobSystem jobSystem(4);

    std::atomic<int> sum{ 0 };

    // Create 3 jobs that each add a value
    auto job1 = jobSystem.Schedule([&]() {
        sum.fetch_add(1, std::memory_order_relaxed);
        });

    auto job2 = jobSystem.Schedule([&]() {
        sum.fetch_add(10, std::memory_order_relaxed);
        });

    auto job3 = jobSystem.Schedule([&]() {
        sum.fetch_add(100, std::memory_order_relaxed);
        });

    // Final job depends on all three
    auto finalJob = jobSystem.Schedule([&]() {
        sum.fetch_add(1000, std::memory_order_relaxed);
        }, { job1, job2, job3 });

    finalJob.Wait();

    ASSERT_EQ(sum.load(std::memory_order_acquire), 1111);
}

TEST(JobSystem, DependencyChainExecutesInOrder)
{
    JobSystem jobSystem(2);

    std::vector<int> executionOrder;
    std::mutex orderMutex;

    auto job1 = jobSystem.Schedule([&]() {
        std::lock_guard lock(orderMutex);
        executionOrder.push_back(1);
        });

    auto job2 = jobSystem.Schedule([&]() {
        std::lock_guard lock(orderMutex);
        executionOrder.push_back(2);
        }, { job1 });

    auto job3 = jobSystem.Schedule([&]() {
        std::lock_guard lock(orderMutex);
        executionOrder.push_back(3);
        }, { job2 });

    job3.Wait();

    ASSERT_EQ(executionOrder.size(), 3);
    ASSERT_EQ(executionOrder[0], 1);
    ASSERT_EQ(executionOrder[1], 2);
    ASSERT_EQ(executionOrder[2], 3);
}

// ============================================================================
// Batch Scheduling Tests
// ============================================================================

TEST(JobSystem, ScheduleBatchExecutesAllTasks)
{
    JobSystem jobSystem(4);
    std::atomic<int> counter{ 0 };

    std::vector<std::function<void()>> tasks;
    for (int i = 0; i < 20; ++i) {
        tasks.push_back([&]() {
            counter.fetch_add(1, std::memory_order_relaxed);
            });
    }

    auto handles = jobSystem.ScheduleBatch(std::move(tasks));
    jobSystem.WaitAll(handles);

    ASSERT_EQ(counter.load(std::memory_order_acquire), 20);
}

TEST(JobSystem, ScheduleBatchReturnsCorrectNumberOfHandles)
{
    JobSystem jobSystem(2);

    std::vector<std::function<void()>> tasks;
    for (int i = 0; i < 5; ++i) {
        tasks.push_back([]() {});
    }

    auto handles = jobSystem.ScheduleBatch(std::move(tasks));

    ASSERT_EQ(handles.size(), 5);
}

// ============================================================================
// ParallelFor Tests
// ============================================================================

TEST(JobSystem, ParallelForProcessesAllIndices)
{
    JobSystem jobSystem(4);

    std::vector<int> data(100, 0);

    auto handle = jobSystem.ParallelFor(data.size(), [&](size_t i) {
        data[i] = static_cast<int>(i);
        });

    handle.Wait();

    for (size_t i = 0; i < data.size(); ++i) {
        ASSERT_EQ(data[i], i);
    }
}TEST(JobSystem, ParallelForHandlesEmptyRange)
{
    JobSystem jobSystem(2);

    auto handle = jobSystem.ParallelFor(0, [](size_t) {
        FAIL() << "Should not execute for empty range";
        });

    ASSERT_TRUE(handle.IsComplete());
}

TEST(JobSystem, ParallelForRespectsBatchSize)
{
    JobSystem jobSystem(4);

    std::atomic<int> batchCount{ 0 };
    std::mutex batchMutex;
    std::unordered_set<std::thread::id> threadIds;

    auto handle = jobSystem.ParallelFor(100, [&](size_t i) {
        std::lock_guard lock(batchMutex);
        threadIds.insert(std::this_thread::get_id());
        }, 25); // 100 items / 25 per batch = 4 batches

    handle.Wait();

    // With 4 worker threads and 4 batches, we should see up to 4 different threads
    ASSERT_LE(threadIds.size(), 4);
    ASSERT_GE(threadIds.size(), 1);
}

// ============================================================================
// Wait Tests
// ============================================================================

TEST(JobSystem, WaitBlocksUntilJobCompletes)
{
    JobSystem jobSystem(2);

    std::atomic<bool> jobStarted{ false };
    std::atomic<bool> jobFinished{ false };

    auto handle = jobSystem.Schedule([&]() {
        jobStarted.store(true, std::memory_order_release);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        jobFinished.store(true, std::memory_order_release);
        });

    // Wait for job to start
    while (!jobStarted.load(std::memory_order_acquire)) {
        std::this_thread::yield();
    }

    ASSERT_FALSE(jobFinished.load(std::memory_order_acquire));

    jobSystem.Wait(handle);

    ASSERT_TRUE(jobFinished.load(std::memory_order_acquire));
}

TEST(JobSystem, WaitAllBlocksUntilAllJobsComplete)
{
    JobSystem jobSystem(4);

    std::array<std::atomic<bool>, 5> jobsFinished;
    for (auto& finished : jobsFinished) {
        finished.store(false, std::memory_order_relaxed);
    }

    std::vector<JobHandle> handles;
    for (int i = 0; i < 5; ++i) {
        handles.push_back(jobSystem.Schedule([&, i]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            jobsFinished[i].store(true, std::memory_order_release);
            }));
    }

    jobSystem.WaitAll(handles);

    for (const auto& finished : jobsFinished) {
        ASSERT_TRUE(finished.load(std::memory_order_acquire));
    }
}

// ============================================================================
// Exception Safety Tests
// ============================================================================

TEST(JobSystem, HandlesExceptionInJob)
{
    JobSystem jobSystem(2);

    std::atomic<bool> otherJobExecuted{ false };

    auto throwingJob = jobSystem.Schedule([]() {
        throw std::runtime_error("Test exception");
        });

    auto normalJob = jobSystem.Schedule([&]() {
        otherJobExecuted.store(true, std::memory_order_release);
        });

    throwingJob.Wait();
    normalJob.Wait();

    // Job system should mark throwing job as complete and continue
    ASSERT_TRUE(throwingJob.IsComplete());
    ASSERT_TRUE(otherJobExecuted.load(std::memory_order_acquire));
}

// ============================================================================
// Shutdown Tests
// ============================================================================

TEST(JobSystem, CanShutdownSafely)
{
    JobSystem jobSystem(4);

    std::atomic<int> counter{ 0 };

    for (int i = 0; i < 10; ++i) {
        jobSystem.Schedule([&]() {
            counter.fetch_add(1, std::memory_order_relaxed);
            });
    }

    ASSERT_NO_THROW({
        jobSystem.Shutdown();
        });
}

TEST(JobSystem, ShutdownWaitsForPendingJobs)
{
    JobSystem jobSystem(2);

    std::atomic<int> completedJobs{ 0 }; for (int i = 0; i < 10; ++i) {
        jobSystem.Schedule([&]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            completedJobs.fetch_add(1, std::memory_order_relaxed);
            });
    }

    jobSystem.Shutdown();

    // All jobs should have completed before shutdown returned
    ASSERT_EQ(completedJobs.load(std::memory_order_acquire), 10);
}

TEST(JobSystem, RejectsNewJobsAfterShutdown)
{
    JobSystem jobSystem(2);

    jobSystem.Shutdown();

    std::atomic<bool> executed{ false };
    auto handle = jobSystem.Schedule([&]() {
        executed.store(true, std::memory_order_release);
        });

    // Job should be immediately marked complete (but not executed)
    ASSERT_TRUE(handle.IsComplete());
    ASSERT_FALSE(executed.load(std::memory_order_acquire));
}

// ============================================================================
// Statistics Tests
// ============================================================================

TEST(JobSystem, TracksStatistics)
{
    JobSystem jobSystem(2);

    auto initialStats = jobSystem.GetStats();
    ASSERT_EQ(initialStats.totalJobsScheduled, 0);
    ASSERT_EQ(initialStats.completedJobs, 0);

    std::vector<JobHandle> handles;
    for (int i = 0; i < 5; ++i) {
        handles.push_back(jobSystem.Schedule([]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }));
    }

    auto duringStats = jobSystem.GetStats();
    ASSERT_EQ(duringStats.totalJobsScheduled, 5);

    jobSystem.WaitAll(handles);

    auto finalStats = jobSystem.GetStats();
    ASSERT_EQ(finalStats.totalJobsScheduled, 5);
    ASSERT_EQ(finalStats.completedJobs, 5);
}

// ============================================================================
// Stress Tests
// ============================================================================

TEST(JobSystem, HandlesHighJobVolumeStress)
{
    JobSystem jobSystem(4);

    constexpr int NUM_JOBS = 1000;
    std::atomic<int> counter{ 0 };

    std::vector<JobHandle> handles;
    for (int i = 0; i < NUM_JOBS; ++i) {
        handles.push_back(jobSystem.Schedule([&]() {
            counter.fetch_add(1, std::memory_order_relaxed);
            }));
    }

    jobSystem.WaitAll(handles);

    ASSERT_EQ(counter.load(std::memory_order_acquire), NUM_JOBS);
}

TEST(JobSystem, HandlesComplexDependencyGraph)
{
    JobSystem jobSystem(8);

    // Create a diamond dependency pattern:
    //       root
    //      /    \
    //    mid1  mid2
    //      \    /
    //       final

    std::atomic<int> executionOrder{ 0 };
    std::array<int, 4> order;

    auto root = jobSystem.Schedule([&]() {
        order[0] = executionOrder.fetch_add(1, std::memory_order_relaxed);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        });

    auto mid1 = jobSystem.Schedule([&]() {
        order[1] = executionOrder.fetch_add(1, std::memory_order_relaxed);
        }, { root });

    auto mid2 = jobSystem.Schedule([&]() {
        order[2] = executionOrder.fetch_add(1, std::memory_order_relaxed);
        }, { root });

    auto final = jobSystem.Schedule([&]() {
        order[3] = executionOrder.fetch_add(1, std::memory_order_relaxed);
        }, { mid1, mid2 });

    final.Wait();

    // Root must execute first
    ASSERT_EQ(order[0], 0);
    // Mid jobs must execute after root
    ASSERT_GT(order[1], 0);
    ASSERT_GT(order[2], 0);
    // Final must execute last
    ASSERT_EQ(order[3], 3);
}

TEST(JobSystem, ConcurrentSchedulingAndWaiting)
{
    JobSystem jobSystem(4);

    std::atomic<int> totalCompleted{ 0 };
    std::barrier barrier{ 3 }; // 2 schedulers + 1 main

    auto scheduler1 = std::thread([&]() {
        barrier.arrive_and_wait(); for (int i = 0; i < 50; ++i) {
            auto handle = jobSystem.Schedule([&]() {
                totalCompleted.fetch_add(1, std::memory_order_relaxed);
                });
            if (i % 10 == 0) {
                handle.Wait();
            }
        }
        });

    auto scheduler2 = std::thread([&]() {
        barrier.arrive_and_wait();
        for (int i = 0; i < 50; ++i) {
            auto handle = jobSystem.Schedule([&]() {
                totalCompleted.fetch_add(1, std::memory_order_relaxed);
                });
            if (i % 10 == 0) {
                handle.Wait();
            }
        }
        });

    barrier.arrive_and_wait();

    scheduler1.join();
    scheduler2.join();

    // Wait a bit for any remaining jobs
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    ASSERT_EQ(totalCompleted.load(std::memory_order_acquire), 100);
}

// ============================================================================
// Realistic Usage Pattern Tests
// ============================================================================

TEST(JobSystem, SimulatesPhysicsAnimationPipeline)
{
    JobSystem jobSystem(4);

    struct Entity {
        std::atomic<float> position{ 0.0f };
        std::atomic<float> velocity{ 1.0f };
    };

    std::vector<Entity> entities(100);

    // Physics update
    auto physicsJob = jobSystem.ParallelFor(entities.size(), [&](size_t i) {
        float vel = entities[i].velocity.load(std::memory_order_relaxed);
        float pos = entities[i].position.load(std::memory_order_relaxed);
        entities[i].position.store(pos + vel * 0.016f, std::memory_order_relaxed);
        });

    // Animation depends on physics
    auto animJob = jobSystem.Schedule([&]() {
        // Process animations based on new positions
        for (auto& entity : entities) {
            entity.velocity.fetch_add(0.1f, std::memory_order_relaxed);
        }
        }, { physicsJob });

    animJob.Wait();

    // Verify all entities moved
    for (const auto& entity : entities) {
        ASSERT_GT(entity.position.load(std::memory_order_relaxed), 0.0f);
    }
}

TEST(JobSystem, SimulatesAssetLoadingPipeline)
{
    JobSystem jobSystem(8);

    std::atomic<int> assetsLoaded{ 0 };
    std::atomic<int> assetsProcessed{ 0 };

    // Load 10 assets in parallel
    std::vector<JobHandle> loadJobs;
    for (int i = 0; i < 10; ++i) {
        loadJobs.push_back(jobSystem.Schedule([&]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            assetsLoaded.fetch_add(1, std::memory_order_relaxed);
            }, {}, "Load Asset"));
    }

    // Process each loaded asset
    std::vector<JobHandle> processJobs;
    for (const auto& loadJob : loadJobs) {
        processJobs.push_back(jobSystem.Schedule([&]() {
            assetsProcessed.fetch_add(1, std::memory_order_relaxed);
            }, { loadJob }, "Process Asset"));
    }

    jobSystem.WaitAll(processJobs);

    ASSERT_EQ(assetsLoaded.load(std::memory_order_acquire), 10);
    ASSERT_EQ(assetsProcessed.load(std::memory_order_acquire), 10);
}