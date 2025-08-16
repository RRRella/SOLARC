#pragma once
#include "Preprocessor/API.h"

enum class SOLARC_CORE_API EVENT_TYPE
{
	FAKE_EVENT_TYPE
};

class SOLARC_CORE_API Event
{
public:
	Event(EVENT_TYPE type) :m_Type(type) {}
	~Event() = default;

	EVENT_TYPE GetType() const { return m_Type; }

protected:
	EVENT_TYPE m_Type;
};
