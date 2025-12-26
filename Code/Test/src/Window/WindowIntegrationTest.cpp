// WindowIntegrationTest.cpp
#include <gtest/gtest.h>
#include "Window/Window.h"
#include "Window/WindowContext.h"
#include "Event/WindowEvent.h"
#include <thread>
#include <chrono>

class WindowIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Ensure we're on main thread (required for Win32/Wayland)
        m_Context = &WindowContext::Get();
    }

    void TearDown() override {
        // Optional: explicitly shut down (WindowContext destructor does it too)
    }

    // Helper: pump platform events and window event queue
    void PumpWindowEvents(std::shared_ptr<Window> window) {
        m_Context->PollEvents();     // triggers WndProc / Wayland callbacks
        window->Update();            // processes dispatched WindowEvents
    }

    WindowContext* m_Context = nullptr;
};

// ----------------------------------------------------------------------------
// Visibility: Show -> IsVisible() = true (after event)
// ----------------------------------------------------------------------------

TEST_F(WindowIntegrationTest, Show_MakesWindowVisibleAfterEventProcessing)
{
    auto window = m_Context->CreateWindow("Test Show", 640, 480);
    EXPECT_FALSE(window->IsVisible());

    window->Show();
    // On Win32: Show() is synchronous -> IsVisible() may already be true.
    // On Wayland: Show() only commits -> visibility requires configure event.
    // So we ALWAYS pump events to be safe and consistent.
    PumpWindowEvents(window);

    EXPECT_TRUE(window->IsVisible())
        << "Window should be visible after Show() + event processing";
}

// ----------------------------------------------------------------------------
// Visibility: Hide -> IsVisible() = false
// ----------------------------------------------------------------------------

TEST_F(WindowIntegrationTest, Hide_MakesWindowInvisible)
{
    auto window = m_Context->CreateWindow("Test Hide", 640, 480);
    window->Show();
    PumpWindowEvents(window);
    ASSERT_TRUE(window->IsVisible());

    window->Hide();
    PumpWindowEvents(window);

    EXPECT_FALSE(window->IsVisible());
}

// ----------------------------------------------------------------------------
// Resize via API -> state updated
// ----------------------------------------------------------------------------

TEST_F(WindowIntegrationTest, Resize_ViaAPI_UpdatesDimensions)
{
    auto window = m_Context->CreateWindow("Test Resize", 640, 480);
    window->Show();
    PumpWindowEvents(window);

    const int32_t newW = 1024, newH = 768;
    window->Resize(newW, newH);
    PumpWindowEvents(window);

    // On Win32: Resize() calls SetWindowPos -> WM_SIZE fires immediately.
    // On Wayland: Resize() only sets min/max hints -> true size comes from compositor.
    // BUT: in both cases, your WindowPlatform::Resize() synchronously updates m_Width/m_Height.
    // So this should pass immediately, even without user-driven resize.
    EXPECT_EQ(window->GetWidth(), newW);
    EXPECT_EQ(window->GetHeight(), newH);
}

// ----------------------------------------------------------------------------
// Minimize / Restore
// ----------------------------------------------------------------------------

TEST_F(WindowIntegrationTest, Minimize_And_Restore_UpdatesState)
{
    auto window = m_Context->CreateWindow("Test Min/Restore", 640, 480);
    window->Show();
    PumpWindowEvents(window);

    // Minimize
    window->Minimize();
    PumpWindowEvents(window);
    EXPECT_TRUE(window->IsMinimized());

    // Restore
    window->Restore();
    PumpWindowEvents(window);
    EXPECT_FALSE(window->IsMinimized());
}

// ----------------------------------------------------------------------------
// Thread safety: Resize from main thread must not crash (mutex reentrancy test)
// ----------------------------------------------------------------------------

TEST_F(WindowIntegrationTest, ResizeFromMainThread_DoesNotDeadlock)
{
    auto window = m_Context->CreateWindow("Mutex Safety Test", 640, 480);
    window->Show();
    PumpWindowEvents(window);

    // This is the critical test:
    // - Calls SetWindowPos (Win32) -> triggers WM_SIZE
    // - Or commits resize (Wayland)
    // - Must not deadlock on mutex
    EXPECT_NO_THROW({
        window->Resize(800, 600);
        PumpWindowEvents(window);
        });

    EXPECT_EQ(window->GetWidth(), 800);
    EXPECT_EQ(window->GetHeight(), 600);
}