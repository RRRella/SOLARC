#include <gtest/gtest.h>
#include <memory>
#include <string>

// Include the headers under test
#include "Window/Window.h"
#include "Event/WindowEvent.h"

// ============================================================================
// Mock Window Platform
// ============================================================================

/**
 * A mock platform that satisfies WindowPlatformConcept.
 *
 * It must inherit from EventProducer<WindowEvent> because
 * WindowT registers the platform as a producer in its constructor.
 */

class MockWindowPlatform : public EventProducer<WindowEvent>
{
public:
    std::string m_Title;
    int32_t m_Width;
    int32_t m_Height;

    // State tracking
    bool m_IsVisible = false;
    bool m_IsMinimized = false;

    // Spies (to verify calls were made)
    bool m_WasShown = false;
    bool m_WasHidden = false;
    bool m_WasResized = false;
    bool m_WasMinimized = false;
    bool m_WasMaximized = false;
    bool m_WasRestored = false;

    MockWindowPlatform(const std::string& title, int32_t width, int32_t height)
        : m_Title(title), m_Width(width), m_Height(height)
    {
    }

    // -- Getters --
    const std::string& GetTitle() const { return m_Title; }
    const int32_t& GetWidth() const { return m_Width; }
    const int32_t& GetHeight() const { return m_Height; }
    bool IsVisible() const { return m_IsVisible; }
    bool IsMinimized() const { return m_IsMinimized; }

    // -- Commands (Simulate OS Requests) --

    void Show()
    {
        m_WasShown = true;
        // In the real app, this sends a request to the OS.
        // For testing, we simulate the OS immediately responding with an event.
        DispatchEvent(std::make_shared<WindowShownEvent>());
    }

    void Hide()
    {
        m_WasHidden = true;
        // Simulate OS response
        DispatchEvent(std::make_shared<WindowHiddenEvent>());
    }

    void Resize(int32_t width, int32_t height)
    {
        m_WasResized = true;
        // Simulate OS response: Dispatch Resize Event
        DispatchEvent(std::make_shared<WindowResizeEvent>(width, height));
    }

    void Minimize()
    {
        m_WasMinimized = true;
        // Simulate OS response: Dispatch Minimized Event
        DispatchEvent(std::make_shared<WindowMinimizedEvent>());
    }

    void Maximize()
    {
        m_WasMaximized = true;
        // Note: We don't have a specific Maximize event in the provided code, 
        // usually this triggers a Resize event or a StateChange event.
        // For now, we just track the call.
    }

    void Restore()
    {
        m_WasRestored = true;
        // Simulate OS response: Dispatch Restored Event
        DispatchEvent(std::make_shared<WindowRestoredEvent>());
    }

    // -- Sync Methods (New Interface required by WindowT) --
    // These are called by Window::OnEvent to update authoritative state

    void SyncVisibility(bool visible)
    {
        m_IsVisible = visible;
    }

    void SyncDimensions(int32_t width, int32_t height)
    {
        m_Width = width;
        m_Height = height;
    }

    void SyncMinimized(bool minimized)
    {
        m_IsMinimized = minimized;
    }

    // Helper for testing events manually
    void SimulateEvent(std::shared_ptr<WindowEvent> e)
    {
        DispatchEvent(e);
    }
};

// Define the Window type using our Mock Platform
using TestWindow = WindowT<MockWindowPlatform>;


// ============================================================================
// Test Suite
// ============================================================================

class WindowTest : public ::testing::Test {
protected:
    std::shared_ptr<TestWindow> CreateWindowAndMock(
        MockWindowPlatform** outMockPlatform,
        std::function<void(TestWindow*)> onDestroy = nullptr)
    {
        auto platform = std::make_unique<MockWindowPlatform>("TestWindow", 800, 600);
        *outMockPlatform = platform.get();

        return std::make_shared<TestWindow>(std::move(platform), onDestroy);
    }
};

