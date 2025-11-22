#include "MT/JobSystem.h"
#include <algorithm>
#include <cassert>

JobSystem::JobSystem(size_t numThreads) 
{
    if (numThreads == 0) {
        // Default: use all cores except one (leave one for main thread)
        numThreads = std::max(1u, std::thread::hardware_concurrency() - 1);
    }

    m_Workers.reserve(numThreads);
    for (size_t i = 0; i < numThreads; ++i) {
        m_Workers.emplace_back(&JobSystem::WorkerThread, this, i);
    }
}

JobSystem::~JobSystem() 
{
    Shutdown();
}

JobHandle JobSystem::Schedule(std::function<void()> task,
    std::vector<JobHandle> dependencies,
    const char* debugName) 
{
    assert(task && "Cannot schedule null task");

    auto completionFlag = std::make_shared<std::atomic<bool>>(false);

    Job job(std::move(task), completionFlag, std::move(dependencies), debugName);

    {
        std::lock_guard lock(m_QueueMutex);

        if (m_Shutdown.load(std::memory_order_relaxed)) {
            // System is shutting down, don't accept new jobs
            // Return an already-completed handle
            completionFlag->store(true, std::memory_order_release);
            return JobHandle(completionFlag);
        }

        m_JobQueue.push_back(std::move(job));
        m_TotalScheduled.fetch_add(1, std::memory_order_relaxed);
    }

    m_Condition.notify_one();

    return JobHandle(completionFlag);
}

std::vector<JobHandle> JobSystem::ScheduleBatch(std::vector<std::function<void()>> tasks,
    const char* debugName) 
{
    std::vector<JobHandle> handles;
    handles.reserve(tasks.size());

    {
        std::lock_guard lock(m_QueueMutex);

        if (m_Shutdown.load(std::memory_order_relaxed)) {
            // Return empty completed handles
            for (size_t i = 0; i < tasks.size(); ++i) {
                auto flag = std::make_shared<std::atomic<bool>>(true);
                handles.emplace_back(flag);
            }
            return handles;
        }

        for (auto& task : tasks) {
            auto completionFlag = std::make_shared<std::atomic<bool>>(false);
            handles.emplace_back(completionFlag);

            Job job(std::move(task), completionFlag, {}, debugName);
            m_JobQueue.push_back(std::move(job));
        }

        m_TotalScheduled.fetch_add(tasks.size(), std::memory_order_relaxed);
    }

    // Wake up multiple threads if we added many jobs
    if (tasks.size() > 1) {
        m_Condition.notify_all();
    }
    else {
        m_Condition.notify_one();
    }

    return handles;
}

JobHandle JobSystem::ParallelFor(size_t count,
    std::function<void(size_t)> func,
    size_t batchSize) 
{
    if (count == 0) {
        auto flag = std::make_shared<std::atomic<bool>>(true);
        return JobHandle(flag);
    }

    // Calculate number of batches
    const size_t numBatches = (count + batchSize - 1) / batchSize;

    // Create a counter for completed batches
    auto batchCounter = std::make_shared<std::atomic<size_t>>(0);
    auto completionFlag = std::make_shared<std::atomic<bool>>(false);

    // Schedule batch jobs
    {
        std::lock_guard lock(m_QueueMutex);

        for (size_t batch = 0; batch < numBatches; ++batch) {
            const size_t startIdx = batch * batchSize;
            const size_t endIdx = std::min(startIdx + batchSize, count);

            auto batchTask = [func, startIdx, endIdx, batchCounter, completionFlag, numBatches]() {
                // Execute this batch
                for (size_t i = startIdx; i < endIdx; ++i) {
                    func(i);
                }

                // Mark this batch as complete
                size_t completed = batchCounter->fetch_add(1, std::memory_order_acq_rel) + 1;

                // If this was the last batch, mark the whole parallel-for as complete
                if (completed == numBatches) {
                    completionFlag->store(true, std::memory_order_release);
                }
                };

            // We use a dummy completion flag for individual batches
            auto dummyFlag = std::make_shared<std::atomic<bool>>(false);
            Job job(std::move(batchTask), dummyFlag, {}, "ParallelFor Batch");
            m_JobQueue.push_back(std::move(job));
        }

        m_TotalScheduled.fetch_add(numBatches, std::memory_order_relaxed);
    }

    m_Condition.notify_all();

    return JobHandle(completionFlag);
}

void JobSystem::Wait(const JobHandle& handle) 
{
    handle.Wait();
}

void JobSystem::WaitAll(const std::vector<JobHandle>& handles) 
{
    for (const auto& handle : handles) {
        handle.Wait();
    }
}

bool JobSystem::HasPendingJobs() const 
{
    std::lock_guard lock(m_QueueMutex);
    return !m_JobQueue.empty();
}

JobSystem::Stats JobSystem::GetStats() const
{
    Stats stats;

    {
        std::lock_guard lock(m_QueueMutex);
        stats.pendingJobs = m_JobQueue.size();
    }

    stats.totalJobsScheduled = m_TotalScheduled.load(std::memory_order_relaxed);
    stats.completedJobs = m_TotalCompleted.load(std::memory_order_relaxed);

    return stats;
}

void JobSystem::Shutdown() 
{
    // Signal shutdown
    {
        std::lock_guard lock(m_QueueMutex);
        m_Shutdown.store(true, std::memory_order_relaxed);
    }

    // Wake up all threads
    m_Condition.notify_all();

    // Wait for all threads to finish
    for (auto& worker : m_Workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }

    m_Workers.clear();
}

void JobSystem::WorkerThread(size_t threadIndex) 
{
    while (true) {
        bool executedJob = TryExecuteJob();

        if (!executedJob) {
            // No job was ready, wait for work or shutdown signal
            std::unique_lock lock(m_QueueMutex);

            m_Condition.wait(lock, [this]() {
                return m_Shutdown.load(std::memory_order_relaxed)
                    || !m_JobQueue.empty();
                });

            // Check if we should exit
            if (m_Shutdown.load(std::memory_order_relaxed) && m_JobQueue.empty()) {
                return;
            }
        }
    }
}

bool JobSystem::TryExecuteJob() {
    Job job;

    {
        std::lock_guard lock(m_QueueMutex);

        // Find a job whose dependencies are satisfied
        for (auto it = m_JobQueue.begin(); it != m_JobQueue.end(); ++it) {
            if (it->AreDependenciesSatisfied()) {
                job = std::move(*it);
                m_JobQueue.erase(it);
                break;
            }
        }

        // No ready jobs found
        if (!job.IsValid()) {
            return false;
        }
    }

    // Execute the job outside the lock
    ExecuteJob(job);

    return true;
}

void JobSystem::ExecuteJob(Job& job) 
{
    // Execute the task
    try 
    {
        job.task();
    }
    catch (const std::exception& e) {
        // Log error but don't crash
        // In production, you'd use your logger here
        // spdlog::error("Job '{}' threw exception: {}", 
        //               job.debugName ? job.debugName : "unnamed", 
        //               e.what());
    }
    catch (...) 
    {
    // spdlog::error("Job '{}' threw unknown exception",
    //               job.debugName ? job.debugName : "unnamed");
    }

    // Mark as complete
    job.completionFlag->store(true, std::memory_order_release);
    m_TotalCompleted.fetch_add(1, std::memory_order_relaxed);
}