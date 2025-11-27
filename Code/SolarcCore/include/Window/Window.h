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
 * - All methods must be called from main thread only
 * - Destroy: Idempotent - safe to call multiple times
 */
class SOLARC_CORE_API Window : public EventListener<WindowEvent>
{
    friend class WindowContext;

public:
    Window(
        std::unique_ptr<WindowPlatform> platform,
        std::function<void(Window*)> onDestroy = nullptr
    );

    ~Window() override;

    /**
     * Manually destroy the window
     * note: Idempotent - safe to call multiple times
     * note: Must be called from main thread
     */
    void Destroy();

    /**
     * Show the window
     * note: Must be called from main thread
     */
    void Show();

    /**
     * Hide the window
     * note: Must be called from main thread
     */
    void Hide();

    /**
     * Check if window is visible
     * return true if visible, false otherwise
     * note: Must be called from main thread
     */
    bool IsVisible() const;

    /**
     * Check if window is closed
     * return true if closed, false otherwise
     * note: Must be called from main thread
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
     * Get platform handle (for internal use by WindowContext)
     * return Raw pointer to platform (never null while window is alive)
     */
    WindowPlatform* GetPlatform() const { return m_Platform.get(); }

    /**
     * Process queued window events
     * note: Must be called from main thread
     */
    void Update();

protected:
    /**
     * Handle a window event
     * param e: Event to handle
     * note: Override to customize event handling
     * note: Automatically filters events by window handle
     */
    void OnEvent(const std::shared_ptr<const WindowEvent>& e) override;

private:
    std::unique_ptr<WindowPlatform> m_Platform;
    std::function<void(Window*)> m_OnDestroy;
    bool m_Destroyed = false;
    mutable std::mutex m_DestroyMutex;
};