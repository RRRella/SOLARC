#pragma once
#include "Preprocessor/API.h"
#include "RHIResult.h"
#include "Window/Window.h"
#include "Event/EventListener.h"
#include "Event/WindowEvent.h"
#include <memory>
#include <mutex>

// Forward declarations for backend types
#ifdef SOLARC_RENDERER_DX12
    class DX12Device;
    class DX12Swapchain;
    class DX12CommandContext;
    
    using RHIDevice = DX12Device;
    using RHISwapchain = DX12Swapchain;
    using RHICommandContext = DX12CommandContext;
#elif SOLARC_RENDERER_VULKAN
    class VulkanDevice;
    class VulkanSwapchain;
    class VulkanCommandContext;
    
    using RHIDevice = VulkanDevice;
    using RHISwapchain = VulkanSwapchain;
    using RHICommandContext = VulkanCommandContext;
#else
    #error "No Renderer Backend Selected!"
#endif

/**
 * Rendering Hardware Interface (RHI)
 * 
 * Singleton managing graphics device, swapchain, and command execution.
 * Provides platform-agnostic API for basic rendering operations.
 * 
 * Lifetime:
 * - Initialize() creates GPU resources and registers with window events
 * - Shutdown() destroys resources in correct order (waits for GPU idle)
 * - Must be initialized after Window creation, destroyed before Window destruction
 * 
 * Thread Safety:
 * - Initialize/Shutdown: Main thread only
 * - Frame cycle (Begin/Clear/End/Present): Main thread only
 * - WaitForGPU: Any thread (synchronization primitive)
 */
class SOLARC_CORE_API RHI : public EventListener<WindowEvent>
{
public:
    // Singleton access (must call Initialize first)
    static RHI& Get();

    // Check if RHI is initialized
    static bool IsInitialized() { return s_Instance != nullptr; }

    /**
     * Initialize the RHI with a target window
     * param window: Window to render to (must remain alive until Shutdown)
     * throws std::runtime_error if initialization fails
     * note: Must be called from main thread
     */

    static void Initialize(std::shared_ptr<Window> window);

    /**
     * Shutdown the RHI and release all GPU resources
     * note: Waits for GPU to finish all work before destroying resources
     * note: Idempotent - safe to call multiple times
     * note: Must be called from main thread
     */
    static void Shutdown();

    /**
     * Begin rendering a new frame
     * note: Must be called before any rendering commands
     * note: Automatically waits if previous frame is still in flight
     * note: Skips frame if window is minimized
     */
    void BeginFrame();

    /**
     * Clear the current render target
     * param r, g, b, a: Clear color components (0.0 - 1.0 range)
     * note: Must be called between BeginFrame() and EndFrame()
     */
    void Clear(float r, float g, float b, float a);

    /**
     * End the current frame
     * note: Submits command buffer to GPU
     * note: Must be called after BeginFrame() and rendering commands
     */
    void EndFrame();

    /**
     * Present the rendered frame to the screen
     * note: Respects VSync setting
     * note: Must be called after EndFrame()
     */
    void Present();

    /**
     * Enable or disable vertical synchronization
     * param enabled: true = lock to monitor refresh rate, false = unlimited FPS
     * note: Can be changed at any time, takes effect on next Present()
     */
    void SetVSync(bool enabled);

    /**
     * Get current VSync state
     * return true if VSync is enabled
     */
    bool GetVSync() const { return m_VSync; }

    /**
     * Block until all GPU work is complete
     * note: Thread-safe, can be called from any thread
     * note: Used before resize or shutdown to ensure no resources are in use
     */
    void WaitForGPU();

    /**
     * Handle window resize event
     * param width: New window width
     * param height: New window height
     * note: Called automatically via event system
     * note: Recreates swapchain with new dimensions
     */
    void OnWindowResize(int32_t width, int32_t height);
    /**
     * Get current frame index (for debugging)
     * return Frame counter (wraps at UINT32_MAX)
     */
    uint32_t GetCurrentFrameIndex() const { return m_FrameIndex; }

protected:
    // EventListener override - handles window resize events
    void OnEvent(const std::shared_ptr<const WindowEvent>& e) override;

private:
    // Private constructor/destructor (singleton pattern)
    RHI();
    ~RHI() override;

    friend struct std::default_delete<RHI>;

    // Internal initialization logic
    void InitializeInternal(std::shared_ptr<Window> window);
    void ShutdownInternal();

    // Resize handling
    void ResizeSwapchain(int32_t width, int32_t height);

    // Singleton instance
    static std::unique_ptr<RHI> s_Instance;
    static std::mutex s_InstanceMutex;

    // Core components (owned by RHI)
    std::unique_ptr<RHIDevice> m_Device;
    std::unique_ptr<RHISwapchain> m_Swapchain;
    std::unique_ptr<RHICommandContext> m_CommandContext;

    // Window reference (borrowed, not owned)
    std::weak_ptr<Window> m_Window;

    // State tracking
    bool m_Initialized = false;
    bool m_InFrame = false;      // True between BeginFrame/EndFrame
    bool m_InDummyFrame = false;  // True when window is hidden but frame cycle is logically active
    bool m_VSync = true;         // VSync enabled by default
    uint32_t m_FrameIndex = 0;   // Current frame number

#ifdef SOLARC_RENDERER_VULKAN
        float m_PendingClearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f }; // Default black
        bool m_RenderPassActive = false;
#endif
    
    // Thread safety
    mutable std::mutex m_RHIMutex;
};