#pragma once

#ifdef SOLARC_RENDERER_DX12

#include "Rendering/RHI/RHIResult.h"
#include <wrl/client.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <memory>

using Microsoft::WRL::ComPtr;

/**
 * DX12 Device Wrapper
 * 
 * Manages D3D12 device and DXGI factory creation.
 * Enables debug layer in debug builds for validation.
 * Prefers discrete GPU over integrated.
 */
class DX12Device
{
public:
    /**
     * Create DX12 device
     * throws std::runtime_error if device creation fails
     */
    DX12Device();
    ~DX12Device();

    // Non-copyable, non-movable
    DX12Device(const DX12Device&) = delete;
    DX12Device& operator=(const DX12Device&) = delete;
    DX12Device(DX12Device&&) = delete;
    DX12Device& operator=(DX12Device&&) = delete;

    /**
     * Get the D3D12 device
     * return Raw pointer to ID3D12Device (never null after construction)
     */
    ID3D12Device* GetDevice() const { return m_Device.Get(); }

    /**
     * Get the DXGI factory
     * return Raw pointer to IDXGIFactory4 (never null after construction)
     */
    IDXGIFactory4* GetFactory() const { return m_Factory.Get(); }

    /**
     * Get the adapter (GPU) being used
     * return Raw pointer to IDXGIAdapter1
     */
    IDXGIAdapter1* GetAdapter() const { return m_Adapter.Get(); }

    /**
     * Check if tearing (variable refresh rate) is supported
     * return true if tearing is supported
     */
    bool IsTearingSupported() const { return m_TearingSupported; }

private:
    void EnableDebugLayer();
    void CreateFactory();
    void SelectAdapter();
    void CreateDevice();
    void CheckTearingSupport();

    ComPtr<ID3D12Device> m_Device;
    ComPtr<IDXGIFactory4> m_Factory;
    ComPtr<IDXGIAdapter1> m_Adapter;
    
    bool m_TearingSupported = false;

    #ifdef SOLARC_DEBUG_BUILD
        ComPtr<ID3D12Debug> m_DebugController;
    #endif
};

#endif // SOLARC_RENDERER_DX12