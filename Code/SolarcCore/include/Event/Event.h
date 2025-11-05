#pragma once
#include "Preprocessor/API.h"
#include "MT/ThreadSafeQueue.h"

enum class SOLARC_CORE_API TOP_LEVEL_EVENT_TYPE
{
	STUB_EVENT,
	WINDOW_EVENT
};

class SOLARC_CORE_API Event
{
public:
	Event(TOP_LEVEL_EVENT_TYPE type) :m_TopLevelEventType(type) {}
	~Event() = default;

	TOP_LEVEL_EVENT_TYPE GetTopLevelEventType() const { return m_TopLevelEventType; }

protected:
	TOP_LEVEL_EVENT_TYPE m_TopLevelEventType;
};

// concept expects an event type to have a member called GetTopLevelEventType()
template<typename T>
concept event_type = requires(T & t)
{
	{ t.GetTopLevelEventType() };
};

template<event_type EVENT_TYPE>
using EventQueue = ThreadSafeQueue<std::shared_ptr<const EVENT_TYPE>>;
