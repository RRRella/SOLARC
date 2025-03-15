#include "FreqUsedSymbolsOfTesting.h"
#include "Event/EventCell/EventCell.h"
#include "Event/EventQueue.h"
#include "Event/EventComponent.h"
#include "Event/EventMocks.h"

//Integration test with event component
TEST(AnEventCell , CanRegisterConsumerAndProducerRoles)
{
	std::unique_ptr<MockEventCell> eventCell = std::make_unique<MockEventCell>();

	FakeEventComponent consumerEventComponent;
	FakeEventComponent producerEventComponent;

	FakeEvent fakeEvent;

	eventCell->RegisterConsumerRole(EventConsumerRole{
											static_cast<unsigned int>(MOCK_CELL_CONSUMER_ROLES::TYPE_A) ,
											&consumerEventComponent,
											static_cast<unsigned int>(FakeEventComponent::CONSUMERS::ON_FAKE_EVENT)}
	);
	
	eventCell->RegisterProducerRole(EventProducerRole{ static_cast<unsigned int>(MOCK_CELL_PRODUCER_ROLES::TYPE_A) , &producerEventComponent });

	
	producerEventComponent.TriggerAllProducerCallBacks(fakeEvent);

	ASSERT_EQ(consumerEventComponent.GetLastCatchedEvent(), &fakeEvent);
}

TEST(AnEventCell , CanUnregiterConsumerAndProducerRoles)
{

}

