#ifdef SOLARC_RENDERER_DX12

#include "DX12Swapchain.h"
#include "Logging/LogMacros.h"
#include <stdexcept>


DX12Swapchain::DX12Swapchain(DX12Device* device, ID3D12CommandQueue* commandQueue,
                             std::shared_ptr<Window> window)
    : m_Device(device)
    , m_Window(window)
{
    SOLARC_ASSERT(device != nullptr, "Device cannot be null");
    SOLARC_ASSERT(commandQueue != nullptr, "Command queue cannot be null");
    SOLARC_ASSERT(window != nullptr, "Window cannot be null");

    m_Width = static_cast<UINT>(window->GetWidth());
    m_Height = static_cast<UINT>(window->GetHeight());

    SOLARC_RENDER_INFO("Creating DX12 swapchain ({}x{})", m_Width, m_Height);

    // Get native window handle
    HWND hwnd = static_cast<HWND>(window->GetPlatform()->GetWin32Handle());
    if (!hwnd) {
        throw std::runtime_error("Invalid window handle for swapchain creation");
    }

    CreateSwapchain(commandQueue, hwnd);
    CreateRTVDescriptorHeap();
    CreateRenderTargetViews();

    SOLARC_RENDER_INFO("DX12 swapchain created successfully");
}

DX12Swapchain::~DX12Swapchain()
{
    SOLARC_RENDER_TRACE("Destroying DX12 swapchain");

    ReleaseBackBuffers();
    m_RTVHeap.Reset();
    m_Swapchain.Reset();

    SOLARC_RENDER_TRACE("DX12 swapchain destroyed");
}

void DX12Swapchain::CreateSwapchain(ID3D12CommandQueue* commandQueue, HWND hwnd)
{
    SOLARC_RENDER_DEBUG("Creating DXGI swapchain");

    // Describe the swapchain
    DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};
    swapchainDesc.Width = m_Width;
    swapchainDesc.Height = m_Height;
    swapchainDesc.Format = FORMAT;
    swapchainDesc.Stereo = FALSE;
    swapchainDesc.SampleDesc.Count = 1; // No MSAA
    swapchainDesc.SampleDesc.Quality = 0;
    swapchainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapchainDesc.BufferCount = BUFFER_COUNT;
    swapchainDesc.Scaling = DXGI_SCALING_STRETCH;
    swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // Modern flip model
    swapchainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    swapchainDesc.Flags = m_Device->IsTearingSupported() ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

    ComPtr<IDXGISwapChain1> swapchain1;
    HRESULT hr = m_Device->GetFactory()->CreateSwapChainForHwnd(
        commandQueue,
        hwnd,
        &swapchainDesc,
        nullptr, // Fullscreen desc (null = windowed)
        nullptr, // Restrict output (null = no restriction)
        &swapchain1
    );

    if (FAILED(hr)) {
        auto result = ToRHIResult(hr, "CreateSwapChainForHwnd");
        SOLARC_RENDER_ERROR("Failed to create swapchain: {}", result.GetResultMessage());
        throw std::runtime_error("Failed to create D3D12 swapchain");
    }

    // Upgrade to IDXGISwapChain3 (required for GetCurrentBackBufferIndex)
    hr = swapchain1.As(&m_Swapchain);
    if (FAILED(hr)) {
        auto result = ToRHIResult(hr, "QueryInterface IDXGISwapChain3");
        SOLARC_RENDER_ERROR("Failed to get IDXGISwapChain3 interface: {}", result.GetResultMessage());
        throw std::runtime_error("Failed to get IDXGISwapChain3 interface");
    }

    // Disable Alt+Enter fullscreen toggle (we'll handle this ourselves if needed)
    m_Device->GetFactory()->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);

    SOLARC_RENDER_DEBUG("DXGI swapchain created");
}

void DX12Swapchain::CreateRTVDescriptorHeap()
{
    SOLARC_RENDER_DEBUG("Creating RTV descriptor heap");

    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.NumDescriptors = BUFFER_COUNT;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE; // RTVs are not shader-visible
    rtvHeapDesc.NodeMask = 0;

    HRESULT hr = m_Device->GetDevice()->CreateDescriptorHeap(
        &rtvHeapDesc,
        IID_PPV_ARGS(&m_RTVHeap)
    );

    if (FAILED(hr)) {
        auto result = ToRHIResult(hr, "CreateDescriptorHeap (RTV)");
        SOLARC_RENDER_ERROR("Failed to create RTV descriptor heap: {}", result.GetResultMessage());
        throw std::runtime_error("Failed to create RTV descriptor heap");
    }
    
    m_RTVDescriptorSize = m_Device->GetDevice()->GetDescriptorHandleIncrementSize(
        D3D12_DESCRIPTOR_HEAP_TYPE_RTV
    );

    SOLARC_RENDER_DEBUG("RTV descriptor heap created (descriptor size: {} bytes)", 
        m_RTVDescriptorSize);
}

