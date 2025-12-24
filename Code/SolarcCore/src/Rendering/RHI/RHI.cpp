#include "Rendering/RHI/RHI.h"
#include "Logging/LogMacros.h"
#include <stdexcept>

// Backend-specific includes
#ifdef SOLARC_RENDERER_DX12
    #include "Rendering/RHI/Platform/DX12/DX12Device.h"
    #include "Rendering/RHI/Platform/DX12/DX12Swapchain.h"
    #include "Rendering/RHI/Platform/DX12/DX12CommandContext.h"
#elif SOLARC_RENDERER_VULKAN
    #include "Rendering/RHI/Platform/Vulkan/VulkanDevice.h"
    #include "Rendering/RHI/Platform/Vulkan/VulkanSwapchain.h"
    #include "Rendering/RHI/Platform/Vulkan/VulkanCommandContext.h"
#else
    #error "No renderer backend selected. Define SOLARC_RENDERER_DX12 or SOLARC_RENDERER_VULKAN"
#endif


// Static member initialization
std::unique_ptr<RHI> RHI::s_Instance = nullptr;
std::mutex RHI::s_InstanceMutex;

RHI& RHI::Get()
{
    SOLARC_ASSERT(s_Instance != nullptr, "RHI not initialized. Call RHI::Initialize() first.");
    return *s_Instance;
}

void RHI::Initialize(std::shared_ptr<Window> window)
{
    std::lock_guard lock(s_InstanceMutex);

    if (s_Instance != nullptr) {
        SOLARC_RENDER_WARN("RHI already initialized");
        return;
    }

    #ifdef SOLARC_RENDERER_DX12
        SOLARC_RENDER_INFO("Initializing RHI (Backend: DirectX 12)...");
    #elif SOLARC_RENDERER_VULKAN
        SOLARC_RENDER_INFO("Initializing RHI (Backend: Vulkan)...");
    #endif

    s_Instance = std::unique_ptr<RHI>(new RHI);
    s_Instance->InitializeInternal(window);

    SOLARC_RENDER_INFO("RHI initialized successfully");
}

void RHI::Shutdown()
{
    std::lock_guard lock(s_InstanceMutex);

    if (s_Instance == nullptr) {
        SOLARC_RENDER_WARN("RHI not initialized, nothing to shutdown");
        return;
    }

    SOLARC_RENDER_INFO("Shutting down RHI...");

    s_Instance->ShutdownInternal();
    s_Instance.reset();


    SOLARC_RENDER_INFO("RHI shutdown complete");
}

RHI::RHI()
{
}

RHI::~RHI()
{
    if (m_Initialized) {
        SOLARC_RENDER_WARN("RHI destructor called without explicit Shutdown()");
        ShutdownInternal();
    }
}

void RHI::InitializeInternal(std::shared_ptr<Window> window)
{
    SOLARC_ASSERT(window != nullptr, "Window cannot be null");

    if (m_Initialized) {
        SOLARC_RENDER_WARN("RHI already initialized");
        return;
    }

    m_Window = window;

    try {
        // Create device
        SOLARC_RENDER_DEBUG("Creating RHI device...");
        m_Device = std::make_unique<RHIDevice>();

        // Create command context
        SOLARC_RENDER_DEBUG("Creating command context...");
        m_CommandContext = std::make_unique<RHICommandContext>(m_Device.get());

        // Create swapchain (backend-specific)
        SOLARC_RENDER_DEBUG("Creating swapchain...");
        
        #ifdef SOLARC_RENDERER_DX12
            // DX12 swapchain requires command queue at creation
            m_Swapchain = std::make_unique<RHISwapchain>(
                m_Device.get(),
                m_CommandContext->GetCommandQueue(),
                window
            );
        #elif SOLARC_RENDERER_VULKAN
            // Vulkan swapchain doesn't need command queue at creation
            m_Swapchain = std::make_unique<RHISwapchain>(
                m_Device.get(),
                window
            );
        #endif

        m_Initialized = true;

        SOLARC_RENDER_DEBUG("RHI components created successfully");

    } catch (const std::exception& e) {
        SOLARC_RENDER_ERROR("RHI initialization failed: {}", e.what());
        
        // Clean up partial initialization
        m_Swapchain.reset();
        m_CommandContext.reset();
        m_Device.reset();
        
        throw;
    }
}

