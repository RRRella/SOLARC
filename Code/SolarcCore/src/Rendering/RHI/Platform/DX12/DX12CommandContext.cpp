#ifdef SOLARC_RENDERER_DX12

#include "DX12CommandContext.h"
#include "Logging/LogMacros.h"
#include <stdexcept>

DX12CommandContext::DX12CommandContext(DX12Device* device)
    : m_Device(device)
{
    SOLARC_ASSERT(device != nullptr, "Device cannot be null");

    SOLARC_RENDER_INFO("Creating DX12 command context");

    CreateCommandQueue();
    CreateFrameResources();

    SOLARC_RENDER_INFO("DX12 command context created successfully");
}

DX12CommandContext::~DX12CommandContext()
{
    SOLARC_RENDER_TRACE("Destroying DX12 command context");

    // Wait for all GPU work to complete before destroying resources
    WaitForGPU();

    // Clean up frame resources
    for (auto& frame : m_FrameResources) {
        if (frame.fenceEvent) {
            CloseHandle(frame.fenceEvent);
            frame.fenceEvent = nullptr;
        }
        frame.fence.Reset();
        frame.commandList.Reset();
        frame.commandAllocator.Reset();
    }

    m_CommandQueue.Reset();

    SOLARC_RENDER_TRACE("DX12 command context destroyed");
}

void DX12CommandContext::CreateCommandQueue()
{
    SOLARC_RENDER_DEBUG("Creating command queue");

    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT; // Graphics queue
    queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.NodeMask = 0;

    HRESULT hr = m_Device->GetDevice()->CreateCommandQueue(
        &queueDesc,
        IID_PPV_ARGS(&m_CommandQueue)
    );

    if (FAILED(hr)) {
        auto result = ToRHIResult(hr, "CreateCommandQueue");
        SOLARC_RENDER_ERROR("Failed to create command queue: {}", result.GetResultMessage());
        throw std::runtime_error("Failed to create D3D12 command queue");
    }

    m_CommandQueue->SetName(L"Main Graphics Queue");

    SOLARC_RENDER_DEBUG("Command queue created");
}

void DX12CommandContext::CreateFrameResources()
{
    SOLARC_RENDER_DEBUG("Creating per-frame resources ({} frames in flight)", FRAMES_IN_FLIGHT);

    for (UINT i = 0; i < FRAMES_IN_FLIGHT; ++i) {
        auto& frame = m_FrameResources[i];

        // Create command allocator
        HRESULT hr = m_Device->GetDevice()->CreateCommandAllocator(
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            IID_PPV_ARGS(&frame.commandAllocator)
        );

        if (FAILED(hr)) {
            auto result = ToRHIResult(hr, "CreateCommandAllocator");
            SOLARC_RENDER_ERROR("Failed to create command allocator for frame {}: {}", 
                i, result.GetResultMessage());
            throw std::runtime_error("Failed to create command allocator");
        }

        // Create command list (initially in closed state)
        hr = m_Device->GetDevice()->CreateCommandList(
            0, // NodeMask
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            frame.commandAllocator.Get(),
            nullptr, // Initial pipeline state (null = no PSO)
            IID_PPV_ARGS(&frame.commandList)
        );

        if (FAILED(hr)) {
            auto result = ToRHIResult(hr, "CreateCommandList");
            SOLARC_RENDER_ERROR("Failed to create command list for frame {}: {}", 
                i, result.GetResultMessage());
            throw std::runtime_error("Failed to create command list");
        }

        // Command lists are created in recording state, close it immediately
        frame.commandList->Close();

        // Create fence
        hr = m_Device->GetDevice()->CreateFence(
            0, // Initial value
            D3D12_FENCE_FLAG_NONE,
            IID_PPV_ARGS(&frame.fence)
        );

        if (FAILED(hr)) {
            auto result = ToRHIResult(hr, "CreateFence");
            SOLARC_RENDER_ERROR("Failed to create fence for frame {}: {}", 
                i, result.GetResultMessage());
            throw std::runtime_error("Failed to create fence");
        }

        frame.fenceValue = 0;

        // Create fence event (Win32 event for CPU-side waiting)
        frame.fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (!frame.fenceEvent) {
            SOLARC_RENDER_ERROR("Failed to create fence event for frame {}", i);
            throw std::runtime_error("Failed to create fence event");
        }

        SOLARC_RENDER_TRACE("Created resources for frame {}", i);
    }

    SOLARC_RENDER_DEBUG("Per-frame resources created");
}

ID3D12GraphicsCommandList* DX12CommandContext::GetCommandList() const
{
    return m_FrameResources[m_CurrentFrameIndex].commandList.Get();
}

void DX12CommandContext::BeginFrame()
{
    auto& frame = m_FrameResources[m_CurrentFrameIndex];

    // Wait for this frame's previous submission to complete (if still in flight)
    WaitForFrame(m_CurrentFrameIndex);

    // Reset command allocator (recycles memory from previous frame)
    HRESULT hr = frame.commandAllocator->Reset();
    if (FAILED(hr)) {
        auto result = ToRHIResult(hr, "CommandAllocator::Reset");
        SOLARC_RENDER_ERROR("Failed to reset command allocator: {}", result.GetResultMessage());
        throw std::runtime_error("Failed to reset command allocator");
    }

    // Reset command list (prepares for new recording)
    hr = frame.commandList->Reset(frame.commandAllocator.Get(), nullptr);
    if (FAILED(hr)) {
        auto result = ToRHIResult(hr, "CommandList::Reset");
        SOLARC_RENDER_ERROR("Failed to reset command list: {}", result.GetResultMessage());
        throw std::runtime_error("Failed to reset command list");
    }

    SOLARC_RENDER_TRACE("Frame {} began (fence value: {})", m_CurrentFrameIndex, frame.fenceValue);
}

