#include "Utility/CompileTimeUtil.h"

#ifdef _WIN64
#include "WindowsWindowPlatform.h"
#endif

std::unique_ptr<WindowPlatformFactory> GetWindowPlatformFactory() noexcept
{
#ifdef _WIN64
    return std::make_unique<WindowsWindowPlatformFactory>();
#elif
    return std::make_unique<LinuxWindowFactory>();
#endif
}
