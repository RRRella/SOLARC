#pragma once

#ifdef SOLARC_RENDERER_DX12

#include "DX12Device.h"
#include "Rendering/RHI/RHIResult.h"
#include <wrl/client.h>
#include <d3d12.h>
#include <array>
#include <cstdint>

using Microsoft::WRL::ComPtr;

/**
 * DX12 Command Context
 * 
 * Manages command queue, allocators, and command lists for GPU command submission.
 * Implements frame pacing with fences to prevent CPU from getting ahead of GPU.
 */
class DX12CommandContext
{
public:
    static constexpr UINT FRAMES_IN_FLIGHT = 2; // Double-buffered

    /**
     * Create command context for device
     * param device: DX12 device
     * throws std::runtime_error if creation fails
     */
    explicit DX12CommandContext(DX12Device* device);
    ~DX12CommandContext();

    // Non-copyable, non-movable
    DX12CommandContext(const DX12CommandContext&) = delete;
    DX12CommandContext& operator=(const DX12CommandContext&) = delete;
    DX12CommandContext(DX12CommandContext&&) = delete;
    DX12CommandContext& operator=(DX12CommandContext&&) = delete;

    /**
     * Get the command queue
     * return Pointer to ID3D12CommandQueue
     */
    ID3D12CommandQueue* GetCommandQueue() const { return m_CommandQueue.Get(); }

    /**
     * Get the current command list
     * return Pointer to ID3D12GraphicsCommandList
     */
    ID3D12GraphicsCommandList* GetCommandList() const;

    /**
     * Begin a new frame
     * note: Waits for the fence from FRAMES_IN_FLIGHT ago
     * note: Resets command allocator and command list for current frame
     */
    void BeginFrame();

    /**
     * End the current frame
     * note: Closes the command list and submits it to the queue
     * note: Signals the fence for this frame
     */
    void EndFrame();

    /**
     * Wait for all frames to complete on GPU
     * note: Blocks until GPU is idle
     * note: Used before resize or shutdown
     */
    void WaitForGPU();

    /**
     * Get current frame index (for debugging)
     */
    UINT GetCurrentFrameIndex() const { return m_CurrentFrameIndex; }

    /**
     * Transition a resource to a new state
     * param resource: Resource to transition
     * param stateBefore: Current state
     * param stateAfter: Desired state
     */
    void TransitionResource(
        ID3D12Resource* resource,
        D3D12_RESOURCE_STATES stateBefore,
        D3D12_RESOURCE_STATES stateAfter
    );
    
    /**
     * Clear the render target
     * param rtv: Render target view to clear
     * param clearColor: RGBA clear color (4 floats)
     */
    void ClearRenderTarget(
        D3D12_CPU_DESCRIPTOR_HANDLE rtv,
        const float clearColor[4]
    );
    
    /**
     * Set the render target
     * param rtv: Render target view to bind
     */
    void SetRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE rtv);
    
    /**
     * Set viewport and scissor rect to cover entire render target
     * param width: Viewport width
     * param height: Viewport height
     */
    void SetViewportAndScissor(UINT width, UINT height);

private:
    struct FrameResources
    {
        ComPtr<ID3D12CommandAllocator> commandAllocator;
        ComPtr<ID3D12GraphicsCommandList> commandList;
        ComPtr<ID3D12Fence> fence;
        UINT64 fenceValue = 0;
        HANDLE fenceEvent = nullptr;
    };

    void CreateCommandQueue();
    void CreateFrameResources();
    void WaitForFrame(UINT frameIndex);
    void SignalFrame(UINT frameIndex);

    DX12Device* m_Device; // Non-owning
    ComPtr<ID3D12CommandQueue> m_CommandQueue;
    std::array<FrameResources, FRAMES_IN_FLIGHT> m_FrameResources;
    UINT m_CurrentFrameIndex = 0;
};

#endif // SOLARC_RENDERER_DX12