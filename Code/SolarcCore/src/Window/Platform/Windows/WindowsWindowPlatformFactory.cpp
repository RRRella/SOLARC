#ifdef _WIN32
#include "Window/Platform/Windows/WindowsWindowPlatformFactory.h"
#include "Window/Platform/Windows/WindowsWindowPlatform.h"

std::unique_ptr<WindowPlatform> WindowsWindowPlatformFactory::Create(
    const std::string& title,
    int32_t width,
    int32_t height)
{
    return std::make_unique<WindowsWindowPlatform>(title, width, height, m_Context);
}
#endif