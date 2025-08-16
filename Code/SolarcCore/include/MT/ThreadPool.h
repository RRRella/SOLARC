#include "Utility/UUID.h"

template<typename F>
class TaskScheduler;

template<typename R, typename... ARGS>
class TaskScheduler<R(ARGS...)>
{
public:
	virtual std::function<R(ARGS...)> GetNextTask() = 0;
	virtual void PushTask(std::function<R(ARGS...)> task) noexcept = 0;
	virtual bool IsEmpty() const = 0;

	virtual ~TaskScheduler() = default;
};

template<typename F>
class FcfsTaskScheduler;

template<typename R, typename... ARGS>
class FcfsTaskScheduler<R(ARGS...)> : public TaskScheduler<R(ARGS...)>
{
public:
	std::function<R(ARGS...)> GetNextTask() override;
	void PushTask(std::function<R(ARGS...)> task) noexcept override;

	bool IsEmpty() const override
	{
		std::lock_guard lock(m_Mtx);
		return m_TaskQueue.empty();
	}

private:

	std::queue<std::function<R(ARGS...)>> m_TaskQueue;
	mutable std::mutex m_Mtx;
};

template<typename R, typename... ARGS>
inline std::function<R(ARGS...)> FcfsTaskScheduler<R(ARGS...)>::GetNextTask()
{
	std::lock_guard lock(m_Mtx);

	if (m_TaskQueue.empty())
		return nullptr;

	auto item = m_TaskQueue.front();
	m_TaskQueue.pop();

	return item;
}

template<typename R, typename ...ARGS>
inline void FcfsTaskScheduler<R(ARGS...)>::PushTask(std::function<R(ARGS...)> task) noexcept
{
	std::lock_guard lock(m_Mtx);

	m_TaskQueue.push(task);
}

class SOLARC_CORE_API ThreadPool
{
public:
	ThreadPool(std::unique_ptr<TaskScheduler<void()>>&& scheduler, size_t num);
	~ThreadPool()
	{
		Shutdown();

		std::lock_guard lock(m_GlobalSchedulersMtx);
		m_GlobalSchedulers.erase(m_ID);
	}

	void Execute(std::function<void()> task);

	void Pause() noexcept
	{
		m_Stalled.store(true, std::memory_order_relaxed);
	}
	void Resume() noexcept
	{
		m_Stalled.store(false, std::memory_order_relaxed);
		m_Cond.notify_all();
	}

	void ForceWakeUp() noexcept
	{
		m_Cond.notify_all();
	}

	void Shutdown();

private:

	decltype(auto) PickRandomScheduler();
	std::shared_ptr<TaskScheduler<void()>> PickVictimScheduler();

	void ExecuteOnThread();

	std::vector<std::thread> m_Threads;
	mutable std::mutex m_Mtx;
	std::condition_variable m_Cond;
	bool m_Shutdown = false;

	std::shared_ptr<TaskScheduler<void()>> m_Scheduler;

	inline static std::atomic<uint64_t> m_GlobalTaskPushedCount = 0;
	inline static std::unordered_map<UUID, std::shared_ptr<TaskScheduler<void()>>> m_GlobalSchedulers;
	mutable std::mutex m_GlobalSchedulersMtx;

	size_t m_Num;

	UUID m_ID;

	std::atomic_bool m_Stalled;
};