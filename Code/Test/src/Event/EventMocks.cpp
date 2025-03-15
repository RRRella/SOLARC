#include "EventMocks.h"

MockEventCell::MockEventCell() : EventCell(1, 1)
{
	m_Producers[static_cast<unsigned int>(MOCK_CELL_PRODUCER_ROLES::TYPE_A)] = std::make_shared<std::function<void(Event&)>>(
		[this](Event& e)
		{
			auto safeCallBack = m_Consumers[static_cast<unsigned int>(MOCK_CELL_CONSUMER_ROLES::TYPE_A)].lock();

			assert(safeCallBack != nullptr, "consumer is null");

			(*safeCallBack)(e);
		}
	);
}

FakeEventComponent::FakeEventComponent() : EventComponent(static_cast<unsigned int>(CONSUMERS::NUMBER_OF_CONSUMERS))
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
