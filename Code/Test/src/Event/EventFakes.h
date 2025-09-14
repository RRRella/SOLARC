#pragma once
#include "FreqUsedSymbolsOfTesting.h"
#include "Event/Event.h"
#include "Window.h"
#include "Event/EventCommunication.h"
#include "Utility/CompileTimeUtil.h"

//Fake Event
class FakeEvent : public Event
{
public:
	FakeEvent(const std::string& eventMessage) : Event(EVENT_TYPE::FAKE_EVENT_TYPE), m_Message(eventMessage){}
	~FakeEvent() = default;

	std::string GetEventMessage() const { return m_Message; }
private:
	std::string m_Message;

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

	void TriggerEvent(const std::string& eventMessage)
	{
		for (auto eventQueue : m_ProducerQueues)
		{
			auto eventQueuePtr = eventQueue.lock();

			if (eventQueuePtr)
			{
				eventQueuePtr->Dispatch(std::make_shared<FakeEvent>(eventMessage));
			}
		}
	}

	void WaitToConsumeEvent()
	{
		//We are not going to do anything special with the event itself
		auto event = m_EventQueue->WaitOnNext();

		dataDependentOnEvent = std::static_pointer_cast<const FakeEvent>(event)->GetEventMessage();
	}

	void ConsumeEvent()
	{
		//We are not going to do anything special with the event itself
		auto event = m_EventQueue->TryNext();
		if (!event)
			return;

		dataDependentOnEvent = std::static_pointer_cast<const FakeEvent>(event)->GetEventMessage();
	}

	std::shared_ptr<const Event> ReturnEvent()
	{
		return m_EventQueue->TryNext();
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
		const WindowResizeEvent* ee = (const WindowResizeEvent*)event.get();
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