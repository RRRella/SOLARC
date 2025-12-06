#pragma once
#ifdef _WIN32
#include "Window/WindowPlatformFactory.h"
#include "Preprocessor/API.h"

class WindowsWindowContextPlatform;

class SOLARC_CORE_API WindowsWindowPlatformFactory : public WindowPlatformFactory
{
public:
    explicit WindowsWindowPlatformFactory(WindowsWindowContextPlatform* context)
        : m_Context(context) {
    }

    std::unique_ptr<WindowPlatform> Create(
        const std::string& title,
        int32_t width,
        int32_t height) override;

private:
    WindowsWindowContextPlatform* m_Context;
};
#endif