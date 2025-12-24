#ifdef SOLARC_RENDERER_DX12

#include "DX12Device.h"
#include "Logging/LogMacros.h"
#include <stdexcept>
#include <vector>

DX12Device::DX12Device()
{
    SOLARC_RENDER_INFO("Creating DX12 device...");

    EnableDebugLayer();
    CreateFactory();
    SelectAdapter();
    CreateDevice();
    CheckTearingSupport();

    SOLARC_RENDER_INFO("DX12 device created successfully");
}

DX12Device::~DX12Device()
{
    SOLARC_RENDER_TRACE("Destroying DX12 device");

    // Release in reverse order
    m_Device.Reset();
    m_Adapter.Reset();
    m_Factory.Reset();

    #ifdef SOLARC_DEBUG_BUILD
        m_DebugController.Reset();
    #endif

    SOLARC_RENDER_TRACE("DX12 device destroyed");
}

void DX12Device::EnableDebugLayer()
{
    #ifdef SOLARC_DEBUG_BUILD
        SOLARC_RENDER_DEBUG("Enabling D3D12 debug layer");

        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&m_DebugController)))) {
            m_DebugController->EnableDebugLayer();

            // Enable GPU-based validation (more thorough, but slower)
            ComPtr<ID3D12Debug1> debugController1;
            if (SUCCEEDED(m_DebugController.As(&debugController1))) {
                debugController1->SetEnableGPUBasedValidation(true);
                SOLARC_RENDER_DEBUG("GPU-based validation enabled");
            }
        } else {
            SOLARC_RENDER_WARN("Failed to enable D3D12 debug layer (missing Graphics Tools?)");
        }
    #endif
}

void DX12Device::CreateFactory()
{
    SOLARC_RENDER_DEBUG("Creating DXGI factory");

    UINT factoryFlags = 0;

    #ifdef SOLARC_DEBUG_BUILD
        factoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
    #endif

    HRESULT hr = CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&m_Factory));
    if (FAILED(hr)) {
        auto result = ToRHIResult(hr, "CreateDXGIFactory2");
        SOLARC_RENDER_ERROR("Failed to create DXGI factory: {}", result.GetResultMessage());
        throw std::runtime_error("Failed to create DXGI factory");
    }

    SOLARC_RENDER_DEBUG("DXGI factory created");
}

void DX12Device::SelectAdapter()
{
    SOLARC_RENDER_DEBUG("Selecting GPU adapter");

    ComPtr<IDXGIAdapter1> adapter;
    SIZE_T maxDedicatedVideoMemory = 0;

    // Enumerate adapters and select the one with most dedicated video memory
    for (UINT i = 0; m_Factory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i) {
        DXGI_ADAPTER_DESC1 desc;
        adapter->GetDesc1(&desc);

        // Skip software adapters
        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
            SOLARC_RENDER_TRACE("Skipping software adapter: {}", 
                reinterpret_cast<const char*>(desc.Description));
            continue;
        }

        // Check if adapter supports D3D12
        if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_1, 
            _uuidof(ID3D12Device), nullptr))) {
            
            SOLARC_RENDER_DEBUG("Found compatible adapter: {} (VRAM: {} MB)",
                reinterpret_cast<const char*>(desc.Description),
                desc.DedicatedVideoMemory / (1024 * 1024));

            // Prefer adapter with most video memory (discrete GPU over integrated)
            if (desc.DedicatedVideoMemory > maxDedicatedVideoMemory) {
                maxDedicatedVideoMemory = desc.DedicatedVideoMemory;
                m_Adapter = adapter;
            }
        }
    }

    if (!m_Adapter) {
        SOLARC_RENDER_ERROR("No compatible D3D12 adapter found");
        throw std::runtime_error("No compatible GPU found for D3D12");
    }

    DXGI_ADAPTER_DESC1 selectedDesc;
    m_Adapter->GetDesc1(&selectedDesc);
    SOLARC_RENDER_INFO("Selected GPU: {} (VRAM: {} MB)",
        reinterpret_cast<const char*>(selectedDesc.Description),
        selectedDesc.DedicatedVideoMemory / (1024 * 1024));
}

void DX12Device::CreateDevice()
{
    SOLARC_RENDER_DEBUG("Creating D3D12 device with feature level 12_1");

    HRESULT hr = D3D12CreateDevice(
        m_Adapter.Get(),
        D3D_FEATURE_LEVEL_12_1, // Enforced feature level
        IID_PPV_ARGS(&m_Device)
    );
    if (FAILED(hr)) {
        auto result = ToRHIResult(hr, "D3D12CreateDevice");
        SOLARC_RENDER_ERROR("Failed to create D3D12 device: {}", result.GetResultMessage());
        throw std::runtime_error("Failed to create D3D12 device");
    }

    #ifdef SOLARC_DEBUG_BUILD
        // Configure debug device to break on errors
        ComPtr<ID3D12InfoQueue> infoQueue;
        if (SUCCEEDED(m_Device.As(&infoQueue))) {
            infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
            infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);

            // Filter out common benign warnings
            D3D12_MESSAGE_ID hide[] = {
                D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
            };

            D3D12_INFO_QUEUE_FILTER filter = {};
            filter.DenyList.NumIDs = _countof(hide);
            filter.DenyList.pIDList = hide;
            infoQueue->AddStorageFilterEntries(&filter);

            SOLARC_RENDER_DEBUG("D3D12 debug info queue configured");
        }
    #endif

    SOLARC_RENDER_DEBUG("D3D12 device created");
}

void DX12Device::CheckTearingSupport()
{
    BOOL allowTearing = FALSE;
    ComPtr<IDXGIFactory5> factory5;

    if (SUCCEEDED(m_Factory.As(&factory5))) {
        HRESULT hr = factory5->CheckFeatureSupport(
            DXGI_FEATURE_PRESENT_ALLOW_TEARING,
            &allowTearing,
            sizeof(allowTearing)
        );

        m_TearingSupported = SUCCEEDED(hr) && allowTearing;
    }

    SOLARC_RENDER_DEBUG("Tearing support (VRR): {}", m_TearingSupported ? "YES" : "NO");
}

#endif // SOLARC_RENDERER_DX12