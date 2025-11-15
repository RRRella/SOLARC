#pragma once
#include "Preprocessor/API.h"
#include "MT/ThreadSafeQueue.h"

class SOLARC_CORE_API Event
{
public:
	enum class TYPE
	{
		STUB_EVENT,
		WINDOW_EVENT,
		APPLICATION_EVENT
	};

	Event(TYPE type)
		: m_TopLevelEventType(type)
	{
	}
	~Event() = default;

	TYPE GetTopLevelEventType() const { return m_TopLevelEventType; }

protected:
	TYPE m_TopLevelEventType;
};

// concept expects an event type to have a member called GetTopLevelEventType()
template<typename T>
concept event_type = requires(T & t)
{
	{ t.GetTopLevelEventType() };
};

template<event_type EVENT_TYPE>
using EventQueue = ThreadSafeQueue<std::shared_ptr<const EVENT_TYPE>>;
