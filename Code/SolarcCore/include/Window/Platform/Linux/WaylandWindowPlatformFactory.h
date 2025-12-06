#pragma once

#ifdef __linux__

#include "Window/WindowPlatformFactory.h"
#include "Preprocessor/API.h"

class WaylandWindowContextPlatform;

class SOLARC_CORE_API WaylandWindowPlatformFactory : public WindowPlatformFactory
{
public:
    explicit WaylandWindowPlatformFactory(WaylandWindowContextPlatform* context)
        : m_Context(context)
    {
    }

    std::unique_ptr<WindowPlatform> Create(
        const std::string& title,
        int32_t width,
        int32_t height) override;

private:
    WaylandWindowContextPlatform* m_Context; // Safe: owned by same WindowContext
};

#endif