void RHI::ShutdownInternal()
{
    if (!m_Initialized) return;

    // End any in-flight frame
    {
        std::lock_guard lock(m_RHIMutex);
        if (m_InFrame) {
#ifdef SOLARC_RENDERER_DX12
            // For DX12, just close the command list
            m_CommandContext->EndFrame();
#elif SOLARC_RENDERER_VULKAN
            // For Vulkan, we need to properly end the frame/command buffer
            // Check if render pass is active and end it
            if (m_RenderPassActive) {
                m_CommandContext->EndRenderPass();
                m_RenderPassActive = false;
            }
#endif
            m_InFrame = false;
        }
    }

    // Wait for all submitted work (including last frame's GPU work)
    if (m_CommandContext) {
        m_CommandContext->WaitForGPU();
    }

    // For DX12, submit a final fence signal to ensure complete GPU idle state
    // This is necessary because DX12 drivers may have internal work queues
#ifdef SOLARC_RENDERER_DX12
    if (m_CommandContext && m_Device) {
        auto queue = m_CommandContext->GetCommandQueue();
        if (queue) {
            ComPtr<ID3D12Fence> finalFence;
            HANDLE event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
            if (event != nullptr) {
                HRESULT hr = m_Device->GetDevice()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&finalFence));
                if (SUCCEEDED(hr) && finalFence) {
                    queue->Signal(finalFence.Get(), 1);
                    finalFence->SetEventOnCompletion(1, event);
                    WaitForSingleObject(event, INFINITE);
                }
                CloseHandle(event);
            }
        }
    }
#endif

    // Now it's safe to destroy resources in reverse order
    std::lock_guard lock(m_RHIMutex);
    m_Swapchain.reset();
    m_CommandContext.reset();
    m_Device.reset();
    m_Window.reset();
    m_Initialized = false;
}

void RHI::BeginFrame()
{
    SOLARC_ASSERT(m_Initialized, "RHI not initialized");
    std::lock_guard lock(m_RHIMutex);

    // Check if we're already in a frame (real or dummy)
    SOLARC_ASSERT(!m_InFrame && !m_InDummyFrame, "BeginFrame called twice without EndFrame");

    auto window = m_Window.lock();
    bool windowReady = window &&
        !window->IsMinimized() &&
        window->IsVisible() &&
        window->GetWidth() > 0 &&
        window->GetHeight() > 0;

    if (!windowReady) {
        // Enter dummy frame: state machine active, but no GPU work
        m_InDummyFrame = true;
        m_InFrame = false;
        SOLARC_RENDER_TRACE("Entering dummy frame (window hidden/minimized/invalid)");
        return;
    }

#ifdef SOLARC_RENDERER_DX12
    // Begin command recording
    m_CommandContext->BeginFrame();

    // DX12: Transition back buffer from PRESENT to RENDER_TARGET
    m_CommandContext->TransitionResource(
        m_Swapchain->GetCurrentBackBuffer(),
        D3D12_RESOURCE_STATE_PRESENT,
        D3D12_RESOURCE_STATE_RENDER_TARGET
    );

    m_CommandContext->SetViewportAndScissor(
        m_Swapchain->GetWidth(),
        m_Swapchain->GetHeight()
    );

    m_CommandContext->SetRenderTarget(m_Swapchain->GetCurrentRTV());

#elif SOLARC_RENDERER_VULKAN
    auto initResult = m_Swapchain->Initialize();
    if (!initResult) {
        if (initResult.GetStatus() == RHIStatus::SWAPCHAIN_OUT_OF_DATE) {
            // Treat as dummy frame if swapchain can't be created yet
            m_InDummyFrame = true;
            m_InFrame = false;
            SOLARC_RENDER_TRACE("Entering dummy frame (swapchain deferred)");
            return;
        }
        throw std::runtime_error("Failed to initialize swapchain: " + initResult.GetResultMessage());
    }

    m_CommandContext->WaitForCurrentFrame();

    auto result = m_Swapchain->AcquireNextImage();
    if (!result) {
        if (result.GetStatus() == RHIStatus::SWAPCHAIN_OUT_OF_DATE) {
            ResizeSwapchain(window->GetWidth(), window->GetHeight());
            m_InDummyFrame = true;
            m_InFrame = false;
            SOLARC_RENDER_TRACE("Entering dummy frame (swapchain out of date)");
            return;
        }
        throw std::runtime_error("Failed to acquire swapchain image: " + result.GetResultMessage());
    }

    m_CommandContext->BeginFrame(m_Swapchain->GetImageAvailableSemaphore());
    m_RenderPassActive = false;
#endif

    m_InFrame = true;
    m_InDummyFrame = false;
}

