#pragma once
#include "Preprocessor/API.h"
#include "WindowPlatform.h"
#include <windows.h>
#include "Window.h"

#ifndef UNICODE
#define UNICODE
#endif 

struct SOLARC_CORE_API WindowsWindowMetaData
{
	std::string name = "ENerf Window";
	uint32_t style = WS_OVERLAPPEDWINDOW;
	uint32_t extendedStyle = 0;
	int posX = 0;
	int posY = 0;
	int width = 1920;
	int height = 1080;
};

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

class WindowsWindowPlatform : public WindowPlatform
{
public:
    WindowsWindowPlatform(const WindowsWindowMetaData& metaData)
    {
        HINSTANCE hInstance = GetModuleHandle(NULL);

        m_Hwnd = CreateWindowEx(
            metaData.extendedStyle,
            m_WindowClass.lpszClassName,
            metaData.name.c_str(),
            metaData.style,
            metaData.posX, metaData.posY, metaData.width, metaData.height,
            NULL,
            NULL,
            hInstance,
            NULL
        );

        ShowWindow(m_Hwnd, SW_SHOW);

        assert(m_Hwnd, "Failed to create win32 window.");
    }

    static void RegisterWindowClass() noexcept
    {
        HINSTANCE hInstance = GetModuleHandle(NULL);

        m_WindowClass.lpfnWndProc = WindowProc;
        m_WindowClass.hInstance = hInstance;
        m_WindowClass.lpszClassName = "Solarc win32 class name";

        RegisterClass(&m_WindowClass);
    }

    bool PeekNextMessage() override;
    void ProcessMessage() override;
    void Update() override;

    MSG GetMeesageData() const noexcept { return msg; }

    void SetHWND(HWND hwnd);

private:
    MSG msg = { };
    HWND m_Hwnd;
    static inline WNDCLASS m_WindowClass = {};
};
