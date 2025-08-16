#include "FreqUsedSymbolsOfTesting.h"
#include "EventFakes.h"

TEST(EventCommunication, IntegrationTestBetweenEventCellAndComponent)
{
	FakeEventComponent producerComponent;
	FakeEventComponent consumerComponent;
	std::string& dependentData = consumerComponent.GetData();

	FakeEventCell eventCell;

	eventCell.RegisterProducer(producerComponent);
	eventCell.RegisterConsumer(consumerComponent);
	producerComponent.TriggerEvent();
	ASSERT_EQ(dependentData, "Unaffected");
	consumerComponent.WaitToConsumeEvent();
	ASSERT_EQ(dependentData, "Affected");




	dependentData = "Unaffected";
	eventCell.UnRegisterProducer(producerComponent.GetEventCommunicationID());
	producerComponent.TriggerEvent();
	ASSERT_EQ(dependentData, "Unaffected");

	std::this_thread::sleep_for(std::chrono::seconds(2));

	consumerComponent.ConsumeEvent();
	ASSERT_EQ(dependentData, "Unaffected");




	eventCell.RegisterProducer(producerComponent);
	eventCell.UnRegisterConsumer(consumerComponent.GetEventCommunicationID());
	producerComponent.TriggerEvent();
	ASSERT_EQ(dependentData, "Unaffected");

	std::this_thread::sleep_for(std::chrono::seconds(2));

	consumerComponent.ConsumeEvent();
	ASSERT_EQ(dependentData, "Unaffected");



	EXPECT_THROW(eventCell.UnRegisterProducer(consumerComponent.GetEventCommunicationID()) , std::runtime_error);    //different component ID
	EXPECT_THROW(eventCell.UnRegisterConsumer(consumerComponent.GetEventCommunicationID()), std::runtime_error);	 //repeative unregister attempt
	EXPECT_THROW(eventCell.RegisterProducer(producerComponent), std::runtime_error);								 //repeative register attempt
}

