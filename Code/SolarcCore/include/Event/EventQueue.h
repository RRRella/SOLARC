#pragma once
#include "Preprocessor/API.h"
#include "Event/Event.h"
#include "MT/ThreadPool.h"
class EventCell;

class SOLARC_CORE_API EventQueue
{
public:
	EventQueue() = default;
	virtual ~EventQueue() = default;

	virtual void Dispatch(std::shared_ptr<const Event>) = 0;
	std::shared_ptr<const Event> TryNext();
	std::shared_ptr<const Event> WaitOnNext();
	bool IsEmpty();

protected:
	std::queue<std::shared_ptr<const Event>> m_Queue;
	std::mutex m_Mtx;
	std::condition_variable m_CV;
};

class SOLARC_CORE_API ProducerEventQueue : public EventQueue
{
public:
	ProducerEventQueue(EventCell* eventCell) : m_EventCell(eventCell) {}
	~ProducerEventQueue() = default;
	void Dispatch(std::shared_ptr<const Event> e) override;
private:
	inline static std::unique_ptr<ThreadPool> threadPool = nullptr;
	EventCell* m_EventCell;
};

class SOLARC_CORE_API ConsumerEventQueue : public EventQueue
{
public:
	ConsumerEventQueue() = default;
	~ConsumerEventQueue() = default;
	void Dispatch(std::shared_ptr<const Event> e) override;
};