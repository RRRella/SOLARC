#pragma once
#include "WindowPlatform.h"
#include "Event/EventListener.h"
#include "Event/ObserverBus.h"
#include "Event/WindowEvent.h"
#include "Logging/LogMacros.h"
#include <memory>
#include <mutex>
#include <functional>
#include <type_traits>
#include <stdexcept>

// C++20 Concept: defines required interface for a window platform
template<typename T>
concept WindowPlatformConcept = requires(T & t) {
    // Basic properties
    { t.GetTitle() } -> std::same_as<const std::string&>;
    { t.GetWidth() } -> std::same_as<const int32_t&>;
    { t.GetHeight() } -> std::same_as<const int32_t&>;

    // Visibility
    { t.Show() } -> std::same_as<void>;
    { t.Hide() } -> std::same_as<void>;
    { t.IsVisible() } -> std::convertible_to<bool>;

    // Events (optional but expected for integration)
    // We assume it derives from EventProducer<WindowEvent> externally
};

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

template<WindowPlatformConcept PlatformT>
class WindowT : public EventListener<WindowEvent>
{

public:
    WindowT(
        std::unique_ptr<PlatformT> platform,
        std::function<void(WindowT<PlatformT>*)> onDestroy = nullptr
    );

    ~WindowT() override;

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
    PlatformT* GetPlatform() const { return m_Platform.get(); }

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
    std::unique_ptr<PlatformT> m_Platform;
    std::function<void(WindowT<PlatformT>*)> m_OnDestroy;
    bool m_Destroyed = false;
    mutable std::mutex m_DestroyMutex;

    ObserverBus<WindowEvent> m_Bus;
};

// Production alias
using Window = WindowT<class WindowPlatform>;

template<WindowPlatformConcept PlatformT>
WindowT<PlatformT>::WindowT(
    std::unique_ptr<PlatformT> platform,
    std::function<void(WindowT<PlatformT>*)> onDestroy)
    : m_Platform(std::move(platform))
    , m_OnDestroy(std::move(onDestroy))
{
    if (!m_Platform)
    {
        SOLARC_ERROR("Window: Platform cannot be null");
        throw std::invalid_argument("Window platform must not be null");
    }

    m_Bus.RegisterProducer(m_Platform.get());
    m_Bus.RegisterListener(this);

    SOLARC_WINDOW_TRACE("Window created: '{}'", m_Platform->GetTitle());
}

template<WindowPlatformConcept PlatformT>
WindowT<PlatformT>::~WindowT()
{
    SOLARC_WINDOW_TRACE("Window destructor: '{}'",
        m_Platform ? m_Platform->GetTitle() : "null");
    Destroy();
}

template<WindowPlatformConcept PlatformT>
void WindowT<PlatformT>::Destroy()
{
    std::lock_guard lock(m_DestroyMutex);
    if (m_Destroyed) return;

    m_Destroyed = true;

    SOLARC_WINDOW_INFO("Destroying window: '{}'",
        m_Platform ? m_Platform->GetTitle() : "null");

    if (m_OnDestroy)
    {
        m_OnDestroy(this);
    }
}

template<WindowPlatformConcept PlatformT>
void WindowT<PlatformT>::Show()
{
    std::lock_guard lock(m_DestroyMutex);
    if (m_Platform && !m_Destroyed)
    {
        m_Platform->Show();
        SOLARC_WINDOW_DEBUG("Window shown: '{}'", m_Platform->GetTitle());
    }
}

template<WindowPlatformConcept PlatformT>
void WindowT<PlatformT>::Hide()
{
    std::lock_guard lock(m_DestroyMutex);
    if (m_Platform && !m_Destroyed)
    {
        m_Platform->Hide();
        SOLARC_WINDOW_DEBUG("Window hidden: '{}'", m_Platform->GetTitle());
    }
}

template<WindowPlatformConcept PlatformT>
bool WindowT<PlatformT>::IsVisible() const
{
    std::lock_guard lock(m_DestroyMutex);
    return m_Platform && !m_Destroyed ? m_Platform->IsVisible() : false;
}

template<WindowPlatformConcept PlatformT>
bool WindowT<PlatformT>::IsClosed() const
{
    std::lock_guard lock(m_DestroyMutex);
    return m_Destroyed;
}

template<WindowPlatformConcept PlatformT>
void WindowT<PlatformT>::Update()
{
    // Process all queued events

    m_Bus.Communicate();

    ProcessEvents();
}

template<WindowPlatformConcept PlatformT>
void WindowT<PlatformT>::OnEvent(const std::shared_ptr<const WindowEvent>& e)
{
    switch (e->GetWindowEventType())
    {
    case WindowEvent::TYPE::CLOSE:
        SOLARC_WINDOW_INFO("Window close event received: '{}'", GetTitle());
        Destroy();
        break;

    case WindowEvent::TYPE::SHOWN:
        SOLARC_WINDOW_TRACE("Window shown event: '{}'", GetTitle());
        break;

    case WindowEvent::TYPE::HIDDEN:
        SOLARC_WINDOW_TRACE("Window hidden event: '{}'", GetTitle());
        break;

    default:
        SOLARC_WINDOW_TRACE("Window generic event: '{}'", GetTitle());
        break;
    }
}

// Explicit instantiation for production type
template class WindowT<WindowPlatform>;

