#include "FreqUsedSymbolsOfTesting.h"
#include "Event/Event.h"
#include "Event/EventFakes.h"


TEST(AEvent, ReturnsItsType)
{
	FakeEvent fakeEvent("");

	Event* event = &fakeEvent;

	ASSERT_EQ(event->GetType(), EVENT_TYPE::FAKE_EVENT_TYPE);
}