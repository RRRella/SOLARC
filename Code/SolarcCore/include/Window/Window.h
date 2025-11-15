#pragma once
#include "WindowPlatform.h"
#include "Event/EventListener.h"
#include "Event/WindowEvent.h"

class Window : public EventListener<WindowEvent>
{
    friend class WindowContext; // Allows WindowContext to set m_OnDestroy

public:
    Window(
        std::shared_ptr<WindowPlatform> platform,
        Window* parent = nullptr,
        std::function<void(Window*)> onDestroy = nullptr
    );
    ~Window() override;

    void Destroy(); // Safe manual destruction

    void Show();
    void Hide();
    bool IsVisible() const;

    const std::string& GetTitle()  const { return m_Platform->GetTitle(); }
    const int32_t& GetWidth()  const { return m_Platform->GetWidth(); }
    const int32_t& GetHeight() const { return m_Platform->GetHeight(); }

    Window* GetParent() const { return m_Parent; }

    // Process events from the queue
    void Update();

protected:
    void OnEvent(const std::shared_ptr<const WindowEvent>& e) override;

private:
    std::shared_ptr<WindowPlatform> m_Platform;
    Window* m_Parent;
    std::function<void(Window*)> m_OnDestroy;
    bool m_Destroyed = false;
    mutable std::mutex m_DestroyMutex;
};