void DX12Swapchain::CreateRenderTargetViews()
{
    SOLARC_RENDER_DEBUG("Creating render target views for back buffers");

    m_BackBuffers.resize(BUFFER_COUNT);

    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_RTVHeap->GetCPUDescriptorHandleForHeapStart();

    for (UINT i = 0; i < BUFFER_COUNT; ++i) {
        HRESULT hr = m_Swapchain->GetBuffer(i, IID_PPV_ARGS(&m_BackBuffers[i]));
        if (FAILED(hr)) {
            auto result = ToRHIResult(hr, "GetBuffer");
            SOLARC_RENDER_ERROR("Failed to get back buffer {}: {}", i, result.GetResultMessage());
            throw std::runtime_error("Failed to get swapchain back buffer");
        }

        m_Device->GetDevice()->CreateRenderTargetView(
            m_BackBuffers[i].Get(),
            nullptr, // Use resource format
            rtvHandle
        );

        rtvHandle.ptr += m_RTVDescriptorSize;

        SOLARC_RENDER_TRACE("Created RTV for back buffer {}", i);
    }

    SOLARC_RENDER_DEBUG("Render target views created");
}

UINT DX12Swapchain::GetCurrentBackBufferIndex() const
{
    return m_Swapchain->GetCurrentBackBufferIndex();
}

ID3D12Resource* DX12Swapchain::GetCurrentBackBuffer() const
{
    UINT index = GetCurrentBackBufferIndex();
    return m_BackBuffers[index].Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE DX12Swapchain::GetCurrentRTV() const
{
    UINT index = GetCurrentBackBufferIndex();
    
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_RTVHeap->GetCPUDescriptorHandleForHeapStart();
    rtvHandle.ptr += index * m_RTVDescriptorSize;
    
    return rtvHandle;
}

RHIResult DX12Swapchain::Present(bool vsync)
{
    UINT syncInterval = vsync ? 1 : 0;
    UINT presentFlags = (!vsync && m_Device->IsTearingSupported()) ? DXGI_PRESENT_ALLOW_TEARING : 0;

    HRESULT hr = m_Swapchain->Present(syncInterval, presentFlags);

    if (FAILED(hr)) {
        auto result = ToRHIResult(hr, "Present");
        SOLARC_RENDER_ERROR("Present failed: {}", result.GetResultMessage());
        return result;
    }

    return RHIResult(RHIStatus::SUCCESS);
}

RHIResult DX12Swapchain::Resize(UINT width, UINT height)
{
    SOLARC_ASSERT(width > 0 && height > 0, "Invalid swapchain dimensions");

    SOLARC_RENDER_INFO("Resizing swapchain: {}x{} -> {}x{}", m_Width, m_Height, width, height);

    m_Width = width;
    m_Height = height;

    UINT flags = m_Device->IsTearingSupported() ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

    HRESULT hr = m_Swapchain->ResizeBuffers(
        BUFFER_COUNT,
        m_Width,
        m_Height,
        FORMAT,
        flags
    );

    if (FAILED(hr)) {
        auto result = ToRHIResult(hr, "ResizeBuffers");
        SOLARC_RENDER_ERROR("Failed to resize swapchain: {}", result.GetResultMessage());
        return result;
    }

    SOLARC_RENDER_DEBUG("Swapchain resized successfully");
    return RHIResult(RHIStatus::SUCCESS);
}

void DX12Swapchain::ReleaseBackBuffers()
{
    SOLARC_RENDER_TRACE("Releasing back buffer references");

    for (auto& buffer : m_BackBuffers) {
        buffer.Reset();
    }
}

void DX12Swapchain::RecreateBackBuffers()
{
    SOLARC_RENDER_TRACE("Recreating back buffers after resize");

    CreateRenderTargetViews();

    SOLARC_RENDER_DEBUG("Back buffers recreated");
}

#endif // SOLARC_RENDERER_DX12