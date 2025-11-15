#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "Window/WindowPlatform.h"
#include "Window/Window.h"
#include "Window/WindowContext.h"
#include "Window/WindowContextPlatform.h"
#include "Event/WindowEvent.h"

using ::testing::_;
using ::testing::NiceMock;
using ::testing::StrictMock;
using ::testing::Return;
using ::testing::Invoke;

// Mock WindowPlatform
class MockWindowPlatform : public WindowPlatform
{
public:
    MockWindowPlatform(const std::string& title , const int32_t& width , const int32_t& height) 
        : WindowPlatform(title , width, height)
    {}

    MOCK_METHOD(void, Show, (), (override));
    MOCK_METHOD(void, Hide, (), (override));
    MOCK_METHOD(bool, IsVisible, (), (const, override));
};

// Mock WindowContextPlatform
class MockWindowContextPlatform : public WindowContextPlatform
{
public:
    MOCK_METHOD(void, PollEvents, (), (override));
    MOCK_METHOD(void, Shutdown, (), (override));
    MOCK_METHOD(std::shared_ptr<WindowPlatform>, CreateWindowPlatform,
        (const std::string&, const int32_t&, const int32_t&), (override));

    // Test-friendly method to dispatch events
    void DispatchEventForTest(std::shared_ptr<const WindowEvent> event)
    {
        DispatchWindowEvent(std::move(event));
    }

    void CallBusCommunicate()
    {
    }
};
// Testable Window subclass
class TestableWindow : public Window
{
public:
    using Window::Window;

    int GetCloseEventCount() const { return m_CloseEventCount; }

protected:
    void OnEvent(const std::shared_ptr<const WindowEvent>& e) override
    {
        if (e->GetWindowEventType() == WindowEvent::TYPE::CLOSE)
        {
            m_CloseEventCount++;
            Window::OnEvent(e); // Trigger Destroy()
        }
    }

private:
    int m_CloseEventCount = 0;
};

// Test: Window::Show/Hide delegates to platform
TEST(Window_Basic_ShowHide, ShowAndHideDelegatesToPlatform)
{
    auto mockPlatform = std::make_shared<StrictMock<MockWindowPlatform>>("TestWindow", 1024, 768);
    EXPECT_CALL(*mockPlatform, Show()).Times(1);
    EXPECT_CALL(*mockPlatform, Hide()).Times(1);

    Window window(mockPlatform);
    window.Show();
    window.Hide();
}

// Test: WindowContext creates standard Window
TEST(WindowContext_Creation, CreatesStandardWindow)
{
    auto mockPlatform = std::make_shared<StrictMock<MockWindowPlatform>>("TestWindow", 1024, 768);
    auto mockCtxPlatform = std::make_unique<StrictMock<MockWindowContextPlatform>>();

    EXPECT_CALL(*mockCtxPlatform, CreateWindowPlatform("TestWindow", 1024, 768))
        .WillOnce(Return(mockPlatform));
    EXPECT_CALL(*mockCtxPlatform, Shutdown()).Times(1);

    EXPECT_CALL(*mockPlatform, Show()).Times(1);

    WindowContext ctx(std::move(mockCtxPlatform));
    auto window = ctx.CreateWindow("TestWindow", 1024, 768);
    ASSERT_NE(window, nullptr);
    window->Show();
}

// Test: Custom window receives close event and destroys itself
TEST(Window_CloseEvent, CustomWindowHandlesCloseRequested)
{
    auto mockPlatform = std::make_shared<StrictMock<MockWindowPlatform>>("TestWindow", 1024, 768);
    auto mockCtxPlatform = std::make_unique<StrictMock<MockWindowContextPlatform>>();

    auto* rawPlatform = static_cast<MockWindowContextPlatform*>(mockCtxPlatform.get());
    
    EXPECT_CALL(*mockCtxPlatform, CreateWindowPlatform("TestWindow", 1024, 768))
        .WillOnce(Return(mockPlatform));
    EXPECT_CALL(*mockCtxPlatform, Shutdown()).Times(1);
    EXPECT_CALL(*mockCtxPlatform, PollEvents()).Times(1);

    WindowContext ctx(std::move(mockCtxPlatform));

    // Create custom testable window
    auto window = ctx.CreateWindow<TestableWindow>("TestWindow", 1024, 768);
    ASSERT_NE(window, nullptr);

    // Simulate OS close event
    auto closeEvent = std::make_shared<WindowCloseEvent>();

    // Use the test-friendly dispatch method
    rawPlatform->DispatchEventForTest(closeEvent);

    ctx.PollEvents();

    EXPECT_EQ(window->GetCloseEventCount(), 1);
}

// Test: WindowContext::Shutdown destroys all windows
TEST(WindowContext_Shutdown, DestroysAllWindowsOnShutdown)
{
    auto mockPlatform1 = std::make_shared<StrictMock<MockWindowPlatform>>("TestWindow", 1024, 768);
    auto mockPlatform2 = std::make_shared<StrictMock<MockWindowPlatform>>("TestWindow2", 1024, 768);
    auto mockCtxPlatform = std::make_unique<StrictMock<MockWindowContextPlatform>>();

    EXPECT_CALL(*mockCtxPlatform, CreateWindowPlatform(_, _, _))
        .WillOnce(Return(mockPlatform1))
        .WillOnce(Return(mockPlatform2));
    EXPECT_CALL(*mockCtxPlatform, Shutdown()).Times(1); // destructor calls this

    {
        WindowContext ctx(std::move(mockCtxPlatform));
        auto w1 = ctx.CreateWindow("TestWindow", 1024, 768);
        auto w2 = ctx.CreateWindow("TestWindow2", 1024, 768);
    } // destructor automatically calls Shutdown()
}

// Test: Manual destruction is safe
TEST(WindowContext_ManualDestruction, SafeToDestroyManually)
{
    auto mockPlatform = std::make_shared<StrictMock<MockWindowPlatform>>("TestWindow", 1024, 768);
    auto mockCtxPlatform = std::make_unique<StrictMock<MockWindowContextPlatform>>();

    EXPECT_CALL(*mockCtxPlatform, CreateWindowPlatform(_, _, _))
        .WillOnce(Return(mockPlatform));
    EXPECT_CALL(*mockCtxPlatform, Shutdown()).Times(1); // destructor calls this

    WindowContext ctx(std::move(mockCtxPlatform));
    auto window = ctx.CreateWindow("TestWindow", 1024, 768);

    window->Destroy(); // Manual destroy
}

// Test: PollEvents delegates to platform
TEST(WindowContext_PollEvents, DelegatesToPlatform)
{
    auto mockCtxPlatform = std::make_unique<StrictMock<MockWindowContextPlatform>>();
    EXPECT_CALL(*mockCtxPlatform, PollEvents()).Times(1);
    EXPECT_CALL(*mockCtxPlatform, Shutdown()).Times(1); // destructor calls this

    WindowContext ctx(std::move(mockCtxPlatform));
    ctx.PollEvents();
}