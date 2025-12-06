#ifdef _WIN32
#include "Window/Platform/Windows/WindowsWindowContextPlatformFactory.h"
#include "Window/Platform/Windows/WindowsWindowContextPlatform.h"
#include "Window/Platform/Windows/WindowsWindowPlatformFactory.h"

auto WindowsWindowContextPlatformFactory::Create() const -> Components
{
    auto context = std::make_unique<WindowsWindowContextPlatform>();
    auto windowFactory = std::make_unique<WindowsWindowPlatformFactory>(context.get());
    return Components{
        .context = std::move(context),
        .windowFactory = std::move(windowFactory)
    };
}

std::unique_ptr<WindowContextPlatformFactory> CreateWindowContextPlatformFactory()
{
    return std::make_unique<WindowsWindowContextPlatformFactory>();
}

#endif