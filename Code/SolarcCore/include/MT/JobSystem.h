#pragma once
#include "Preprocessor/API.h"
#include "MT/Job.h"
#include "MT/JobHandle.h"
#include <vector>
#include <deque>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <functional>

class SOLARC_CORE_API JobSystem 
{
public:
    // Create job system with specified number of worker threads
    // If numThreads == 0, uses hardware_concurrency - 1 (leaving one core for main thread)
    explicit JobSystem(size_t numThreads = 0);

    ~JobSystem();

    // Non-copyable, non-movable
    JobSystem(const JobSystem&) = delete;
    JobSystem& operator=(const JobSystem&) = delete;
    JobSystem(JobSystem&&) = delete;
    JobSystem& operator=(JobSystem&&) = delete;

    // Schedule a job with optional dependencies
    // debugName is optional but useful for profiling
    JobHandle Schedule(std::function<void()> task,
        std::vector<JobHandle> dependencies = {},
        const char* debugName = nullptr);

    // Schedule multiple independent jobs at once (more efficient than calling Schedule repeatedly)
    std::vector<JobHandle> ScheduleBatch(std::vector<std::function<void()>> tasks,
        const char* debugName = nullptr);

    // Wait for a single job to complete
    void Wait(const JobHandle& handle);

    // Wait for multiple jobs to complete
    void WaitAll(const std::vector<JobHandle>& handles);

    // Parallel for: Execute a function for each index in range [0, count)
    // Automatically splits work across available threads
    JobHandle ParallelFor(size_t count,
        std::function<void(size_t index)> func,
        size_t batchSize = 64);

    // Get number of worker threads
    size_t GetWorkerCount() const { return m_Workers.size(); }

    // Check if there are any pending jobs
    bool HasPendingJobs() const;

    // Shutdown the job system (called automatically in destructor)
    void Shutdown();

    // Get statistics (useful for debugging/profiling)
    struct Stats 
    {
        size_t pendingJobs = 0;
        size_t completedJobs = 0;
        size_t totalJobsScheduled = 0;
    };
    Stats GetStats() const;

private:
    void WorkerThread(size_t threadIndex);
    bool TryExecuteJob();
    void ExecuteJob(Job& job);

    std::vector<std::thread> m_Workers;
    std::deque<Job> m_JobQueue;
    mutable std::mutex m_QueueMutex;
    std::condition_variable m_Condition;
    std::atomic<bool> m_Shutdown{ false };

    // Statistics
    std::atomic<size_t> m_TotalScheduled{ 0 };
    std::atomic<size_t> m_TotalCompleted{ 0 };
};