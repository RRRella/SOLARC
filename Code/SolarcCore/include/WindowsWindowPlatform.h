#pragma once

#ifdef WIN32

#include "Preprocessor/API.h"
#include <windows.h>
#include <windowsx.h>
#include "Window.h"

#ifndef UNICODE
#define UNICODE
#endif 

class WindowsWindowPlatform : public WindowPlatform
{
public:
    WindowsWindowPlatform(const WindowEventCallBack& callback , const WindowMetaData& metaData);
    ~WindowsWindowPlatform();

    void PollEvents() override;
    bool Show() override;
    bool Hide() override;

    HWND GetHWND();

private:

    static LRESULT CALLBACK WndProcSetup(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        if (uMsg == WM_NCCREATE)
        {
            auto cs = reinterpret_cast<CREATESTRUCT*>(lParam);
            auto window = static_cast<WindowsWindowPlatform*>(cs->lpCreateParams);

            // Store pointer to our instance
            SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));

            // Swap the WndProc
            SetWindowLongPtr(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&WindowsWindowPlatform::WndProc));

            return DefWindowProc(hwnd, uMsg, wParam, lParam);
        }

        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        auto window = reinterpret_cast<WindowsWindowPlatform*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

        if (window)
        {
            switch (uMsg)
            {

            case WM_SIZE:
            {
                int width = GET_X_LPARAM(lParam);
                int height = GET_Y_LPARAM(lParam);

                auto resizeEvent = std::make_shared<const WindowResizeEvent>(width, height);

                window->m_CallBack(std::static_pointer_cast<const WindowEvent>(resizeEvent));

                break;
            }

            case WM_DESTROY:
            {
                m_WindowInstanceCount--;
                if (m_WindowInstanceCount == 0)
                {
                    HINSTANCE hInstance = GetModuleHandle(NULL);

                    UnregisterClass("SolarcWin32Class", hInstance);
                }
            }


            }
        }

        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    HWND m_Hwnd;
    static inline std::unique_ptr<WNDCLASS> m_WindowClass = nullptr;
    static inline uint64_t m_WindowInstanceCount = 0;
};

class SOLARC_CORE_API WindowsWindowPlatformFactory : public WindowPlatformFactory
{
public:
    WindowsWindowPlatformFactory() = default;
    ~WindowsWindowPlatformFactory() = default;

    std::unique_ptr<WindowPlatform> Create(const WindowEventCallBack& callback, const WindowMetaData& metaData) override;
private:
};

#endif