void RHI::Clear(float r, float g, float b, float a)
{
    SOLARC_ASSERT(m_Initialized, "RHI not initialized");
    std::lock_guard lock(m_RHIMutex);

    // Allow Clear() in both real and dummy frames
    if (!m_InFrame && !m_InDummyFrame) {
        SOLARC_ASSERT(false, "Clear called outside BeginFrame/EndFrame");
    }

    // Skip actual clearing in dummy mode
    if (m_InDummyFrame) {
        return;
    }

#ifdef SOLARC_RENDERER_DX12
    const float clearColor[4] = { r, g, b, a };
    m_CommandContext->ClearRenderTarget(m_Swapchain->GetCurrentRTV(), clearColor);
#elif SOLARC_RENDERER_VULKAN
    if (!m_RenderPassActive) {
        const float clearColor[4] = { r, g, b, a };
        m_CommandContext->BeginRenderPass(
            m_Swapchain->GetCurrentFramebuffer(),
            m_Swapchain->GetRenderPass(),
            m_Swapchain->GetWidth(),
            m_Swapchain->GetHeight(),
            clearColor
        );
        m_RenderPassActive = true;
    }
    else {
        SOLARC_RENDER_WARN("Clear called multiple times in same frame - only first clear takes effect");
    }
#endif
}

