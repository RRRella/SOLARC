#include "Event/EventQueue.h"
#include "Event/Event.h"
#include "SolarcApp.h"

std::shared_ptr<const Event> EventQueue::TryNext()
{
	std::lock_guard lock(m_Mtx);
	if (m_Queue.empty()) return nullptr;
	auto event = m_Queue.front();
	m_Queue.pop();
	return event;
}

std::shared_ptr<const Event> EventQueue::WaitOnNext()
{
	std::unique_lock lock(m_Mtx);
	m_CV.wait(lock, [this]()->bool { return !m_Queue.empty(); });
	auto event = m_Queue.front();
	m_Queue.pop();
	return event;
}

bool EventQueue::IsEmpty()
{
	std::lock_guard lock(m_Mtx);
	return m_Queue.empty();
}

void ProducerEventQueue::Dispatch(std::shared_ptr<const Event> e)
{
	std::lock_guard lock(m_Mtx);
	m_Queue.push(e);
	m_CV.notify_one();

	if (!threadPool)
		threadPool = std::make_unique<ThreadPool>(std::make_unique<FcfsTaskScheduler<void()>>(), SolarcApp::Get().GetThreadCountFor("Event Thread"));

	threadPool->Execute(std::bind(&EventCell::Communicate , m_EventCell));
}

void ConsumerEventQueue::Dispatch(std::shared_ptr<const Event> e)
{
	std::lock_guard lock(m_Mtx);
	m_Queue.push(e);
	m_CV.notify_one();
}