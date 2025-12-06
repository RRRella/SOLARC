#pragma once
#include "Window.h"
#include "WindowContextPlatform.h"
#include "Event/ObserverBus.h"
#include "Event/EventProducer.h"
#include "MT/ThreadChecker.h"
#include <unordered_set>
#include <memory>
#include <mutex>

#undef CreateWindow

/**
 * Manages window lifecycle and event distribution
 *
 * Responsibilities:
 * - Create and track windows
 * - Poll platform events and dispatch to windows
 * - Manage window cleanup
 *
 * Thread Safety:
 * - All methods must be called from main thread only
 */
class SOLARC_CORE_API WindowContext
{
public:
    // Singleton access
    static WindowContext& Get();

    WindowContext(const WindowContext&) = delete;
    WindowContext& operator=(const WindowContext&) = delete;

    WindowContext(WindowContext&&) = delete;
    WindowContext& operator=(WindowContext&&) = delete;

    /**
     * Create a window
     * template param T: Window type (must derive from Window)
     * template param Args: Constructor argument types
     * param title: Window title
     * param width: Window width
     * param height: Window height
     * param args: Additional constructor arguments for custom window types
     * return Shared pointer to created window
     * throws std::runtime_error if platform window creation fails
     * note: Must be called from main thread
     */
    template<typename T = Window, typename... Args>
        requires std::derived_from<T, Window>
    std::shared_ptr<T> CreateWindow(const std::string& title,
        const int32_t& width,
        const int32_t& height,
        Args&&... args)
    {
        SOLARC_WINDOW_INFO("Creating window: '{}' ({}x{})", title, width, height);

        // Create platform window
        auto platform = std::make_unique<WindowPlatform>(title, width, height);
        if (!platform)
        {
            SOLARC_WINDOW_ERROR("Failed to create window platform for: '{}'", title);
            throw std::runtime_error("Failed to create window platform");
        }

        // Set destruction callback
        auto onDestroy = [this](Window* w) { OnDestroyWindow(w); };

        auto window = std::make_shared<T>(
            std::move(platform),
            onDestroy,
            std::forward<Args>(args)...
        );

        // Track ownership
        {
            std::lock_guard lock(m_WindowsMutex);
            m_Windows.insert(std::static_pointer_cast<Window>(window));
        }

        SOLARC_WINDOW_INFO("Window created successfully: '{}'", title);
        return window;
    }

    /**
     * Poll platform events and distribute to windows
     * note: Must be called from main thread only
     */
    void PollEvents();

    /**
     * Shutdown window context and destroy all windows
     * note: Must be called from main thread, idempotent
     */
    void Shutdown();

    /**
     * Get number of active windows
     * return Window count
     */
    size_t GetWindowCount() const
    {
        std::lock_guard lock(m_WindowsMutex);
        return m_Windows.size();
    }

private:
    WindowContext();  // Private
    ~WindowContext(); // Private

    void OnDestroyWindow(Window* window);

    WindowContextPlatform& m_Platform = WindowContextPlatform::Get(); // Reference to singleton
    std::unordered_set<std::shared_ptr<Window>> m_Windows;
    mutable std::mutex m_WindowsMutex;
    ThreadChecker m_ThreadChecker;
    bool m_Shutdown = false;
};