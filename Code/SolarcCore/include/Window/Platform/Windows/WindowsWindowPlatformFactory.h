#pragma once

#ifdef _WIN32

#include "Window/WindowPlatformFactory.h"

class WindowsWindowPlatformFactory : public WindowPlatformFactory
{
public:
    WindowsWindowPlatformFactory() = default;

    std::unique_ptr<WindowPlatform> Create(
        const std::string& title,
        int32_t width,
        int32_t height) override;
};

#endif