void RHI::EndFrame()
{
    SOLARC_ASSERT(m_Initialized, "RHI not initialized");
    std::lock_guard lock(m_RHIMutex);

    if (m_InDummyFrame) {
        // Dummy frame: just clean up state
        m_InDummyFrame = false;
        m_FrameIndex++;
        return;
    }

    SOLARC_ASSERT(m_InFrame, "EndFrame called without BeginFrame");

    auto window = m_Window.lock();
    // Even if window became invalid between BeginFrame and EndFrame,
    // we assume GPU work was already started, so proceed carefully.
    // (In practice, this should not happen if window state is stable.)

#ifdef SOLARC_RENDERER_DX12
    m_CommandContext->TransitionResource(
        m_Swapchain->GetCurrentBackBuffer(),
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_PRESENT
    );

    m_CommandContext->EndFrame();
#elif SOLARC_RENDERER_VULKAN
    if (m_RenderPassActive) {
        m_CommandContext->EndRenderPass();
        m_RenderPassActive = false;
    }
    else {
        const float defaultClear[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        m_CommandContext->BeginRenderPass(
            m_Swapchain->GetCurrentFramebuffer(),
            m_Swapchain->GetRenderPass(),
            m_Swapchain->GetWidth(),
            m_Swapchain->GetHeight(),
            defaultClear
        );
        m_CommandContext->EndRenderPass();
    }

    VkSemaphore renderFinished = m_Swapchain->GetRenderFinishedSemaphoreForImage();
    m_CommandContext->EndFrame(renderFinished);
#endif

    m_InFrame = false;
    m_FrameIndex++;
}

void RHI::Present()
{
    SOLARC_ASSERT(m_Initialized, "RHI not initialized");
    std::lock_guard lock(m_RHIMutex);

    // Cannot present if we're still in a frame (real or dummy)
    SOLARC_ASSERT(!m_InFrame && !m_InDummyFrame, "Present called before EndFrame");

    auto window = m_Window.lock();
    if (!window || window->IsMinimized() || !window->IsVisible() ||
        window->GetWidth() <= 0 || window->GetHeight() <= 0) {
        // Nothing to present
        return;
    }

#ifdef SOLARC_RENDERER_DX12
    auto result = m_Swapchain->Present(m_VSync);
#elif SOLARC_RENDERER_VULKAN
    auto result = m_Swapchain->Present(m_VSync);

    m_Swapchain->AdvanceFrame();
#endif

    if (!result) {
        if (result.GetStatus() == RHIStatus::DEVICE_LOST) {
            SOLARC_RENDER_ERROR("Device lost! Application should exit.");
        }
        if (result.GetStatus() == RHIStatus::SWAPCHAIN_OUT_OF_DATE) {
            auto window = m_Window.lock();
            if (window && window->GetWidth() > 0 && window->GetHeight() > 0) {
                ResizeSwapchain(window->GetWidth(), window->GetHeight());
            }
            else {
                SOLARC_RENDER_WARN("Swapchain out of date but window not ready for resize");
            }
        }
    }
}

void RHI::SetVSync(bool enabled)
{
    std::lock_guard lock(m_RHIMutex);

    if (m_VSync != enabled) {
        m_VSync = enabled;
        SOLARC_RENDER_INFO("VSync {}", enabled ? "enabled" : "disabled");

#ifdef SOLARC_RENDERER_VULKAN
        // Vulkan: VSync change requires swapchain recreation
        // For Phase 1, we'll just apply it on next present
        // For proper implementation, trigger swapchain recreation here
        SOLARC_RENDER_DEBUG("VSync change will take effect on next present (may require resize)");
#endif
    }
}

void RHI::WaitForGPU()
{
    SOLARC_ASSERT(m_Initialized, "RHI not initialized");

    // No lock needed - WaitForGPU is thread-safe
    m_CommandContext->WaitForGPU();
}

void RHI::OnWindowResize(int32_t width, int32_t height)
{
    SOLARC_ASSERT(m_Initialized, "RHI not initialized");

    // Ignore invalid dimensions (minimized window)
    if (width <= 0 || height <= 0) {
        SOLARC_RENDER_TRACE("Ignoring invalid resize: {}x{}", width, height);
        return;
    }

    SOLARC_RENDER_INFO("Handling window resize: {}x{}", width, height);

    ResizeSwapchain(width, height);
}

void RHI::ResizeSwapchain(int32_t width, int32_t height)
{
    SOLARC_ASSERT(m_Initialized, "RHI not initialized");
    SOLARC_ASSERT(!m_InFrame, "Cannot resize during frame rendering");

    // Wait for GPU to finish all work
    SOLARC_RENDER_DEBUG("Waiting for GPU before resize...");
    m_CommandContext->WaitForGPU();

    // Resize swapchain
    SOLARC_RENDER_DEBUG("Resizing swapchain buffers...");
    auto result = m_Swapchain->Resize(static_cast<UINT>(width), static_cast<UINT>(height));
    if (!result) {
        SOLARC_RENDER_ERROR("Swapchain resize failed: {}", result.GetResultMessage());
        throw std::runtime_error("Failed to resize swapchain");
    }

    SOLARC_RENDER_INFO("Swapchain resized successfully to {}x{}", width, height);
}

void RHI::OnEvent(const std::shared_ptr<const WindowEvent>& e)
{
    if (!m_Initialized) {
        return;
    }

    switch (e->GetWindowEventType()) {
        case WindowEvent::TYPE::RESIZED: {
            auto resizeEvent = std::static_pointer_cast<const WindowResizeEvent>(e);
            OnWindowResize(resizeEvent->GetWidth(), resizeEvent->GetHeight());
            break;
        }

        case WindowEvent::TYPE::CLOSE: {
            SOLARC_RENDER_INFO("Window close event received, RHI will shutdown");
            // RHI will be shutdown by SolarcApp state machine
            break;
        }

        default:
            // Ignore other window events
            break;
    }
}
