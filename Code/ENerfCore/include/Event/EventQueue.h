#pragma once
#include "Preprocessor/API.h"

#include "Event/Event.h"

class ENERF_CORE_API EventQueue
{
public:
	EventQueue(EVENT_TYPE type) : m_Type(type){}
	~EventQueue() = default;

private:
	std::queue<Event*> m_Queue;
	EVENT_TYPE m_Type;
};