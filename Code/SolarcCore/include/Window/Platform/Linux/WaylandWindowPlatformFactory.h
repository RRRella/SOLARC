#pragma once

#ifdef linux

#include "Window/WindowPlatformFactory.h"

class WaylandWindowContextPlatform;

class WaylandWindowPlatformFactory : public WindowPlatformFactory
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
    WaylandWindowContextPlatform* m_Context;
};

#endif