#pragma once
#include "Preprocessor/API.h"
#include "Event/EventComponent.h"

struct SOLARC_CORE_API WindowsMetaData
{
	std::string name = "ENerf Window";
	uint32_t style = WS_OVERLAPPEDWINDOW;
	uint32_t extendedStyle = 0;
	int posX = 0;
	int posY = 0;
	int width = 1920;
	int height = 1080;
};

class SOLARC_CORE_API WindowPlatform
{
public:
	virtual bool PeekNextMessage() = 0;
	virtual void ProcessMessage() = 0;
	virtual void Update() = 0;

};

class SOLARC_CORE_API Window : public EventComponent
{
public:
	enum class CONSUMERS
	{
		NUM_CONSUMERS
	};

	void Update();

	WindowPlatform* GetMessagePipe() const { return m_Platform.get(); }
protected:


	Window(std::unique_ptr<WindowPlatform> platform)
		:EventComponent(static_cast<uint64_t>(CONSUMERS::NUM_CONSUMERS)), m_Platform(std::move(platform))
	{
	}

	std::unique_ptr<WindowPlatform> m_Platform;
};

class SOLARC_CORE_API WindowFactory
{
public:
	WindowFactory() = default;
	~WindowFactory() = default;

	virtual std::unique_ptr<Window> Create(const WindowsMetaData& metaData) const = 0;
private:
};