#ifdef linux

#include "Window/Platform/Linux/WaylandWindowPlatformFactory.h"
#include "Window/Platform/Linux/WaylandWindowPlatform.h"
#include "Window/Platform/Linux/WaylandWindowContextPlatform.h"

std::unique_ptr<WindowPlatform> WaylandWindowPlatformFactory::Create(
    const std::string& title,
    int32_t width,
    int32_t height)
{
    return std::make_unique<WaylandWindowPlatform>(title, width, height, m_Context);
}

#endif