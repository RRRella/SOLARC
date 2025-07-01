#include "EventMocks.h"

FakeEventCell::FakeEventCell() : EventCell(1, 1)
{
	//We are implementing this dispatcher with assertions because it's going to be used in test only

	m_Dispatchers[static_cast<unsigned int>(FAKE_EVENT_CELL_DISPATCHER_ROLES::TYPE_A)] = std::function<void(Event&)>(
		[this](Event& e)
		{
			auto catcher = m_RegisteredCatchers[static_cast<unsigned int>(FAKE_EVENT_CELL_CATCHER_ROLES::TYPE_A)];

			if (!catcher)
				assert(false, "catcher block is null");

			if (!catcher->isRegistered)
				assert(false, "catcher block is not registered");

			auto safeCallBack = catcher->callBack.lock();

			assert(safeCallBack != nullptr, "catcher callback is null");

			(*safeCallBack)(e);
		}
	);
}

FakeEventComponent::FakeEventComponent() : EventComponent(static_cast<uint64_t>(CONSUMERS::NUM_CONSUMERS))
{
	m_Consumers[static_cast<unsigned int>(CONSUMERS::ON_FAKE_EVENT)]
		=
		std::make_shared<std::function<void(Event&)>>(

			[this](Event& e)->void
			{
				m_LastCatchedEvent = &e;
			}

	);
}

void FakeEventComponent::TriggerAllProducerCallBacks(FakeEvent& fakeEvent)
{
	for (auto producer = m_Producers.begin(); producer != m_Producers.end(); ++producer)
	{
		auto safeProducer = (*producer).lock();

		(*safeProducer)(fakeEvent);
	}
}
