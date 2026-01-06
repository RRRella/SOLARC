#pragma once
#include "Event/WindowEvent.h"
#include <functional>
#include <memory>
#include <mutex>
#include "Preprocessor/API.h"
#include <string>
#include <unordered_map>

#ifdef _WIN32

    #include <windows.h>

#elif defined(__linux__)

    struct wl_seat;
    struct wl_keyboard;
    struct wl_pointer;
    
    #include <wayland-client.h>
    #include "xdg-shell-client-protocol.h"
    #include "xdg-decoration-unstable-v1-client-protocol.h"

#endif

class WindowPlatform; // Forward declare

/**
 * Abstract backend for WindowContext (per-platform)
 */
class WindowContextPlatform
{
public:
    // Singleton access
    static WindowContextPlatform& Get();

    // Prevent copying/moving
    WindowContextPlatform(const WindowContextPlatform&) = delete;
    WindowContextPlatform& operator=(const WindowContextPlatform&) = delete;

    WindowContextPlatform(WindowContextPlatform&&) = delete;
    WindowContextPlatform& operator=(WindowContextPlatform&&) = delete;

    void PollEvents();
    void Shutdown();

#ifdef _WIN32
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static const std::string& GetWindowClassName() { return m_WindowClassName; }
    static HINSTANCE GetInstance() { return m_hInstance; }

#elif defined(__linux__)
    wl_surface* CreateSurface();
    xdg_surface* CreateXdgSurface(wl_surface* surface);
    wl_display* GetDisplay() const { return m_Display; }
    zxdg_decoration_manager_v1* GetDecorationManager() const {
        return m_DecorationManager;
    }

#endif

private:
    WindowContextPlatform();  // Private constructor
    ~WindowContextPlatform(); // Private destructor

#ifdef _WIN32
    static inline std::mutex m_WindowClassMtx;
    static inline std::string m_WindowClassName = "SolarcEngineWindowClass";
    static inline HINSTANCE m_hInstance = nullptr;
    static inline bool m_ClassRegistered = false;

#elif defined(__linux__)

    static void registry_global(void* data, wl_registry* registry, uint32_t name, const char* interface, uint32_t version);
    static void registry_global_remove(void* data, wl_registry* registry, uint32_t name);
    static void xdg_wm_base_ping(void* data, xdg_wm_base* xdg_wm_base, uint32_t serial);

    wl_display* m_Display = nullptr;
    wl_registry* m_Registry = nullptr;
    wl_compositor* m_Compositor = nullptr;
    xdg_wm_base* m_XdgWmBase = nullptr;
    zxdg_decoration_manager_v1* m_DecorationManager = nullptr;

    bool m_ShuttingDown = false;

    static const wl_registry_listener s_RegistryListener;
    static const xdg_wm_base_listener s_XdgWmBaseListener;


    // ========================================================================
    // Input: Seat and Input Device Objects
    // ========================================================================
    wl_seat* m_Seat = nullptr;
    wl_keyboard* m_Keyboard = nullptr;
    wl_pointer* m_Pointer = nullptr;

    // Listener structures
    static const wl_seat_listener s_SeatListener;
    static const wl_keyboard_listener s_KeyboardListener;
    static const wl_pointer_listener s_PointerListener;

    // Seat capabilities tracking
    static void seat_capabilities(void* data, wl_seat* seat, uint32_t capabilities);
    static void seat_name(void* data, wl_seat* seat, const char* name);

    // Keyboard event handlers
    static void keyboard_keymap(void* data, wl_keyboard* keyboard, uint32_t format,
        int32_t fd, uint32_t size);
    static void keyboard_enter(void* data, wl_keyboard* keyboard, uint32_t serial,
        wl_surface* surface, wl_array* keys);
    static void keyboard_leave(void* data, wl_keyboard* keyboard, uint32_t serial,
        wl_surface* surface);
    static void keyboard_key(void* data, wl_keyboard* keyboard, uint32_t serial,
        uint32_t time, uint32_t key, uint32_t state);
    static void keyboard_modifiers(void* data, wl_keyboard* keyboard, uint32_t serial,
        uint32_t mods_depressed, uint32_t mods_latched,
        uint32_t mods_locked, uint32_t group);
    static void keyboard_repeat_info(void* data, wl_keyboard* keyboard,
        int32_t rate, int32_t delay);

    // Pointer (mouse) event handlers
    static void pointer_enter(void* data, wl_pointer* pointer, uint32_t serial,
        wl_surface* surface, wl_fixed_t sx, wl_fixed_t sy);
    static void pointer_leave(void* data, wl_pointer* pointer, uint32_t serial,
        wl_surface* surface);
    static void pointer_motion(void* data, wl_pointer* pointer, uint32_t time,
        wl_fixed_t sx, wl_fixed_t sy);
    static void pointer_button(void* data, wl_pointer* pointer, uint32_t serial,
        uint32_t time, uint32_t button, uint32_t state);
    static void pointer_axis(void* data, wl_pointer* pointer, uint32_t time,
        uint32_t axis, wl_fixed_t value);
    static void pointer_frame(void* data, wl_pointer* pointer);
    static void pointer_axis_source(void* data, wl_pointer* pointer, uint32_t axis_source);
    static void pointer_axis_stop(void* data, wl_pointer* pointer, uint32_t time, uint32_t axis);
    static void pointer_axis_discrete(void* data, wl_pointer* pointer, uint32_t axis, int32_t discrete);

    WindowPlatform* m_PointerFocusedWindow = nullptr;
    WindowPlatform* m_KeyboardFocusedWindow = nullptr;

#endif
};