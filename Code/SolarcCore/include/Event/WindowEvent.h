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

    WindowEvent(TYPE t, void* windowHandle = nullptr)
        : Event(Event::TYPE::WINDOW_EVENT)
        , m_WindowEventType(t)
        , m_WindowHandle(windowHandle)
    {
    }

    TYPE GetWindowEventType() const { return m_WindowEventType; }
    void* GetWindowHandle() const { return m_WindowHandle; }

protected:
    TYPE m_WindowEventType;
    void* m_WindowHandle;
};

class WindowCloseEvent : public WindowEvent
{
public:
    explicit WindowCloseEvent(void* windowHandle = nullptr)
        : WindowEvent(TYPE::CLOSE, windowHandle)
    {
    }
};