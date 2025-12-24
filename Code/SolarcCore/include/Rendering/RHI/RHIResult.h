#pragma once
#include <string>

#ifdef SOLARC_RENDERER_DX12
    #include <wrl/client.h>
    #include <d3d12.h>
    #include <dxgi1_6.h>
#elif SOLARC_RENDERER_VULKAN
    #include <vulkan/vulkan.h>
#endif

#include "Preprocessor/API.h"

/**
 * RHI operation status codes
 * Platform-agnostic error categories
 */
enum class RHIStatus
{
    SUCCESS = 0,
    DEVICE_LOST,
    OUT_OF_MEMORY,
    SWAPCHAIN_OUT_OF_DATE,
    INITIALIZATION_FAILED,
    INVALID_OPERATION,
    UNKNOWN_ERROR
};

/**
 * Result wrapper for RHI operations
 * Wraps platform-specific error codes (HRESULT, VkResult) into unified type
 */
class SOLARC_CORE_API RHIResult
{
public:
    RHIStatus status = RHIStatus::SUCCESS;
    std::string message;

    #ifdef SOLARC_RENDERER_DX12
        HRESULT hresult = S_OK;
    #elif SOLARC_RENDERER_VULKAN
        VkResult vkresult = VK_SUCCESS;
    #endif

    // Default constructor (success)
    RHIResult() = default;

    // Construct from status and message
    RHIResult(RHIStatus s, const std::string& msg = "")
        : status(s), message(msg)
    {}

    // Check if operation succeeded
    bool IsSuccess() const { return status == RHIStatus::SUCCESS; }
    
    // Allow implicit conversion to bool for easy checking
    explicit operator bool() const { return IsSuccess(); }

    // Get human-readable description
    const std::string& GetResultMessage() const { return message; }
    RHIStatus GetStatus() const { return status; }
};

// Platform-specific result conversion functions
#ifdef SOLARC_RENDERER_DX12

inline RHIResult ToRHIResult(HRESULT hr, const std::string& context = "")
{
    if (SUCCEEDED(hr)) {
        return RHIResult(RHIStatus::SUCCESS, "");
    }

    RHIResult result;
    result.hresult = hr;

    switch (hr) {
        case DXGI_ERROR_DEVICE_REMOVED:
        case DXGI_ERROR_DEVICE_RESET:
            result.status = RHIStatus::DEVICE_LOST;
            result.message = context + " - Device removed/reset";
            break;

        case E_OUTOFMEMORY:
            result.status = RHIStatus::OUT_OF_MEMORY;
            result.message = context + " - Out of memory";
            break;

        case DXGI_ERROR_INVALID_CALL:
            result.status = RHIStatus::INVALID_OPERATION;
            result.message = context + " - Invalid API call";
            break;

        default:
            result.status = RHIStatus::UNKNOWN_ERROR;
            result.message = context + " - HRESULT: 0x" + 
                std::to_string(static_cast<uint32_t>(hr));
            break;
    }

    return result;
}

#elif SOLARC_RENDERER_VULKAN

inline RHIResult ToRHIResult(VkResult vk, const std::string& context = "")
{
    if (vk == VK_SUCCESS) {
        return RHIResult(RHIStatus::SUCCESS, "");
    }

    RHIResult result;
    result.vkresult = vk;

    switch (vk) {
        case VK_ERROR_DEVICE_LOST:
            result.status = RHIStatus::DEVICE_LOST;
            result.message = context + " - Device lost";
            break;

        case VK_ERROR_OUT_OF_HOST_MEMORY:
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
            result.status = RHIStatus::OUT_OF_MEMORY;
            result.message = context + " - Out of memory";
            break;

        case VK_ERROR_OUT_OF_DATE_KHR:
        case VK_SUBOPTIMAL_KHR:
            result.status = RHIStatus::SWAPCHAIN_OUT_OF_DATE;
            result.message = context + " - Swapchain out of date";
            break;

        case VK_ERROR_INITIALIZATION_FAILED:
            result.status = RHIStatus::INITIALIZATION_FAILED;
            result.message = context + " - Initialization failed";
            break;
        case VK_ERROR_SURFACE_LOST_KHR:
            result.status = RHIStatus::SWAPCHAIN_OUT_OF_DATE;
            result.message = context + " - Swapchain out of date";
            break;

        default:
            result.status = RHIStatus::UNKNOWN_ERROR;
            result.message = context + " - VkResult: " + std::to_string(static_cast<int32_t>(vk));
            break;
    }

    return result;
}

#endif