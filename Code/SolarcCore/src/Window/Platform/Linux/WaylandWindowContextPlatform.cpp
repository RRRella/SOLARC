#ifdef __linux__
#include "Window/WindowContextPlatform.h"
#include "Window/WindowPlatform.h"
#include "Logging/LogMacros.h"
#include <stdexcept>
#include <cstring>
#include <poll.h>
#include "Input/WaylandKeyMapping.h"
#include "Input/MouseButton.h"
#include <linux/input-event-codes.h>  // For BTN_LEFT, BTN_RIGHT, etc.
#include <unistd.h>  // For close()


const wl_registry_listener WindowContextPlatform::s_RegistryListener = {
    .global = registry_global,
    .global_remove = registry_global_remove
};

const xdg_wm_base_listener WindowContextPlatform::s_XdgWmBaseListener = {
    .ping = xdg_wm_base_ping
};

WindowContextPlatform::WindowContextPlatform()
{
    m_Display = wl_display_connect(nullptr);
    if (!m_Display)
        throw std::runtime_error("Failed to connect to Wayland display");

    m_Registry = wl_display_get_registry(m_Display);
    wl_registry_add_listener(m_Registry, &s_RegistryListener, this);
    wl_display_roundtrip(m_Display);

    if (!m_Compositor || !m_XdgWmBase)
    {
        Shutdown();
        throw std::runtime_error("Missing required Wayland globals");
    }

    xdg_wm_base_add_listener(m_XdgWmBase, &s_XdgWmBaseListener, this);
    SOLARC_WINDOW_INFO("Wayland context initialized");
}

WindowContextPlatform::~WindowContextPlatform()
{
    Shutdown();
}

WindowContextPlatform& WindowContextPlatform::Get()
{
    static WindowContextPlatform instance;
    return instance;
}

void WindowContextPlatform::PollEvents()
{
    if (!m_Display || m_ShuttingDown) return;

    if (wl_display_prepare_read(m_Display) == 0)
    {
        struct pollfd pfd = 
        {
            .fd = wl_display_get_fd(m_Display),
            .events = POLLIN
        };
        

        if (poll(&pfd, 1, 0) > 0 && (pfd.revents & POLLIN))
        {
            wl_display_read_events(m_Display);
        }
        else
        {
            wl_display_cancel_read(m_Display);
        }

    }



    // Dispatch any events now available

    wl_display_dispatch_pending(m_Display);
    wl_display_flush(m_Display);
}

void WindowContextPlatform::Shutdown()
{
    if (m_ShuttingDown) return;
    m_ShuttingDown = true;

    if (m_Pointer) { wl_pointer_destroy(m_Pointer); m_Pointer = nullptr; }
    if (m_Keyboard) { wl_keyboard_destroy(m_Keyboard); m_Keyboard = nullptr; }
    if (m_Seat) { wl_seat_destroy(m_Seat); m_Seat = nullptr; }

    if (m_XdgWmBase) { xdg_wm_base_destroy(m_XdgWmBase); m_XdgWmBase = nullptr; }
    if (m_Compositor) { wl_compositor_destroy(m_Compositor); m_Compositor = nullptr; }
    if (m_Registry) { wl_registry_destroy(m_Registry); m_Registry = nullptr; }
    if (m_Display) { wl_display_disconnect(m_Display); m_Display = nullptr; }
    if (m_DecorationManager) {
        zxdg_decoration_manager_v1_destroy(m_DecorationManager);
        m_DecorationManager = nullptr;
    }

    SOLARC_WINDOW_INFO("Wayland context shut down");
}

wl_surface* WindowContextPlatform::CreateSurface()
{
    return m_Compositor ? wl_compositor_create_surface(m_Compositor) : nullptr;
}

xdg_surface* WindowContextPlatform::CreateXdgSurface(wl_surface* surface)
{
    return (m_XdgWmBase && surface) ? xdg_wm_base_get_xdg_surface(m_XdgWmBase, surface) : nullptr;
}

