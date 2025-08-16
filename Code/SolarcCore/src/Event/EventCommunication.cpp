#include "Event/EventCommunication.h"

std::shared_ptr<const Event> EventCell::ProducerRegisterBlock::GetEvent()
{
	std::lock_guard lock(m_Mtx);

	if (!m_EventQueue)
		return nullptr;

	return m_EventQueue->TryNext();
}


void EventCell::ProducerRegisterBlock::Register(EventComponent& eventComponent, EventCell* eventCell)
{
	std::lock_guard lock(m_Mtx);

	if (m_IsRegistered)
		throw std::runtime_error("Event Registration on already registered block");

	m_EventQueue = std::make_shared<ProducerEventQueue>(eventCell);

	eventComponent.RegisterProducerRole(std::static_pointer_cast<ProducerEventQueue>(m_EventQueue));

	m_EventComponentID = eventComponent.m_UUID;

	m_IsRegistered = true;
}

void EventCell::ProducerRegisterBlock::Unregister(const UUID& eventComponentID)
{
	std::lock_guard lock(m_Mtx);

	if (!m_IsRegistered)
		throw std::runtime_error("This block is already unregistered");
	else if (m_EventComponentID != eventComponentID)
		throw std::runtime_error("event component ID is different");

	m_EventQueue.reset();
	m_IsRegistered = false;
}


void EventCell::ConsumerRegisterBlock::PushEvent(std::shared_ptr<const Event> e)
{
	std::lock_guard lock(m_Mtx);
	if (m_IsRegistered)
	{
		auto queuePtr = m_EventQueue.lock();
		assert(queuePtr , "Registered event queue in now empty , probably we are deleting an it's component without unregistering.");
		queuePtr->Dispatch(e);
	}
}

void EventCell::ConsumerRegisterBlock::Register(EventComponent& eventComponent)
{
	std::lock_guard lock(m_Mtx);

	if (m_IsRegistered)
		throw std::runtime_error("Event Registration on already registered block");

	if (!eventComponent.m_EventQueue)
		throw std::runtime_error("Consumer event queue is empty. cannot register into event cell");

	m_EventQueue = eventComponent.m_EventQueue;

	m_EventComponentID = eventComponent.m_UUID;

	m_IsRegistered = true;
}

void EventCell::ConsumerRegisterBlock::Unregister(const UUID& eventComponentID)
{
	std::lock_guard lock(m_Mtx);

	if (!m_IsRegistered)
		throw std::runtime_error("This block is already unregistered");
	else if (m_EventComponentID != eventComponentID)
		throw std::runtime_error("Key registration is different");

	m_EventQueue.reset();
	m_IsRegistered = false;
}

UUID EventCell::EventRegisterBlock::GetEventComponentID() const
{
	std::lock_guard lock(m_Mtx);
	if (m_IsRegistered)
		return m_EventComponentID;
	else
		throw std::runtime_error("This block is unregistered , can't return event component ID");
}