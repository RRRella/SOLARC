#ifdef linux

#include "Window/Platform/Linux/WaylandWindowContextPlatformFactory.h"
#include "Window/Platform/Linux/WaylandWindowContextPlatform.h"
#include "Window/Platform/Linux/WaylandWindowPlatformFactory.h"

PlatformComponents WaylandWindowContextPlatformFactory::CreateComponents()
{
    PlatformComponents components;
    
    // Create context first
    auto context = std::make_unique<WaylandWindowContextPlatform>();
    auto* contextPtr = context.get();
    
    // Create window factory with pointer to context
    components.context = std::move(context);
    components.windowFactory = std::make_unique<WaylandWindowPlatformFactory>(contextPtr);
    
    return components;
}

#endif