#include "FreqUsedSymbolsOfTesting.h"
#include "Event/EventCell/EventCell.h"
#include "Event/EventQueue.h"
#include "Event/EventComponent.h"
#include "Event/EventMocks.h"

class EventCellTest : public ::testing::Test
{
protected:

	EventCellTest() :eventCell(std::make_unique<FakeEventCell>()), 
		consumerEventComponent(std::make_unique<FakeEventComponent>()), 
		producerEventComponent(std::make_unique<FakeEventComponent>())
	{
	}

	~EventCellTest()
	{

	}

	void SetUp() override
	{
		catcherRegisterInfo = eventCell->RegisterCatcher(
			static_cast<uint64_t>(FAKE_EVENT_CELL_CATCHER_ROLES::TYPE_A),
			consumerEventComponent.get(),
			static_cast<uint64_t>(FakeEventComponent::CONSUMERS::ON_FAKE_EVENT)
		);

		dispatcherRegisterInfo = eventCell->RegisterDispatcher(static_cast<uint64_t>(FAKE_EVENT_CELL_DISPATCHER_ROLES::TYPE_A), producerEventComponent.get());
	}

	void TearDown() override
	{
		eventCell.reset();
		consumerEventComponent.reset();
		producerEventComponent.reset();
	}

	EventRegisterInfo dispatcherRegisterInfo;
	EventRegisterInfo catcherRegisterInfo;

	std::unique_ptr<FakeEventCell> eventCell = std::make_unique<FakeEventCell>();

	std::unique_ptr<FakeEventComponent> consumerEventComponent;
	std::unique_ptr<FakeEventComponent> producerEventComponent;
};

TEST_F(EventCellTest, CanRegisterCatcherAndDispatcherRoles)
{
	FakeEvent fakeEvent;

	ASSERT_EQ(dispatcherRegisterInfo.eventCellID, eventCell->GetID());
	ASSERT_EQ(dispatcherRegisterInfo.roleTypeInCell, EVENT_ROLE_TYPE::DISPATCHER);
	ASSERT_EQ(dispatcherRegisterInfo.eventComponentID, producerEventComponent->GetID());
	ASSERT_EQ(dispatcherRegisterInfo.roleIDInCell , static_cast<uint64_t>(FAKE_EVENT_CELL_DISPATCHER_ROLES::TYPE_A));

	ASSERT_EQ(catcherRegisterInfo.eventCellID, eventCell->GetID());	
	ASSERT_EQ(catcherRegisterInfo.roleTypeInCell, EVENT_ROLE_TYPE::CATCHER);
	ASSERT_EQ(catcherRegisterInfo.eventComponentID, consumerEventComponent->GetID());
	ASSERT_EQ(catcherRegisterInfo.roleIDInCell, static_cast<uint64_t>(FAKE_EVENT_CELL_CATCHER_ROLES::TYPE_A));
	
	producerEventComponent->TriggerAllProducerCallBacks(fakeEvent);

	ASSERT_EQ(consumerEventComponent->GetLastCatchedEvent(), &fakeEvent);
}

TEST_F(EventCellTest, CanUnregiterConsumerAndProducerRoles)
{
	eventCell->UnregisterDispatcher(dispatcherRegisterInfo.roleIDInCell);
	
	bool isDispatcherRegistered = eventCell->IsRegistered(dispatcherRegisterInfo.roleTypeInCell, dispatcherRegisterInfo.roleIDInCell);
	
	eventCell->UnregisterCatcher(catcherRegisterInfo.roleIDInCell);
	
	bool isCatcherRegistered = eventCell->IsRegistered(catcherRegisterInfo.roleTypeInCell, catcherRegisterInfo.roleIDInCell);
	
	ASSERT_EQ(isDispatcherRegistered, false);
	ASSERT_EQ(isCatcherRegistered, false);
}

