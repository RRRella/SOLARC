#pragma once
#include "WindowPlatform.h"
#include "Event/EventListener.h"
#include "Event/EventProducer.h"
#include "Event/ObserverBus.h"
#include "Event/WindowEvent.h"
#include "Logging/LogMacros.h"
#include "Preprocessor/API.h"
#include <memory>
#include <mutex>
#include <functional>
#include <type_traits>
#include <stdexcept>

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
    #ifdef _Win32
    { t.IsMinimized() } -> std::convertible_to<bool>;
    #endif

    // Window State Commands
    { t.Resize(0, 0) } -> std::same_as<void>;
    { t.Minimize() } -> std::same_as<void>;
    { t.Maximize() } -> std::same_as<void>;
    { t.Restore() } -> std::same_as<void>;

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
class WindowT : public EventListener<WindowEvent> , public EventProducer<WindowEvent>
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
    * Resize the window
    * note: Must be called from main thread
    */
    void Resize(int32_t width, int32_t height);

    /**
     * Minimize the window
     * note: Must be called from main thread
     */
    void Minimize();

    /**
     * Maximize the window
     * note: Must be called from main thread
     */
    void Maximize();

    /**
     * Restore the window (from minimized/maximized state)
     * note: Must be called from main thread
     */
    void Restore();

    /**
     * Check if window is visible
     * return true if visible, false otherwise
     * note: Must be called from main thread
     */
    bool IsVisible() const;

    /**
    * Check if window is minimized
    * return true if minimized, false otherwise
    * note: Must be called from main thread
    */
    bool IsMinimized() const;

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
    const PlatformT* GetPlatform() const { return m_Platform.get(); }

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

extern template class WindowT<WindowPlatform>;

// Production alias
using Window = WindowT<WindowPlatform>;

template<WindowPlatformConcept PlatformT>
inline WindowT<PlatformT>::WindowT(
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
inline WindowT<PlatformT>::~WindowT()
{
    SOLARC_WINDOW_TRACE("Window destructor: '{}'",
        m_Platform ? m_Platform->GetTitle() : "null");
    Destroy();
}

template<WindowPlatformConcept PlatformT>
inline void WindowT<PlatformT>::Destroy()
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

    m_Platform.reset();
}

template<WindowPlatformConcept PlatformT>
inline void WindowT<PlatformT>::Show()
{
    std::lock_guard lock(m_DestroyMutex);
    if (m_Platform && !m_Destroyed)
    {
        m_Platform->Show();
        SOLARC_WINDOW_DEBUG("Window shown: '{}'", m_Platform->GetTitle());
    }
}

template<WindowPlatformConcept PlatformT>
inline void WindowT<PlatformT>::Hide()
{
    std::lock_guard lock(m_DestroyMutex);
    if (m_Platform && !m_Destroyed)
    {
        m_Platform->Hide();
        SOLARC_WINDOW_DEBUG("Window hidden: '{}'", m_Platform->GetTitle());
    }
}

template<WindowPlatformConcept PlatformT>
inline void WindowT<PlatformT>::Resize(int32_t width, int32_t height)
{
    std::lock_guard lock(m_DestroyMutex);
    if (m_Platform && !m_Destroyed)
    {
        m_Platform->Resize(width, height);
        SOLARC_WINDOW_DEBUG("Window resize requested: '{}' to {}x{}",
            m_Platform->GetTitle(), width, height);
    }
}

template<WindowPlatformConcept PlatformT>
inline void WindowT<PlatformT>::Minimize()
{
    std::lock_guard lock(m_DestroyMutex);
    if (m_Platform && !m_Destroyed)
    {
        m_Platform->Minimize();
        SOLARC_WINDOW_DEBUG("Window minimize requested: '{}'", m_Platform->GetTitle());
    }
}

template<WindowPlatformConcept PlatformT>
inline void WindowT<PlatformT>::Maximize()
{
    std::lock_guard lock(m_DestroyMutex);
    if (m_Platform && !m_Destroyed)
    {
        m_Platform->Maximize();
        SOLARC_WINDOW_DEBUG("Window maximize requested: '{}'", m_Platform->GetTitle());
    }
}

template<WindowPlatformConcept PlatformT>
inline void WindowT<PlatformT>::Restore()
{
    std::lock_guard lock(m_DestroyMutex);
    if (m_Platform && !m_Destroyed)
    {
        m_Platform->Restore();
        SOLARC_WINDOW_DEBUG("Window restore requested: '{}'", m_Platform->GetTitle());
    }
}

template<WindowPlatformConcept PlatformT>
inline bool WindowT<PlatformT>::IsVisible() const
{
    std::lock_guard lock(m_DestroyMutex);
    return m_Platform && !m_Destroyed ? m_Platform->IsVisible() : false;
}

template<WindowPlatformConcept PlatformT>
inline bool WindowT<PlatformT>::IsMinimized() const
{
    std::lock_guard lock(m_DestroyMutex);
    return m_Platform && !m_Destroyed ? m_Platform->IsMinimized() : false;
}

template<WindowPlatformConcept PlatformT>
inline bool WindowT<PlatformT>::IsClosed() const
{
    std::lock_guard lock(m_DestroyMutex);
    return m_Destroyed;
}

template<WindowPlatformConcept PlatformT>
inline void WindowT<PlatformT>::Update()
{
    // Process all queued events

    m_Bus.Communicate();

    ProcessEvents();
}

template<WindowPlatformConcept PlatformT>
inline void WindowT<PlatformT>::OnEvent(const std::shared_ptr<const WindowEvent>& e)
{
    switch (e->GetWindowEventType())
    {

    case WindowEvent::TYPE::CLOSE:
    {
        SOLARC_WINDOW_INFO("Window close event received: '{}'", GetTitle());
        Destroy();
        break;
    }

    case WindowEvent::TYPE::SHOWN:
    {
        // Update Platform State
        SOLARC_WINDOW_TRACE("Window shown event: '{}'", GetTitle());
        break;
    }

    case WindowEvent::TYPE::HIDDEN:
    {
        // Update Platform State
        SOLARC_WINDOW_TRACE("Window hidden event: '{}'", GetTitle());
        break;
    }

    case WindowEvent::TYPE::RESIZED:
    {
        auto resizeEvent = std::static_pointer_cast<const WindowResizeEvent>(e);

        SOLARC_WINDOW_DEBUG("Window resize event: '{}' ({}x{})",
            GetTitle(), resizeEvent->GetWidth(), resizeEvent->GetHeight());
        break;
    }

    case WindowEvent::TYPE::MINIMIZED:
    {
        SOLARC_WINDOW_TRACE("Window minimized event: '{}'", GetTitle());
        break;
    }

    case WindowEvent::TYPE::MAXIMIZED:
    {
        SOLARC_WINDOW_TRACE("Window maximized event: '{}'", GetTitle());
        break;
    }

    case WindowEvent::TYPE::RESTORED:
    {
        SOLARC_WINDOW_TRACE("Window restored event: '{}'", GetTitle());
        break;
    }

    default:
        SOLARC_WINDOW_TRACE("Window generic event: '{}'", GetTitle());
        break;
    }

    DispatchEvent(e);
}