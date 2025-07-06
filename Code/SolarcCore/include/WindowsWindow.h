#pragma once
#include "Preprocessor/API.h"
#include <windows.h>
#include "Window.h"

#ifndef UNICODE
#define UNICODE
#endif 

class SOLARC_CORE_API WindowsWindow : public Window
{
public:
	WindowsWindow(const WindowsMetaData& metaData);
	~WindowsWindow();

    static void RegisterWindowClass() noexcept;
private:

    HWND m_Hwnd;

    //For now window class is going to get defined the same for every window instance
    //Will refactor later
    static inline WNDCLASS m_WindowClass = {};
};

class SOLARC_CORE_API WindowsWindowFactory : public WindowFactory
{
public:
    WindowsWindowFactory() = default;
    ~WindowsWindowFactory() = default;

    std::unique_ptr<Window> Create(const WindowsMetaData& metaData) const override;
private:
};