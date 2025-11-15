#pragma once
#include "Window.h"
#include "WindowContextPlatform.h"
#include "Event/ObserverBus.h"
#include "Event/EventProducer.h"


class WindowContext : public EventProducer<WindowEvent>
{
public:
    explicit WindowContext(std::unique_ptr<WindowContextPlatform> platform);
    ~WindowContext() override;

    // Custom window creation (for testing or specialization)
    template<typename T = Window, typename... Args>
    std::shared_ptr<T> CreateWindow(const std::string& title, const int32_t& width, const int32_t& height , Args&&... args) requires std::derived_from<T, Window>
    {
        auto platform = m_Platform->CreateWindowPlatform(title, width, height);
        if (!platform)
            throw std::runtime_error("Failed to create window platform");

        auto window = std::make_shared<T>(platform , std::forward<Args>(args)...);
        window->m_Platform = std::move(platform); // Inject platform

        // Set destruction callback
        auto onDestroy = [this](Window* w) { OnDestroyWindow(w); };
        window->m_OnDestroy = onDestroy;

        // Register as event listener
        m_Bus.RegisterListener(window.get());

        // Track ownership
        {
            std::lock_guard lock(m_WindowsMutex);
            m_Windows.insert(std::static_pointer_cast<Window>(window));
        }

        return window;
    }

    void PollEvents();
    void Shutdown();

private:
    void OnDestroyWindow(Window* window);

    std::unique_ptr<WindowContextPlatform> m_Platform;
    ObserverBus<WindowEvent> m_Bus;
    std::unordered_set<std::shared_ptr<Window>> m_Windows;
    mutable std::mutex m_WindowsMutex;
};