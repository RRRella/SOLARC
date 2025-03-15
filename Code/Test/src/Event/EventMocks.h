#pragma once
#include "FreqUsedSymbolsOfTesting.h"
#include "Event/Event.h"
#include "Event/EventComponent.h"
#include "Event/EventCell/EventCell.h"
#include "Preprocessor/API.h"

//Fake Event
class FakeEvent: public Event
{
public:
	FakeEvent() : Event(EVENT_TYPE::MOCK_EVENT_TYPE) {}
	~FakeEvent() = default;

	void Flip() { data = ~data; }

	unsigned long long GetData() const { return data; }
private:
	unsigned long long data = UINT64_MAX;
};
//

//Fake Event Component
class FakeEventComponent : public EventComponent
{
public:
	enum class CONSUMERS
	{
		ON_FAKE_EVENT,
		NUMBER_OF_CONSUMERS
	};

	FakeEventComponent();

	void TriggerAllProducerCallBacks(FakeEvent& fakeEvent);

	Event* GetLastCatchedEvent() const noexcept { return m_LastCatchedEvent; }

private:
	Event* m_LastCatchedEvent = nullptr;
};
//

//Mock event cell
enum class MOCK_CELL_CONSUMER_ROLES
{
	TYPE_A
};

enum class MOCK_CELL_PRODUCER_ROLES
{
	TYPE_A
};

class MockEventCell : public EventCell
{
public:
	MockEventCell();
	~MockEventCell() = default;

	MOCK_METHOD(void, Communicate, (), (override));

private:
};
//