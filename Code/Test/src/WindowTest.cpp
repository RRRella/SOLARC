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
 * Crucial: It must inherit from EventProducer<WindowEvent> because
 * WindowT registers the platform as a producer in its constructor.
 */
class MockWindowPlatform : public EventProducer<WindowEvent>
{
public:
    std::string m_Title;
    int32_t m_Width;
    int32_t m_Height;

    // State tracking for verification
    bool m_IsVisible = false;
    bool m_WasShown = false;
    bool m_WasHidden = false;

    MockWindowPlatform(const std::string& title, int32_t width, int32_t height)
        : m_Title(title), m_Width(width), m_Height(height)
    {
    }

    // Concept Requirements
    const std::string& GetTitle() const { return m_Title; }
    const int32_t& GetWidth() const { return m_Width; }
    const int32_t& GetHeight() const { return m_Height; }

    void Show()
    {
        m_IsVisible = true;
        m_WasShown = true;
    }

    void Hide()
    {
        m_IsVisible = false;
        m_WasHidden = true;
    }

    bool IsVisible() const { return m_IsVisible; }

    // Helper for testing events: 
    // Simulate the OS generating an event (e.g., clicking 'X')
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
    // Helper to create a window and keep a raw pointer to the mock for assertions
    std::shared_ptr<TestWindow> CreateWindowAndMock(
        MockWindowPlatform** outMockPlatform,
        std::function<void(TestWindow*)> onDestroy = nullptr)
    {
        auto platform = std::make_unique<MockWindowPlatform>("TestWindow", 800, 600);
        *outMockPlatform = platform.get(); // Keep raw ptr before move

        return std::make_shared<TestWindow>(std::move(platform), onDestroy);
    }
};

// ----------------------------------------------------------------------------
// Construction & Properties
// ----------------------------------------------------------------------------

TEST_F(WindowTest, Constructor_ThrowsOnNullPlatform)
{
    // WindowT should verify that the platform pointer is valid
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

    // Verify we can access the platform raw pointer
    EXPECT_EQ(window->GetPlatform(), mock);
}

// ----------------------------------------------------------------------------
// Visibility Logic
// ----------------------------------------------------------------------------

TEST_F(WindowTest, Show_CallsPlatformShow)
{
    MockWindowPlatform* mock = nullptr;
    auto window = CreateWindowAndMock(&mock, nullptr);

    EXPECT_FALSE(window->IsVisible());

    window->Show();

    EXPECT_TRUE(mock->m_WasShown);
    EXPECT_TRUE(window->IsVisible());
}

TEST_F(WindowTest, Hide_CallsPlatformHide)
{
    MockWindowPlatform* mock = nullptr;
    auto window = CreateWindowAndMock(&mock, nullptr);

    window->Show();
    window->Hide();

    EXPECT_TRUE(mock->m_WasHidden);
    EXPECT_FALSE(window->IsVisible());
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
    window->Destroy(); // Second call should be ignored
    window->Destroy(); // Third call should be ignored

    EXPECT_EQ(destroyCount, 1);
}

TEST_F(WindowTest, OperationsIgnoredAfterDestruction)
{
    MockWindowPlatform* mock = nullptr;
    auto window = CreateWindowAndMock(&mock, nullptr);

    window->Destroy();

    // Reset mock state
    mock->m_WasShown = false;

    // Try to show a destroyed window
    window->Show();

    // Should NOT have called Show on platform
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

    // 1. Simulate the platform generating a Close Event (e.g. User clicked X)
    mock->SimulateEvent(std::make_shared<WindowCloseEvent>());

    // 2. The event is now in the Bus buffer. 
    // We must call Update() to process the bus and dispatch to OnEvent.
    window->Update();

    // 3. Verify the window reacted to the event by destroying itself
    EXPECT_TRUE(callbackCalled);
    EXPECT_TRUE(window->IsClosed());
}

TEST_F(WindowTest, Event_OtherEventsDoNotDestroy)
{
    bool callbackCalled = false;
    MockWindowPlatform* mock = nullptr;

    auto window = CreateWindowAndMock(&mock, [&](TestWindow*) {
        callbackCalled = true;
        });

    // Simulate a generic event (e.g., Shown/Hidden/Moved)
    mock->SimulateEvent(std::make_shared<WindowShownEvent>());

    window->Update();

    // Verify window is STILL ALIVE
    EXPECT_FALSE(callbackCalled);
    EXPECT_FALSE(window->IsClosed());
}