// ----------------------------------------------------------------------------
// Construction & Properties
// ----------------------------------------------------------------------------

TEST_F(WindowTest, Constructor_ThrowsOnNullPlatform)
{
    EXPECT_THROW({
        TestWindow w(nullptr, nullptr);
        }, std::invalid_argument);
}

TEST_F(WindowTest, Properties_DelegatesToPlatform)
{ 
    MockWindowPlatform* mock = nullptr;
    auto window = CreateWindowAndMock(&mock, nullptr);

    EXPECT_EQ(window->GetTitle(), "TestWindow");
    EXPECT_EQ(window->GetWidth(), 800);
    EXPECT_EQ(window->GetHeight(), 600);
    EXPECT_EQ(window->GetPlatform(), mock);
}


// ----------------------------------------------------------------------------
// Visibility Logic
// ----------------------------------------------------------------------------

TEST_F(WindowTest, Show_CallsPlatformShow_AndUpdatesStateViaEvent)
{
    MockWindowPlatform* mock = nullptr;
    auto window = CreateWindowAndMock(&mock, nullptr);

    EXPECT_FALSE(window->IsVisible());

    // 1. Request Show
    window->Show();

    // Check that the platform request was made
    EXPECT_TRUE(mock->m_WasShown);

    // CRITICAL CHANGE: The state should NOT be true yet.
    // It waits for the Event Bus to process the WindowShownEvent.
    EXPECT_FALSE(window->IsVisible());

    // 2. Process Events
    window->Update();

    // 3. Now the state should be updated
    EXPECT_TRUE(window->IsVisible());
}

TEST_F(WindowTest, Hide_CallsPlatformHide_AndUpdatesStateViaEvent)
{
    MockWindowPlatform* mock = nullptr;
    auto window = CreateWindowAndMock(&mock, nullptr);

    // Get into shown state first
    window->Show();
    window->Update();
    ASSERT_TRUE(window->IsVisible());

    // 1. Request Hide
    window->Hide();

    EXPECT_TRUE(mock->m_WasHidden);

    // State shouldn't change until update
    EXPECT_TRUE(window->IsVisible());

    // 2. Process Events
    window->Update();

    // 3. Now hidden
    EXPECT_FALSE(window->IsVisible());
}

TEST_F(WindowTest, Resize_CallsPlatform_AndUpdatesStateViaEvent)
{
    MockWindowPlatform* mock = nullptr;
    auto window = CreateWindowAndMock(&mock, nullptr);

    // 1. Request Resize
    window->Resize(1280, 720);

    // Verify platform call
    EXPECT_TRUE(mock->m_WasResized);

    // State not updated yet (event pending)
    EXPECT_EQ(window->GetWidth(), 800);

    // 2. Process Events
    window->Update();

    // 3. Verify state update
    EXPECT_EQ(window->GetWidth(), 1280);
    EXPECT_EQ(window->GetHeight(), 720);
}

TEST_F(WindowTest, MinimizeCommand_CallsPlatform_AndUpdatesState)
{
    MockWindowPlatform* mock = nullptr;
    auto window = CreateWindowAndMock(&mock, nullptr);

    EXPECT_FALSE(window->IsMinimized());

    // 1. Request Minimize via Window API
    window->Minimize();

    EXPECT_TRUE(mock->m_WasMinimized);

    // State pending
    EXPECT_FALSE(window->IsMinimized());

    // 2. Process
    window->Update();

    EXPECT_TRUE(window->IsMinimized());
}

TEST_F(WindowTest, MaximizeCommand_CallsPlatform)
{
    MockWindowPlatform* mock = nullptr;
    auto window = CreateWindowAndMock(&mock, nullptr);

    window->Maximize();

    EXPECT_TRUE(mock->m_WasMaximized);
}

