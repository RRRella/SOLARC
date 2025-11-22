#pragma once
#include "WindowPlatform.h"
#include "Event/EventListener.h"
#include "Event/WindowEvent.h"
#include "Logging/LogMacros.h"
#include <memory>
#include <mutex>
#include <functional>

/**
 * Platform-agnostic window class
 *
 * Lifetime:
 * - Created by WindowContext::CreateWindow()
 * - Owned by shared_ptr (multiple owners possible)
 * - Destruction triggers callback to WindowContext for cleanup
 *
 * Thread Safety:
 * - Show/Hide/IsVisible: Thread-safe
 * - Update: Should be called from main thread only
 * - Destroy: Thread-safe, idempotent
 */
class SOLARC_CORE_API Window : public EventListener<WindowEvent>
{
    friend class WindowContext;

public:
    Window(
        std::shared_ptr<WindowPlatform> platform,
        Window* parent = nullptr,
        std::function<void(Window*)> onDestroy = nullptr
    );

    ~Window() override;

    /**
     * Manually destroy the window
     * note: Idempotent - safe to call multiple times
     * note: Thread-safe
     */
    void Destroy();

    /**
     * Show the window
     */
    void Show();

    /**
     * Hide the window
     */
    void Hide();

    /**
     * Check if window is visible
     * return true if visible, false otherwise
     */
    bool IsVisible() const;

    /**
     * Check if window is closed
     * return true if closed, false otherwise
     */
    bool IsClosed() const;

    /**
     * Get window title
     */
    const std::string& GetTitle() const { return m_Platform->GetTitle(); }

    /**
     * Get window width
     */
    const int32_t& GetWidth() const { return m_Platform->GetWidth(); }

    /**
     * Get window height
     */
    const int32_t& GetHeight() const { return m_Platform->GetHeight(); }

    /**
     * Get parent window
     * return Pointer to parent, or nullptr if no parent
     */
    Window* GetParent() const { return m_Parent; }

    /**
     * Process queued window events
     * note: Should be called from main thread
     */
    void Update();

protected:
    /**
     * Handle a window event
     * param e: Event to handle
     * note: Override to customize event handling
     */
    void OnEvent(const std::shared_ptr<const WindowEvent>& e) override;

private:
    std::shared_ptr<WindowPlatform> m_Platform;
    Window* m_Parent;
    std::function<void(Window*)> m_OnDestroy;
    bool m_Destroyed = false;
    mutable std::mutex m_DestroyMutex;
};