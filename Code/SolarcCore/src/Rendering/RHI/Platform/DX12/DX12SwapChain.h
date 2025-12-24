#pragma once

#ifdef SOLARC_RENDERER_DX12

#include "DX12Device.h"
#include "Rendering/RHI/RHIResult.h"
#include "Window/Window.h"
#include <wrl/client.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <memory>
#include <vector>

using Microsoft::WRL::ComPtr;

/**
 * DX12 Swapchain Wrapper
 * 
 * Manages swapchain and render target views for presenting to window.
 * Handles resize and format selection.
 */
class DX12Swapchain
{
public:
    static constexpr UINT BUFFER_COUNT = 2; // Double-buffered
    static constexpr DXGI_FORMAT FORMAT = DXGI_FORMAT_R8G8B8A8_UNORM;

    /**
     * Create swapchain for given window
     * param device: DX12 device
     * param commandQueue: Command queue for presentation
     * param window: Target window
     * throws std::runtime_error if creation fails
     */
    DX12Swapchain(DX12Device* device, ID3D12CommandQueue* commandQueue, 
                  std::shared_ptr<Window> window);
    ~DX12Swapchain();

    // Non-copyable, non-movable
    DX12Swapchain(const DX12Swapchain&) = delete;
    DX12Swapchain& operator=(const DX12Swapchain&) = delete;
    DX12Swapchain(DX12Swapchain&&) = delete;
    DX12Swapchain& operator=(DX12Swapchain&&) = delete;

    /**
     * Get the current back buffer index
     * return Index of the current back buffer (0 or 1)
     */
    UINT GetCurrentBackBufferIndex() const;

    /**
     * Get the current back buffer resource
     * return Pointer to current back buffer
     */
    ID3D12Resource* GetCurrentBackBuffer() const;

    /**
     * Get RTV (Render Target View) for current back buffer
     * return CPU descriptor handle for RTV
     */
    D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentRTV() const;

    /**
     * Present the current frame
     * param vsync: If true, sync to monitor refresh rate. If false, present immediately.
     * return RHIResult indicating success or failure
     */
    RHIResult Present(bool vsync);

    /**
     * Resize swapchain to new dimensions
     * param width: New width (must be > 0)
     * param height: New height (must be > 0)
     * return RHIResult indicating success or failure
     * note: Must call ReleaseBackBuffers() before and RecreateBackBuffers() after
     */
    RHIResult Resize(UINT width, UINT height);

    /**
     * Release references to back buffers
     * note: Must be called before Resize()
     */
    void ReleaseBackBuffers();

    /**
     * Recreate back buffer resources after resize
     * note: Must be called after Resize()
     */
    void RecreateBackBuffers();

    /**
     * Get current swapchain dimensions
     */
    UINT GetWidth() const { return m_Width; }
    UINT GetHeight() const { return m_Height; }

private:
    void CreateSwapchain(ID3D12CommandQueue* commandQueue, HWND hwnd);
    void CreateRTVDescriptorHeap();
    void CreateRenderTargetViews();

    DX12Device* m_Device; // Non-owning
    std::weak_ptr<Window> m_Window;

    ComPtr<IDXGISwapChain3> m_Swapchain;
    ComPtr<ID3D12DescriptorHeap> m_RTVHeap;
    std::vector<ComPtr<ID3D12Resource>> m_BackBuffers;

    UINT m_Width = 0;
    UINT m_Height = 0;
    UINT m_RTVDescriptorSize = 0;
};

#endif // SOLARC_RENDERER_DX12