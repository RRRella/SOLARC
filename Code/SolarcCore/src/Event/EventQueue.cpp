#include "Event/EventQueue.h"
#include "Event/Event.h"
#include "SolarcApp.h"
#include "Window.h"
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
	{
		std::lock_guard lock(m_Mtx);
		m_Queue.push(e);
		m_CV.notify_one();
	}

	if (threadPool)
		threadPool->Execute(std::bind(&EventCell::Communicate, m_EventCell));
	else
		m_EventCell->Communicate();
}

void ConsumerEventQueue::Dispatch(std::shared_ptr<const Event> e)
{
	std::lock_guard lock(m_Mtx);
	m_Queue.push(e);
	m_CV.notify_one();
}