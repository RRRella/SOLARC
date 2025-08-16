#pragma once
#include "FreqUsedSymbolsOfTesting.h"
#include "Event/Event.h"
#include "Event/EventCommunication.h"
#include "Utility/CompileTimeUtil.h"

//Fake Event
class FakeEvent : public Event
{
public:
	FakeEvent() : Event(EVENT_TYPE::FAKE_EVENT_TYPE) {}
	~FakeEvent() = default;
private:

};
//

//Fake Event Component
class FakeEventComponent : public EventComponent
{
public:
	FakeEventComponent() 
	{
		m_EventQueue = std::make_shared<ConsumerEventQueue>();
	};
	std::string& GetData() { return dataDependentOnEvent; }

	void TriggerEvent()
	{
		for (auto eventQueue : m_ProducerQueues)
		{
			auto eventQueuePtr = eventQueue.lock();

			if (eventQueuePtr)
			{
				eventQueuePtr->Dispatch(std::make_shared<FakeEvent>());
			}
		}
	}

	void WaitToConsumeEvent()
	{
		//We are not going to do anything special with the event itself
		auto event = m_EventQueue->WaitOnNext();

		dataDependentOnEvent = "Affected";
	}

	void ConsumeEvent()
	{
		//We are not going to do anything special with the event itself
		auto event = m_EventQueue->TryNext();
		if (!event)
			return;

		dataDependentOnEvent = "Affected";
	}

private:
	std::string dataDependentOnEvent = "Unaffected";
};
//

//Fake event cell
class FakeEventCell : public EventCell
{
public:
	FakeEventCell()
	{
	}

	void Communicate() override
	{
		auto event = m_ProducerReg.GetEvent();
		if (event)
			m_ConsumerReg.PushEvent(event);
	}


	void RegisterProducer(EventComponent& eventComponent)
	{
		m_ProducerReg.Register(eventComponent, this);
	}

	void RegisterConsumer(EventComponent& eventComponent)
	{
		m_ConsumerReg.Register(eventComponent);
	}

	void UnRegisterProducer(const UUID& eventComponentID)
	{
		m_ProducerReg.Unregister(eventComponentID);
	}
	
	void UnRegisterConsumer(const UUID& eventComponentID)
	{
		m_ConsumerReg.Unregister(eventComponentID);
	}

private:

	ProducerRegisterBlock m_ProducerReg;
	ConsumerRegisterBlock m_ConsumerReg;
};
//