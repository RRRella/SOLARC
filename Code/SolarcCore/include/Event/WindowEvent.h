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
        RESIZED,
        MINIMIZED,
        RESTORED,
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
    explicit WindowCloseEvent()
        : WindowEvent(TYPE::CLOSE)
    {
    }
};


class WindowShownEvent : public WindowEvent
{
public:
    explicit WindowShownEvent()
        : WindowEvent(TYPE::SHOWN)
    {
    }
};


class WindowHiddenEvent : public WindowEvent
{
public:
    explicit WindowHiddenEvent()
        : WindowEvent(TYPE::HIDDEN)
    {
    }
};

class WindowResizeEvent : public WindowEvent
{
public:
    WindowResizeEvent(int32_t width, int32_t height)
        : WindowEvent(TYPE::RESIZED)
        , m_Width(width)
        , m_Height(height)
    {
    }

    int32_t GetWidth() const { return m_Width; }
    int32_t GetHeight() const { return m_Height; }

private:
    int32_t m_Width;
    int32_t m_Height;
};

class WindowMinimizedEvent : public WindowEvent
{
public:
    explicit WindowMinimizedEvent()
        : WindowEvent(TYPE::MINIMIZED)
    {
    }
};

class WindowRestoredEvent : public WindowEvent
{
public:
    explicit WindowRestoredEvent()
        : WindowEvent(TYPE::RESTORED)
    {
    }
};