void WindowContextPlatform::registry_global(
    void* data, wl_registry* registry, uint32_t name, const char* interface, uint32_t version)
{
    auto* ctx = static_cast<WindowContextPlatform*>(data);
    if (strcmp(interface, wl_compositor_interface.name) == 0)
    {
        ctx->m_Compositor = static_cast<wl_compositor*>(
            wl_registry_bind(registry, name, &wl_compositor_interface, std::min(version, 4u)));
    }
    else if (strcmp(interface, xdg_wm_base_interface.name) == 0)
    {
        ctx->m_XdgWmBase = static_cast<xdg_wm_base*>(
            wl_registry_bind(registry, name, &xdg_wm_base_interface, std::min(version, 1u)));
    }
    else if (strcmp(interface, zxdg_decoration_manager_v1_interface.name) == 0)
    {
        ctx->m_DecorationManager = static_cast<zxdg_decoration_manager_v1*>(
            wl_registry_bind(registry, name, &zxdg_decoration_manager_v1_interface, 1));
    }
    else if (strcmp(interface, wl_seat_interface.name) == 0)
    {
        ctx->m_Seat = static_cast<wl_seat*>(
            wl_registry_bind(registry, name, &wl_seat_interface, std::min(version, 5u)));

        wl_seat_add_listener(ctx->m_Seat, &s_SeatListener, ctx);
    }
}

void WindowContextPlatform::registry_global_remove(void*, wl_registry*, uint32_t) {}

void WindowContextPlatform::xdg_wm_base_ping(void* data, xdg_wm_base* wm_base, uint32_t serial)
{
    xdg_wm_base_pong(wm_base, serial);
}

// ============================================================================
// Seat Listener (Manages Input Device Availability)
// ============================================================================

const wl_seat_listener WindowContextPlatform::s_SeatListener = {
    .capabilities = seat_capabilities,
    .name = seat_name
};

void WindowContextPlatform::seat_capabilities(void* data, wl_seat* seat, uint32_t capabilities)
{
    auto* ctx = static_cast<WindowContextPlatform*>(data);

    // ========================================================================
    // Keyboard Capability
    // ========================================================================
    bool hasKeyboard = capabilities & WL_SEAT_CAPABILITY_KEYBOARD;

    if (hasKeyboard && !ctx->m_Keyboard)
    {
        ctx->m_Keyboard = wl_seat_get_keyboard(seat);
        wl_keyboard_add_listener(ctx->m_Keyboard, &s_KeyboardListener, ctx);
        SOLARC_WINDOW_INFO("Wayland keyboard acquired");
    }
    else if (!hasKeyboard && ctx->m_Keyboard)
    {
        wl_keyboard_destroy(ctx->m_Keyboard);
        ctx->m_Keyboard = nullptr;
        SOLARC_WINDOW_INFO("Wayland keyboard lost");
    }

    // ========================================================================
    // Pointer Capability
    // ========================================================================

    bool hasPointer = capabilities & WL_SEAT_CAPABILITY_POINTER;

    if (hasPointer && !ctx->m_Pointer)
    {
        ctx->m_Pointer = wl_seat_get_pointer(seat);
        wl_pointer_add_listener(ctx->m_Pointer, &s_PointerListener, ctx);
        SOLARC_WINDOW_INFO("Wayland pointer acquired");
    }
    else if (!hasPointer && ctx->m_Pointer)
    {
        wl_pointer_destroy(ctx->m_Pointer);
        ctx->m_Pointer = nullptr;
        SOLARC_WINDOW_INFO("Wayland pointer lost");
    }
}

void WindowContextPlatform::seat_name(void* data, wl_seat* seat, const char* name)
{
    SOLARC_WINDOW_DEBUG("Wayland seat name: {}", name ? name : "unknown");
}

// ============================================================================
// Keyboard Listener
// ============================================================================

const wl_keyboard_listener WindowContextPlatform::s_KeyboardListener = {
    .keymap = keyboard_keymap,
    .enter = keyboard_enter,
    .leave = keyboard_leave,
    .key = keyboard_key,
    .modifiers = keyboard_modifiers,
    .repeat_info = keyboard_repeat_info
};

