#pragma once
#include "Preprocessor/API.h"
#include "Event/EventCommunication.h"

class SOLARC_CORE_API WindowEvent : public Event
{
public:
	WindowEvent() : Event(EVENT_TYPE::WINDOW_EVENT_TYPE) {}
	virtual ~WindowEvent() = default;
protected:

private:

};

using WindowEventCallBack = std::function<void(std::shared_ptr<const WindowEvent> event)>;


class SOLARC_CORE_API WindowResizeEvent : public WindowEvent
{
public:
	WindowResizeEvent(uint16_t newWidth, uint16_t newHeight) 
		: WindowEvent(), m_Width(newWidth), m_Height(newHeight) {}
	virtual ~WindowResizeEvent() = default;

	uint16_t GetWidth() const { return m_Width; }
	uint16_t GetHeight() const { return m_Height; }

private:

	uint16_t m_Width;
	uint16_t m_Height;

};

struct SOLARC_CORE_API WindowMetaData
{
	std::string name = "SOLARC Window";
	int posX = 0;
	int posY = 0;
	int width = 1920;
	int height = 1080;
};

class SOLARC_CORE_API WindowPlatform
{
public:
	WindowPlatform(const WindowEventCallBack& callBack) : m_CallBack(callBack) {}
	virtual ~WindowPlatform() = default;
	
	virtual void PollEvents() = 0;
	virtual bool Show() = 0;
	virtual bool Hide() = 0;

protected:
	WindowEventCallBack m_CallBack;
};

class SOLARC_CORE_API WindowPlatformFactory
{
public:
	WindowPlatformFactory() = default;
	virtual ~WindowPlatformFactory() = default;

	virtual std::unique_ptr<WindowPlatform> Create(const WindowEventCallBack& callback, const WindowMetaData& metaData) = 0;

private:

};

class SOLARC_CORE_API Window : public EventComponent
{
public:
	
	Window(const WindowMetaData& metaData, std::shared_ptr<WindowPlatformFactory> platformFactory)
		:EventComponent()
	{
		m_Platform = platformFactory->Create(std::bind(&Window::CallBack, this, std::placeholders::_1), metaData);
	}

	void PollEvents() { m_Platform->PollEvents(); }
	bool Show() { return m_Platform->Show(); }
	bool Hide() { return m_Platform->Hide(); }

protected:
	std::unique_ptr<WindowPlatform> m_Platform;

	void CallBack(std::shared_ptr<const Event> event)
	{
		std::lock_guard lock(m_ProducerQueuesMtx);
		for (auto it = m_ProducerQueues.begin(); it != m_ProducerQueues.end(); )
		{
			auto eventQueuePtr = it->lock();
			if (eventQueuePtr)
			{
				eventQueuePtr->Dispatch(event);
				++it;
			}
			else
			{
				it = m_ProducerQueues.erase(it);
			}
		}
	}
};