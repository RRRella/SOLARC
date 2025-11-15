#pragma once
#include "Event/Event.h"

class WindowEvent : public Event
{
public:
    enum class TYPE
    {
        SHOWN,
        HIDDEN,
        CLOSE,
        GENERIC
    };

    WindowEvent(TYPE t)
        : Event(Event::TYPE::WINDOW_EVENT)
        , m_WindowEventType(t)
    {
    }

    TYPE GetWindowEventType() const { return m_WindowEventType; }

protected:

    TYPE m_WindowEventType;
};

class WindowCloseEvent : public WindowEvent
{
public:
    WindowCloseEvent() : WindowEvent(TYPE::CLOSE)
    {
    }
    
private:
};