void WindowContextPlatform::keyboard_key(void* data, wl_keyboard* keyboard,
    uint32_t serial, uint32_t time, uint32_t key, uint32_t state)
{
    auto* ctx = static_cast<WindowContextPlatform*>(data);
    WindowPlatform* window = ctx->m_KeyboardFocusedWindow;
    if (!window) return;

    uint16_t scancode = WaylandKeyMapping::XKBKeyToScancode(key);
    bool pressed = (state == WL_KEYBOARD_KEY_STATE_PRESSED);

    bool isRepeat = false;
    if (pressed)
    {
        std::lock_guard lock(window->mtx);
        isRepeat = window->m_CurrentKeyState[scancode];
    }

    window->RecordKeyTransition(scancode, pressed, isRepeat);
}

void WindowContextPlatform::keyboard_enter(void* data, wl_keyboard* keyboard,
    uint32_t serial, wl_surface* surface, wl_array* keys)
{
    WindowPlatform* window = static_cast<WindowPlatform*>(wl_surface_get_user_data(surface));
    if (!window) return;

    auto* ctx = static_cast<WindowContextPlatform*>(data);
    ctx->m_KeyboardFocusedWindow = window;

    window->SetKeyboardFocus(true);
    SOLARC_WINDOW_DEBUG("Window '{}' gained keyboard focus", window->GetTitle());
}

void WindowContextPlatform::keyboard_leave(void* data, wl_keyboard* keyboard,
    uint32_t serial, wl_surface* surface)
{
    auto* ctx = static_cast<WindowContextPlatform*>(data);
    WindowPlatform* window = ctx->m_KeyboardFocusedWindow;
    if (!window) return;

    window->OnFocusLost();
    ctx->m_KeyboardFocusedWindow = nullptr;
    SOLARC_WINDOW_DEBUG("Window '{}' lost keyboard focus", window->GetTitle());
}

void WindowContextPlatform::keyboard_key(void* data, wl_keyboard* keyboard,
    uint32_t serial, uint32_t time, uint32_t key, uint32_t state)
{
    auto* ctx = static_cast<WindowContextPlatform*>(data);
    WindowPlatform* window = ctx->m_KeyboardFocusedWindow;
    if (!window) return;

    uint16_t scancode = WaylandKeyMapping::XKBKeyToScancode(key);
    bool pressed = (state == WL_KEYBOARD_KEY_STATE_PRESSED);

    // Detect repeat: if key is already down and pressed again -> repeat
    bool isRepeat = false;
    if (pressed)
    {
        std::lock_guard lock(window->mtx);
        isRepeat = window->m_CurrentKeyState[scancode];
    }

    window->RecordKeyTransition(scancode, pressed, isRepeat);
}

void WindowContextPlatform::keyboard_modifiers(void* data, wl_keyboard* keyboard,
    uint32_t serial, uint32_t mods_depressed,
    uint32_t mods_latched, uint32_t mods_locked,
    uint32_t group)
{
    // Modifier state tracking (Shift, Ctrl, Alt, etc.)
    // This would integrate with xkbcommon in production
    SOLARC_WINDOW_TRACE("Keyboard modifiers updated");
}

void WindowContextPlatform::keyboard_repeat_info(void* data, wl_keyboard* keyboard,
    int32_t rate, int32_t delay)
{
    SOLARC_WINDOW_DEBUG("Keyboard repeat: rate={}/sec, delay={}ms", rate, delay);
}

// ============================================================================
// Pointer (Mouse) Listener
// ============================================================================

const wl_pointer_listener WindowContextPlatform::s_PointerListener = {
    .enter = pointer_enter,
    .leave = pointer_leave,
    .motion = pointer_motion,
    .button = pointer_button,
    .axis = pointer_axis,
    .frame = pointer_frame,
    .axis_source = pointer_axis_source,
    .axis_stop = pointer_axis_stop,
    .axis_discrete = pointer_axis_discrete
};

