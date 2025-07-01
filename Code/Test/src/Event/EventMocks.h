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
		NUM_CONSUMERS
	};

	FakeEventComponent();

	void TriggerAllProducerCallBacks(FakeEvent& fakeEvent);

	Event* GetLastCatchedEvent() const noexcept { return m_LastCatchedEvent; }

private:
	Event* m_LastCatchedEvent = nullptr;
};
//

//Mock event cell
enum class FAKE_EVENT_CELL_CATCHER_ROLES
{
	TYPE_A
};

enum class FAKE_EVENT_CELL_DISPATCHER_ROLES
{
	TYPE_A
};

class FakeEventCell : public EventCell
{
public:
	FakeEventCell();
	~FakeEventCell() = default;

	virtual void Communicate() override
	{

	}

private:
};
//