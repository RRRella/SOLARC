#include "WindowsWindowPlatform.h"

#ifdef WIN32

void WindowsWindowPlatform::PollEvents()
{
    MSG msg = { };

    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

bool WindowsWindowPlatform::Show()
{
    bool prevState = ShowWindow(m_Hwnd, SW_SHOW);

    if(!prevState)
        UpdateWindow(m_Hwnd);

    return prevState;
}

bool WindowsWindowPlatform::Hide()
{
    bool prevState = ShowWindow(m_Hwnd, SW_HIDE);
    return prevState;
}

HWND WindowsWindowPlatform::GetHWND()
{
    return m_Hwnd;
}


WindowsWindowPlatform::WindowsWindowPlatform(const WindowEventCallBack& callback ,const WindowMetaData& metaData)
    : WindowPlatform(callback)
{
    if (!m_WindowClass)
    {
        m_WindowClass = std::make_unique<WNDCLASS>();
        std::memset(m_WindowClass.get() , 0 , sizeof(WNDCLASS));

        HINSTANCE hInstance = GetModuleHandle(NULL);

        m_WindowClass->style = CS_HREDRAW | CS_VREDRAW;
        m_WindowClass->lpfnWndProc = WndProcSetup;
        m_WindowClass->hInstance = hInstance;
        m_WindowClass->hCursor = LoadCursor(NULL, IDC_ARROW);
        m_WindowClass->hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        m_WindowClass->lpszClassName = "SolarcWin32Class";

        ATOM atom = RegisterClass(m_WindowClass.get());
        assert(atom && "Failed to register window class");
    }

    m_WindowInstanceCount++;

    HINSTANCE hInstance = GetModuleHandle(NULL);

    m_Hwnd = CreateWindowEx(
        0,
        m_WindowClass->lpszClassName,
        metaData.name.c_str(),
        WS_OVERLAPPEDWINDOW,
        metaData.posX, metaData.posY, metaData.width, metaData.height,
        NULL,          
        NULL,       
        hInstance,  
        this        
    );

    assert(m_Hwnd, "Failed to create win32 window.");
}

WindowsWindowPlatform::~WindowsWindowPlatform()
{
}

std::unique_ptr<WindowPlatform> WindowsWindowPlatformFactory::Create(const WindowEventCallBack& callback, const WindowMetaData& metaData)
{
    return std::make_unique<WindowsWindowPlatform>(callback , metaData);
}

#endif