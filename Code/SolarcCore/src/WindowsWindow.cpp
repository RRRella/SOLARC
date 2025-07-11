#include "WindowsWindow.h"

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

class WindowsWindowInstinct : public WindowPlatform
{
public:
    WindowsWindowInstinct() = default;

    bool PeekNextMessage() override;
    void ProcessMessage() override;
    void Update() override;

    MSG GetMeesageData() const noexcept { return msg; }

    void SetHWND(HWND hwnd);

private:
    MSG msg = { };
    HWND m_Hwnd;
};

bool WindowsWindowInstinct::PeekNextMessage()
{   
    return PeekMessage(&msg, NULL, 0, 0, PM_REMOVE);
}

void WindowsWindowInstinct::ProcessMessage()
{
    TranslateMessage(&msg);
    DispatchMessage(&msg);
}

void WindowsWindowInstinct::Update()
{
    UpdateWindow(m_Hwnd);
}

void WindowsWindowInstinct::SetHWND(HWND hwnd)
{
    m_Hwnd = hwnd;
}

WindowsWindow::WindowsWindow(const WindowsMetaData& metaData)
    : Window(std::make_unique<WindowsWindowInstinct>())
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

WindowsWindow::~WindowsWindow()
{
	
}

void WindowsWindow::RegisterWindowClass() noexcept
{
    HINSTANCE hInstance = GetModuleHandle(NULL);

    m_WindowClass.lpfnWndProc = WindowProc;
    m_WindowClass.hInstance = hInstance;
    m_WindowClass.lpszClassName = "Solarc win32 class name";

    RegisterClass(&m_WindowClass);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    return 0;

    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

std::unique_ptr<Window> WindowsWindowFactory::Create(const WindowsMetaData& metaData) const
{
    return std::make_unique<WindowsWindow>(metaData);
}