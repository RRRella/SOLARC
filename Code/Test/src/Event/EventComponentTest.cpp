#include "FreqUsedSymbolsOfTesting.h"
#include "Event/EventComponent.h"
#include "Event/EventMocks.h"

using ::testing::MockFunction;

class EventComponentTest : public ::testing::Test
{
protected:

	EventComponentTest()
	{
		eventComponent = std::make_unique<FakeEventComponent>();
	}

	~EventComponentTest()
	{

	}

	void SetUp() override
	{
		
	}

	void TearDown() override
	{

	}

	std::unique_ptr<EventComponent> eventComponent;
};

TEST_F(EventComponentTest, ReturnsItsEventHandlerCalLBacks)
{
	SharedEventCallBack callBack = eventComponent->GetConsumerCallBack(static_cast<size_t>(FakeEventComponent::CONSUMERS::ON_FAKE_EVENT));

	FakeEvent fakeEvent;

	auto safeCallBack = callBack.lock();
	
	assert(safeCallBack != nullptr, "call back is null");

	(*safeCallBack)(fakeEvent);

	FakeEventComponent* fakeEventComponent = (FakeEventComponent*)eventComponent.get();

	ASSERT_EQ(fakeEventComponent->GetLastCatchedEvent() , &fakeEvent);
}

TEST_F(EventComponentTest, CanGetProducerCallBacks)
{
	MockFunction<std::function<void(Event&)>> callBack;

	//default value for data is UINT64_MAX
	FakeEvent fakeEvent;

	EXPECT_CALL(callBack, Call(::testing::Ref(fakeEvent))).WillOnce(::testing::Invoke([](Event& e)->void
		{
			FakeEvent& fakeEvent = static_cast<FakeEvent&>(e);

			fakeEvent.Flip();
		}));

	auto sharedFunc = std::make_shared<std::function<void(Event&)>>(callBack.AsStdFunction());

	eventComponent->SetProducerCallBack(sharedFunc);

	FakeEventComponent* fakeEventComponent = (FakeEventComponent*)eventComponent.get();

	fakeEventComponent->TriggerAllProducerCallBacks(fakeEvent);

	ASSERT_EQ(fakeEvent.GetData(),0);
}