#include "Utility/CompileTimeUtil.h"
#include "WindowsWindow.h"

std::unique_ptr<WindowFactory> GetWindowFactory() noexcept
{
#ifdef _WIN64
    return std::make_unique<WindowsWindowFactory>();
#elif
    return std::make_unique<LinuxWindowFactory>();
#endif
}

void InitilizeCompileTimeCode()
{
#ifdef _WIN64
    WindowsWindow::RegisterWindowClass();
#endif
}
