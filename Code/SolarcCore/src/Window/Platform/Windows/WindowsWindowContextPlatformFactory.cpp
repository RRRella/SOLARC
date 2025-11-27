#ifdef _WIN32

#include "Window/Platform/Windows/WindowsWindowContextPlatformFactory.h"
#include "Window/Platform/Windows/WindowsWindowContextPlatform.h"
#include "Window/Platform/Windows/WindowsWindowPlatformFactory.h"

PlatformComponents WindowsWindowContextPlatformFactory::CreateComponents()
{
    PlatformComponents components;
    components.context = std::make_unique<WindowsWindowContextPlatform>();
    components.windowFactory = std::make_unique<WindowsWindowPlatformFactory>();
    return components;
}

#endif