void WindowContextPlatform::pointer_enter(void* data, wl_pointer* pointer,
    uint32_t serial, wl_surface* surface, wl_fixed_t sx, wl_fixed_t sy)
{
    WindowPlatform* window = static_cast<WindowPlatform*>(wl_surface_get_user_data(surface));
    if (!window) return;

    auto* ctx = static_cast<WindowContextPlatform*>(data);
    ctx->m_PointerFocusedWindow = window;

    int32_t x = wl_fixed_to_int(sx);
    int32_t y = wl_fixed_to_int(sy);
    window->RecordMousePosition(x, y);

    SOLARC_WINDOW_DEBUG("Pointer entered window '{}' at ({}, {})", window->GetTitle(), x, y);
}

void WindowContextPlatform::pointer_leave(void* data, wl_pointer* pointer,
    uint32_t serial, wl_surface* surface)
{
    auto* ctx = static_cast<WindowContextPlatform*>(data);
    ctx->m_PointerFocusedWindow = nullptr;
    SOLARC_WINDOW_DEBUG("Pointer left surface");
}

void WindowContextPlatform::pointer_motion(void* data, wl_pointer* pointer,
    uint32_t time, wl_fixed_t sx, wl_fixed_t sy)
{
    auto* ctx = static_cast<WindowContextPlatform*>(data);
    WindowPlatform* window = ctx->m_PointerFocusedWindow;
    if (!window) return;

    int32_t x = wl_fixed_to_int(sx);
    int32_t y = wl_fixed_to_int(sy);
    window->RecordMousePosition(x, y);
}

void WindowContextPlatform::pointer_button(void* data, wl_pointer* pointer,
    uint32_t serial, uint32_t time, uint32_t button, uint32_t state)
{
    auto* ctx = static_cast<WindowContextPlatform*>(data);
    WindowPlatform* window = ctx->m_PointerFocusedWindow;
    if (!window) return;

    MouseButton mouseBtn;
    switch (button)
    {
    case BTN_LEFT:   mouseBtn = MouseButton::Left; break;
    case BTN_RIGHT:  mouseBtn = MouseButton::Right; break;
    case BTN_MIDDLE: mouseBtn = MouseButton::Middle; break;
    case BTN_SIDE:   mouseBtn = MouseButton::X1; break;
    case BTN_EXTRA:  mouseBtn = MouseButton::X2; break;
    default: return;
    }

    bool pressed = (state == WL_POINTER_BUTTON_STATE_PRESSED);
    window->RecordMouseButton(mouseBtn, pressed);
}

void WindowContextPlatform::pointer_axis(void* data, wl_pointer* pointer,
    uint32_t time, uint32_t axis, wl_fixed_t value)
{
    auto* ctx = static_cast<WindowContextPlatform*>(data);
    WindowPlatform* window = ctx->m_PointerFocusedWindow;
    if (!window) return;

    float delta = wl_fixed_to_double(value);
    if (axis == WL_POINTER_AXIS_VERTICAL_SCROLL)
    {
        window->RecordMouseWheel(delta, 0.0f);
    }
    else if (axis == WL_POINTER_AXIS_HORIZONTAL_SCROLL)
    {
        window->RecordMouseWheel(0.0f, delta);
    }
}

void WindowContextPlatform::pointer_frame(void* data, wl_pointer* pointer)
{
    // Frame event signals end of a logical input event group
    // All accumulated axis/button events should be processed together
}

void WindowContextPlatform::pointer_axis_source(void* data, wl_pointer* pointer,
    uint32_t axis_source)
{
    // Indicates source of scroll (wheel, finger, continuous)
}

void WindowContextPlatform::pointer_axis_stop(void* data, wl_pointer* pointer,
    uint32_t time, uint32_t axis)
{
    // Scroll gesture stopped
}

void WindowContextPlatform::pointer_axis_discrete(void* data, wl_pointer* pointer,
    uint32_t axis, int32_t discrete)
{
    // Discrete scroll events (wheel "clicks")
    SOLARC_WINDOW_TRACE("Pointer discrete scroll: axis={}, clicks={}", axis, discrete);
}

#endif