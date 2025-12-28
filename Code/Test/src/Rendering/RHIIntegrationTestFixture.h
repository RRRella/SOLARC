#pragma once
#include <gtest/gtest.h>
#include <memory>
#include "Window/Window.h"
#include "Window/WindowContext.h"
#include "Rendering/RHI/RHI.h"
#include "Event/ObserverBus.h"

/**
 * Base fixture for RHI integration tests
 *
 * Creates real Window and initializes real RHI with actual GPU.
 * These are integration tests - they test the complete system.
 */
class RHIPerTestIntegrationTestFixture : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Create a real window (hidden) for testing
        auto& windowCtx = WindowContext::Get();

        m_Window = windowCtx.CreateWindow(
            "RHI Integration Test Window",
            800,  // Width
            600   // Height
        );

        // Keep window hidden during tests
        m_Window->Hide();

        PumpWindowEvents(m_Window);

        // Initialize RHI with the real window
        RHI::Initialize(m_Window);

        m_Bus.RegisterListener(&RHI::Get());
        m_Bus.RegisterProducer(m_Window.get());
    }

    void TearDown() override
    {
        // Shutdown in correct order: RHI first, then window
        if (RHI::IsInitialized()) {
            RHI::Shutdown();
        }

        if (m_Window) {
            m_Window->Destroy();
            m_Window.reset();
        }
    }

    // Helper to run a complete frame cycle
    void RunFrameCycle(float r = 0.1f, float g = 0.2f, float b = 0.3f, float a = 1.0f)
    {
        auto& rhi = RHI::Get();

        rhi.BeginFrame();
        rhi.Clear(r, g, b, a);
        rhi.EndFrame();
        rhi.Present();
    }

    void PumpWindowEvents(std::shared_ptr<Window> window) {
        #ifdef _WIN32
            m_Context->PollEvents();     // triggers WndProc / Wayland callbacks
        #elif defined(__linux__)
            wl_display_roundtrip(WindowContextPlatform::Get().GetDisplay()); 
            window->Update();
        #endif
    }

    std::shared_ptr<Window> m_Window;
    ObserverBus<WindowEvent> m_Bus;
};