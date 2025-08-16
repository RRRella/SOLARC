#include "FreqUsedSymbolsOfTesting.h"
#include "Event/Event.h"
#include "Event/EventFakes.h"


TEST(AEvent, ReturnsItsType)
{
	FakeEvent mockEvent;

	Event* event = &mockEvent;

	ASSERT_EQ(event->GetType(), EVENT_TYPE::FAKE_EVENT_TYPE);
}