void DX12CommandContext::EndFrame()
{
    auto& frame = m_FrameResources[m_CurrentFrameIndex];

    // Close command list (finish recording)
    HRESULT hr = frame.commandList->Close();
    if (FAILED(hr)) {
        auto result = ToRHIResult(hr, "CommandList::Close");
        SOLARC_RENDER_ERROR("Failed to close command list: {}", result.GetResultMessage());
        throw std::runtime_error("Failed to close command list");
    }

    // Submit command list to queue
    ID3D12CommandList* commandLists[] = { frame.commandList.Get() };
    m_CommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

    // Signal fence for this frame
    SignalFrame(m_CurrentFrameIndex);

    SOLARC_RENDER_TRACE("Frame {} ended (fence value: {})", m_CurrentFrameIndex, frame.fenceValue);

    // Advance to next frame
    m_CurrentFrameIndex = (m_CurrentFrameIndex + 1) % FRAMES_IN_FLIGHT;
}

void DX12CommandContext::WaitForFrame(UINT frameIndex)
{
    SOLARC_ASSERT(frameIndex < FRAMES_IN_FLIGHT, "Invalid frame index");

    auto& frame = m_FrameResources[frameIndex];

    // Check if this frame is still in flight
    UINT64 completedValue = frame.fence->GetCompletedValue();
    if (completedValue < frame.fenceValue) {
        // GPU hasn't finished this frame yet, wait for it
        HRESULT hr = frame.fence->SetEventOnCompletion(frame.fenceValue, frame.fenceEvent);
        if (FAILED(hr)) {
            auto result = ToRHIResult(hr, "SetEventOnCompletion");
            SOLARC_RENDER_ERROR("Failed to set fence event: {}", result.GetResultMessage());
            throw std::runtime_error("Failed to set fence event");
        }

        SOLARC_RENDER_TRACE("Waiting for frame {} (fence value: {})", frameIndex, frame.fenceValue);
        WaitForSingleObject(frame.fenceEvent, INFINITE);
    }
}

void DX12CommandContext::SignalFrame(UINT frameIndex)
{
    SOLARC_ASSERT(frameIndex < FRAMES_IN_FLIGHT, "Invalid frame index");

    auto& frame = m_FrameResources[frameIndex];

    // Increment fence value for this frame
    frame.fenceValue++;

    // Signal the fence from the GPU side
    HRESULT hr = m_CommandQueue->Signal(frame.fence.Get(), frame.fenceValue);
    if (FAILED(hr)) {
        auto result = ToRHIResult(hr, "CommandQueue::Signal");
        SOLARC_RENDER_ERROR("Failed to signal fence: {}", result.GetResultMessage());
        throw std::runtime_error("Failed to signal fence");
    }
}

void DX12CommandContext::WaitForGPU()
{
    SOLARC_RENDER_DEBUG("Waiting for GPU to complete all work...");

    // Wait for all frames in flight
    for (UINT i = 0; i < FRAMES_IN_FLIGHT; ++i) {
        WaitForFrame(i);
    }

    SOLARC_RENDER_DEBUG("GPU idle");
}

void DX12CommandContext::TransitionResource(
    ID3D12Resource* resource,
    D3D12_RESOURCE_STATES stateBefore,
    D3D12_RESOURCE_STATES stateAfter)
{
    SOLARC_ASSERT(resource != nullptr, "Resource cannot be null");

    // Skip if already in desired state
    if (stateBefore == stateAfter) {
        return;
    }

    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = resource;
    barrier.Transition.StateBefore = stateBefore;
    barrier.Transition.StateAfter = stateAfter;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    GetCommandList()->ResourceBarrier(1, &barrier);
}

void DX12CommandContext::ClearRenderTarget(
    D3D12_CPU_DESCRIPTOR_HANDLE rtv,
    const float clearColor[4])
{
    SOLARC_ASSERT(clearColor != nullptr, "Clear color cannot be null");

    GetCommandList()->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
}

void DX12CommandContext::SetRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE rtv)
{
    GetCommandList()->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
}

void DX12CommandContext::SetViewportAndScissor(UINT width, UINT height)
{
    // Set viewport
    D3D12_VIEWPORT viewport = {};
    viewport.TopLeftX = 0.0f;
    viewport.TopLeftY = 0.0f;
    viewport.Width = static_cast<FLOAT>(width);
    viewport.Height = static_cast<FLOAT>(height);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;

    GetCommandList()->RSSetViewports(1, &viewport);

    // Set scissor rect (covers entire viewport)
    D3D12_RECT scissorRect = {};
    scissorRect.left = 0;
    scissorRect.top = 0;
    scissorRect.right = static_cast<LONG>(width);
    scissorRect.bottom = static_cast<LONG>(height);

    GetCommandList()->RSSetScissorRects(1, &scissorRect);
}

#endif // SOLARC_RENDERER_DX12