#include "MT/ThreadPool.h"

ThreadPool::ThreadPool(std::unique_ptr <TaskScheduler<void()>>&& scheduler, size_t num) :m_Scheduler(std::move(scheduler)), m_Num(num)
{
	if (m_Num == 0)
		throw std::runtime_error("Can't have zero threads for thread pool");

	m_Stalled.store(false, std::memory_order_relaxed);

	m_Threads.reserve(m_Num);

	for (size_t i = 0; i < m_Num; ++i)
	{
		m_Threads.emplace_back(&ThreadPool::ExecuteOnThread, this);
	}

	GenerateUUID(m_ID);

	std::lock_guard lock(m_GlobalSchedulersMtx);
	m_GlobalSchedulers.insert({ m_ID , m_Scheduler });
}

void ThreadPool::Execute(std::function<void()> task)
{
	{
		std::unique_lock lock(m_Mtx);
		if (!m_Shutdown)
			m_Scheduler->PushTask(task);
		else
			return;

		m_GlobalTaskPushedCount.fetch_add(1, std::memory_order_relaxed);
	}

	if (!m_Stalled.load(std::memory_order_relaxed))
		m_Cond.notify_one();
}

void ThreadPool::ExecuteOnThread()
{
	std::function<void()> task;

	while (!m_Shutdown)
	{
		{
			std::unique_lock lock(m_Mtx);

			m_Cond.wait(lock, [this]()->bool
				{
					return
						m_Shutdown
						||
						(!m_Stalled.load(std::memory_order_relaxed)
							&&
							(!m_Scheduler->IsEmpty() || m_GlobalTaskPushedCount.load(std::memory_order_relaxed) != 0));
				}
			);

			if (m_Shutdown && m_Scheduler->IsEmpty())
				return;

			task = m_Scheduler->GetNextTask();

			if (m_Shutdown)
				continue;
		}

		if (task)
		{
			task();
			m_GlobalTaskPushedCount.fetch_sub(1, std::memory_order_relaxed);
		}

		auto victimScheduler = PickVictimScheduler();

		//scheduler itself has to be thread safe
		//we didn't want to lock the victim with
		//it's thread pool mutex
		if (victimScheduler == nullptr)
			continue;
		else
			task = victimScheduler->GetNextTask();

		if (task)
		{
			task();
			m_GlobalTaskPushedCount.fetch_sub(1, std::memory_order_relaxed);
		}
	}
}

void ThreadPool::Shutdown()
{
	{
		std::lock_guard<std::mutex> lock(m_Mtx);
		m_Shutdown = true;
	}
	m_Cond.notify_all();

	for (auto& thread : m_Threads)
	{
		if (thread.joinable())
			thread.join();
	}

	m_Threads.clear();
}


decltype(auto) ThreadPool::PickRandomScheduler()
{
	static thread_local std::random_device rd;
	static thread_local std::mt19937 gen(rd());


	if (m_GlobalSchedulers.empty())
		return m_GlobalSchedulers.end();

	std::uniform_int_distribution<> dist(0, std::distance(m_GlobalSchedulers.begin(), m_GlobalSchedulers.end()) - 1);

	std::unordered_map<UUID, std::shared_ptr<TaskScheduler<void()>>>::iterator it = m_GlobalSchedulers.begin();
	std::advance(it, dist(gen));
	return it;
}

std::shared_ptr<TaskScheduler<void()>> ThreadPool::PickVictimScheduler()
{
	std::lock_guard lock(m_GlobalSchedulersMtx);

	auto victimSchedulerPair = PickRandomScheduler();

	if (victimSchedulerPair == m_GlobalSchedulers.end())
		return nullptr;


	if (victimSchedulerPair->second == m_Scheduler)
		if (victimSchedulerPair != m_GlobalSchedulers.begin())
			return (--victimSchedulerPair)->second;
		else
		{
			victimSchedulerPair++;
			if (victimSchedulerPair != m_GlobalSchedulers.end())
				return victimSchedulerPair->second;
			else
				return nullptr;
		}
	else
		return victimSchedulerPair->second;
}