TEST_F(WindowTest, RestoreCommand_CallsPlatform_AndUpdatesState)
{
    MockWindowPlatform* mock = nullptr;
    auto window = CreateWindowAndMock(&mock, nullptr);

    // Set to minimized first
    window->Minimize();
    window->Update();
    ASSERT_TRUE(window->IsMinimized());

    // 1. Request Restore
    window->Restore();

    EXPECT_TRUE(mock->m_WasRestored);

    // 2. Process
    window->Update();

    EXPECT_FALSE(window->IsMinimized());
}

// ----------------------------------------------------------------------------
// Destruction Lifecycle
// ----------------------------------------------------------------------------

TEST_F(WindowTest, Destroy_TriggersCallback)
{
    bool destroyed = false;
    MockWindowPlatform* mock = nullptr;

    auto window = CreateWindowAndMock(&mock, [&](TestWindow* w) {
        destroyed = true;
        });

    EXPECT_FALSE(window->IsClosed());

    window->Destroy();

    EXPECT_TRUE(destroyed);
    EXPECT_TRUE(window->IsClosed());
}

TEST_F(WindowTest, Destroy_IsIdempotent)
{
    int destroyCount = 0;
    MockWindowPlatform* mock = nullptr;

    auto window = CreateWindowAndMock(&mock, [&](TestWindow*) {
        destroyCount++;
        });

    window->Destroy();
    window->Destroy();
    window->Destroy();

    EXPECT_EQ(destroyCount, 1);
}

TEST_F(WindowTest, OperationsIgnoredAfterDestruction)
{
    MockWindowPlatform* mock = nullptr;
    auto window = CreateWindowAndMock(&mock, nullptr);

    window->Destroy();

    // Reset mock spy
    mock->m_WasShown = false;

    window->Show();

    EXPECT_FALSE(mock->m_WasShown);
}

// ----------------------------------------------------------------------------
// Event Integration
// ----------------------------------------------------------------------------

TEST_F(WindowTest, Event_CloseEventTriggersDestroy)
{
    bool callbackCalled = false;
    MockWindowPlatform* mock = nullptr;

    auto window = CreateWindowAndMock(&mock, [&](TestWindow*) {
        callbackCalled = true;
        });

    // 1. Simulate the platform generating a Close Event
    mock->SimulateEvent(std::make_shared<WindowCloseEvent>());

    // 2. Process bus
    window->Update();

    // 3. Verify destruction
    EXPECT_TRUE(callbackCalled);
    EXPECT_TRUE(window->IsClosed());
}

TEST_F(WindowTest, Event_ResizeEventUpdatesDimensions)
{
    MockWindowPlatform* mock = nullptr;
    auto window = CreateWindowAndMock(&mock, nullptr);

    // Verify initial
    EXPECT_EQ(window->GetWidth(), 800);

    // 1. Simulate Resize Event (e.g. user dragged corner)
    mock->SimulateEvent(std::make_shared<WindowResizeEvent>(1024, 768));

    // 2. Process
    window->Update();

    // 3. Window should have called mock->SyncDimensions internally
    // and the getter should reflect that
    EXPECT_EQ(window->GetWidth(), 1024);
    EXPECT_EQ(window->GetHeight(), 768);
}

TEST_F(WindowTest, Minimize_CallsPlatformAndUpdatesStateViaEvent)
{
    MockWindowPlatform* mock = nullptr;
    auto window = CreateWindowAndMock(&mock, nullptr);

    // Verify initial state
    EXPECT_FALSE(window->IsMinimized());

    // 1. Request Minimize (Simulate OS action)
    mock->SimulateEvent(std::make_shared<WindowMinimizedEvent>());

    // Verify state is NOT updated immediately (waiting for event bus)
    EXPECT_FALSE(window->IsMinimized());

    // 2. Process Events
    window->Update();

    // 3. Verify state is now updated
    EXPECT_TRUE(window->IsMinimized());

    // 4. Restore
    mock->SimulateEvent(std::make_shared<WindowRestoredEvent>());
    window->Update();
    EXPECT_FALSE(window->IsMinimized());
}