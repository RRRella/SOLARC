#include "FreqUsedSymbolsOfTesting.h"
#include "Event/Event.h"
#include "Event/EventMocks.h"


TEST(AEvent, ReturnsItsType)
{
	FakeEvent mockEvent;

	Event* event = &mockEvent;

	ASSERT_EQ(event->GetType(), EVENT_TYPE::MOCK_EVENT_TYPE);
}