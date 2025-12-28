// WindowIntegrationTest.cpp
#include <gtest/gtest.h>
#include "Window/Window.h"
#include "Window/WindowContext.h"
#include "Event/WindowEvent.h"
#include <thread>
#include <chrono>
#ifdef __linux__
    #include <wayland-client.h>
#endif

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
        #ifdef _WIN32
            m_Context->PollEvents();     // triggers WndProc / Wayland callbacks
        #elif defined(__linux__)
            wl_display_roundtrip(WindowContextPlatform::Get().GetDisplay()); 
            window->Update();
        #endif
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

    auto initialWidth = window->GetWidth();
    auto initialHeight = window->GetHeight();

    EXPECT_EQ(initialWidth , 640);
    EXPECT_EQ(initialHeight , 480);

    const int32_t newW = 200, newH = 200;
    window->Resize(newW, newH);
    PumpWindowEvents(window);
#ifdef _WIN32
    EXPECT_EQ(window->GetWidth(), newW);
    EXPECT_EQ(window->GetHeight(), newH);
#else
    if(window->GetWidth() == initialWidth && window->GetHeight() == initialHeight)
        GTEST_SKIP() << "Wayland compositor ignored resize hints (normal behavior)";
    else
    {
        EXPECT_NE(window->GetWidth() , initialWidth);
        EXPECT_NE(window->GetHeight() , initialHeight);
    }
#endif
}

// ----------------------------------------------------------------------------
// Maximize / Minimize / Restore
// ----------------------------------------------------------------------------

TEST_F(WindowIntegrationTest, IsMinimized_Behavior)
{
    auto window = m_Context->CreateWindow("MinTest", 640, 480);
    window->Show();
    PumpWindowEvents(window);

    window->Minimize();
    PumpWindowEvents(window);

#ifdef _WIN32
    EXPECT_TRUE(window->IsMinimized());
#else
    // On Wayland, we expect either:
    // - true (if compositor sent 0x0), or
    // - false (if compositor didn't)
    // But we can check: if size is 0x0, IsMinimized should be true
    if (window->GetWidth() == 0 && window->GetHeight() == 0)
    {
        EXPECT_TRUE(window->IsMinimized());
    }
    else
    {
        EXPECT_FALSE(window->IsMinimized());
        GTEST_SKIP() << "Compositor did not send 0x0 configure";
    }
#endif
}

// ----------------------------------------------------------------------------
// Thread safety: Resize from main thread must not crash (mutex reentrancy test)
// ----------------------------------------------------------------------------

TEST_F(WindowIntegrationTest, ResizeFromMainThread_DoesNotDeadlock)
{
    auto window = m_Context->CreateWindow("Mutex Safety Test", 640, 480);
    window->Show();
    PumpWindowEvents(window);


    auto initialWidth = window->GetWidth();
    auto initialHeight = window->GetHeight();

    // This is the critical test:
    // - Calls SetWindowPos (Win32) -> triggers WM_SIZE
    // - Or commits resize (Wayland)
    // - Must not deadlock on mutex
    EXPECT_NO_THROW({
        window->Resize(800, 600);
        PumpWindowEvents(window);
        });

#ifdef _WIN32
    EXPECT_EQ(window->GetWidth(), newW);
    EXPECT_EQ(window->GetHeight(), newH);
#else
    if(window->GetWidth() == initialWidth && window->GetHeight() == initialHeight)
        GTEST_SKIP() << "Wayland compositor ignored resize hints (normal behavior)";
    else
    {
        EXPECT_NE(window->GetWidth() , initialWidth);
        EXPECT_NE(window->GetHeight() , initialHeight);
    }
#endif
}