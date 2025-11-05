#pragma once
#include "Preprocessor/API.h"
#include "Event/Event.h"
#include "Event/EventProducer.h"
#include "WindowPlatform.h"

class WindowEvent : public Event
{
public:
	WindowEvent() :Event(TOP_LEVEL_EVENT_TYPE::WINDOW_EVENT) {}
	~WindowEvent() = default;

protected:

};

class SOLARC_CORE_API Window : public EventProducer<WindowEvent>
{
public:
	void Update();

	WindowPlatform* GetMessagePipe() const { return m_Platform.get(); }

protected:


	Window(std::unique_ptr<WindowPlatform> platform)
		: EventProducer<WindowEvent>(), m_Platform(std::move(platform))
	{
	}

	std::unique_ptr<WindowPlatform> m_Platform;
};