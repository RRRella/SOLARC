#ifdef __linux__

#include "Window/Platform/Linux/WaylandWindowContextPlatformFactory.h"
#include "Window/Platform/Linux/WaylandWindowContextPlatform.h"
#include "Window/Platform/Linux/WaylandWindowPlatformFactory.h"

auto WaylandWindowContextPlatformFactory::Create() const -> Components
{
    auto context = std::make_unique<WaylandWindowContextPlatform>();
    auto* contextPtr = context.get(); // Safe: same scope
    auto windowFactory = std::make_unique<WaylandWindowPlatformFactory>(contextPtr);

    return Components{
        .context = std::move(context),
        .windowFactory = std::move(windowFactory)
    };
}

std::unique_ptr<WindowContextPlatformFactory> CreateWindowContextPlatformFactory()
{
    return std::make_unique<WaylandWindowContextPlatformFactory>();
}


#endif