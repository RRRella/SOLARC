#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "Window/WindowPlatform.h"
#include "Window/WindowPlatformFactory.h"
#include "Window/Window.h"
#include "Window/WindowContext.h"
#include "Window/WindowContextPlatform.h"
#include "Window/WindowContextPlatformFactory.h"
#include "Event/WindowEvent.h"

using ::testing::_;
using ::testing::NiceMock;
using ::testing::StrictMock;
using ::testing::Return;
using ::testing::Invoke;

// ============================================================================
// Mock Classes
// ============================================================================

// Mock WindowPlatform
class MockWindowPlatform : public WindowPlatform
{
public:
    MockWindowPlatform(const std::string& title, const int32_t& width, const int32_t& height) 
        : WindowPlatform(title, width, height)
        , m_FakeHandle(reinterpret_cast<void*>(this)) // Unique fake handle
    {}

    MOCK_METHOD(void, Show, (), (override));
    MOCK_METHOD(void, Hide, (), (override));
    MOCK_METHOD(bool, IsVisible, (), (const, override));
    
    void* GetNativeHandle() const override { return m_FakeHandle; }

private:
    void* m_FakeHandle;
};

// Mock WindowPlatformFactory
class MockWindowPlatformFactory : public WindowPlatformFactory
{
public:
    MOCK_METHOD(std::unique_ptr<WindowPlatform>, Create,
        (const std::string&, int32_t, int32_t), (override));
};

// Mock WindowContextPlatform
class MockWindowContextPlatform : public WindowContextPlatform
{
public:
    MOCK_METHOD(void, RegisterWindow, (WindowPlatform*), (override));
    MOCK_METHOD(void, UnregisterWindow, (WindowPlatform*), (override));
    MOCK_METHOD(void, PollEvents, (), (override));
    MOCK_METHOD(void, Shutdown, (), (override));

    // Test-friendly method to dispatch events
    void DispatchEventForTest(std::shared_ptr<const WindowEvent> event)
    {
        DispatchWindowEvent(std::move(event));
    }
};

// Mock WindowContextPlatformFactory
class MockWindowContextPlatformFactory : public WindowContextPlatformFactory
{
public:
    MockWindowContextPlatformFactory(
        std::unique_ptr<WindowContextPlatform> context,
        std::unique_ptr<WindowPlatformFactory> factory)
        : m_Context(std::move(context))
        , m_Factory(std::move(factory))
    {}

    PlatformComponents CreateComponents() override
    {
        PlatformComponents components;
        components.context = std::move(m_Context);
        components.windowFactory = std::move(m_Factory);
        return components;
    }

private:
    std::unique_ptr<WindowContextPlatform> m_Context;
    std::unique_ptr<WindowPlatformFactory> m_Factory;
};

// Testable Window subclass
class TestableWindow : public Window
{
public:
    using Window::Window;
    using Window::OnEvent;

};

// ============================================================================
// Window Basic Tests
// ============================================================================

TEST(Window_Basic_ShowHide, ShowDelegatesToPlatform)
{
    auto mockPlatform = std::make_unique<StrictMock<MockWindowPlatform>>("TestWindow", 1024, 768);
    
    EXPECT_CALL(*mockPlatform, Show()).Times(1);

    Window window(std::move(mockPlatform), nullptr);
    window.Show();
}

TEST(Window_Basic_ShowHide, HideDelegatesToPlatform)
{
    auto mockPlatform = std::make_unique<StrictMock<MockWindowPlatform>>("TestWindow", 1024, 768);
    
    EXPECT_CALL(*mockPlatform, Hide()).Times(1);
    Window window(std::move(mockPlatform), nullptr);
    window.Hide();
}

TEST(Window_Basic_ShowHide, IsVisibleDelegatesToPlatform)
{
    auto mockPlatform = std::make_unique<StrictMock<MockWindowPlatform>>("TestWindow", 1024, 768);
    
    EXPECT_CALL(*mockPlatform, IsVisible()).WillOnce(Return(true));

    Window window(std::move(mockPlatform), nullptr);
    EXPECT_TRUE(window.IsVisible());
}

TEST(Window_Basic_Properties, GetTitleReturnsCorrectValue)
{
    auto mockPlatform = std::make_unique<NiceMock<MockWindowPlatform>>("MyTestWindow", 1024, 768);
    
    Window window(std::move(mockPlatform), nullptr);
    EXPECT_EQ(window.GetTitle(), "MyTestWindow");
}

TEST(Window_Basic_Properties, GetWidthReturnsCorrectValue)
{
    auto mockPlatform = std::make_unique<NiceMock<MockWindowPlatform>>("TestWindow", 1920, 1080);
    
    Window window(std::move(mockPlatform), nullptr);
    EXPECT_EQ(window.GetWidth(), 1920);
}

TEST(Window_Basic_Properties, GetHeightReturnsCorrectValue)
{
    auto mockPlatform = std::make_unique<NiceMock<MockWindowPlatform>>("TestWindow", 1920, 1080);
    
    Window window(std::move(mockPlatform), nullptr);
    EXPECT_EQ(window.GetHeight(), 1080);
}

// ============================================================================
// Window Lifecycle Tests
// ============================================================================

TEST(Window_Lifecycle, DestroyIsIdempotent)
{
    bool callbackInvoked = false;
    auto onDestroy = [&callbackInvoked](Window*) { callbackInvoked = true; };
    
    auto mockPlatform = std::make_unique<NiceMock<MockWindowPlatform>>("TestWindow", 1024, 768);
    
    Window window(std::move(mockPlatform), onDestroy);
    
    window.Destroy();
    EXPECT_TRUE(callbackInvoked);
    EXPECT_TRUE(window.IsClosed());
    
    callbackInvoked = false;
    window.Destroy(); // Second destroy should be no-op
    EXPECT_FALSE(callbackInvoked);
}

TEST(Window_Lifecycle, DestroyCallsOnDestroyCallback)
{
    bool callbackInvoked = false;
    Window* capturedWindow = nullptr;
    
    auto onDestroy = [&](Window* w) {
        callbackInvoked = true;
        capturedWindow = w;
    };
    
    auto mockPlatform = std::make_unique<NiceMock<MockWindowPlatform>>("TestWindow", 1024, 768);
    
    Window window(std::move(mockPlatform), onDestroy);
    
    window.Destroy();
    
    EXPECT_TRUE(callbackInvoked);
    EXPECT_EQ(capturedWindow, &window);
}

TEST(Window_Lifecycle, IsClosedReturnsTrueAfterDestroy)
{
    auto mockPlatform = std::make_unique<NiceMock<MockWindowPlatform>>("TestWindow", 1024, 768);
    
    Window window(std::move(mockPlatform), nullptr);
    
    EXPECT_FALSE(window.IsClosed());
    window.Destroy();
    EXPECT_TRUE(window.IsClosed());
}

TEST(Window_Lifecycle, OperationsAfterDestroyAreSafe)
{
    auto mockPlatform = std::make_unique<StrictMock<MockWindowPlatform>>("TestWindow", 1024, 768);
    
    // Show/Hide should not be called after destroy
    EXPECT_CALL(*mockPlatform, Show()).Times(0);
    EXPECT_CALL(*mockPlatform, Hide()).Times(0);
    EXPECT_CALL(*mockPlatform, IsVisible()).Times(0);
    
    Window window(std::move(mockPlatform), nullptr);
    window.Destroy();
    
    // These should be no-ops
    window.Show();
    window.Hide();
    EXPECT_FALSE(window.IsVisible());
}

// ============================================================================
// Window Event Tests
// ============================================================================

TEST(Window_Events, CloseEventTriggersDestroy)
{
    auto mockPlatform = std::make_unique<NiceMock<MockWindowPlatform>>("TestWindow", 1024, 768);
    void* handle = mockPlatform->GetNativeHandle();
    
    bool destroyed = false;
    auto onDestroy = [&destroyed](Window*) { destroyed = true; };
    
    TestableWindow window(std::move(mockPlatform), onDestroy);
    
    // Dispatch close event with matching handle
    auto closeEvent = std::make_shared<WindowCloseEvent>(handle);
    window.OnEvent(closeEvent);
    
    EXPECT_TRUE(destroyed);
    EXPECT_TRUE(window.IsClosed());
}

TEST(Window_Events, EventWithWrongHandleIsIgnored)
{
    auto mockPlatform = std::make_unique<NiceMock<MockWindowPlatform>>("TestWindow", 1024, 768);
    void* wrongHandle = reinterpret_cast<void*>(0xDEADBEEF);
    
    bool destroyed = false;
    auto onDestroy = [&destroyed](Window*) { destroyed = true; };
    
    TestableWindow window(std::move(mockPlatform), onDestroy);
    
    // Dispatch close event with wrong handle
    auto closeEvent = std::make_shared<WindowCloseEvent>(wrongHandle);
    window.OnEvent(closeEvent);
    
    EXPECT_FALSE(destroyed);
    EXPECT_FALSE(window.IsClosed());
}

TEST(Window_Events, EventWithNullHandleIsProcessed)
{
    auto mockPlatform = std::make_unique<NiceMock<MockWindowPlatform>>("TestWindow", 1024, 768);
    
    bool destroyed = false;
    auto onDestroy = [&destroyed](Window*) { destroyed = true; };
    
    TestableWindow window(std::move(mockPlatform), onDestroy);
    
    // Dispatch close event with null handle (broadcast event)
    auto closeEvent = std::make_shared<WindowCloseEvent>(nullptr);
    window.OnEvent(closeEvent);
    
    EXPECT_TRUE(destroyed);
}

// ============================================================================
// WindowContext Creation Tests
// ============================================================================

TEST(WindowContext_Creation, CreatesStandardWindow)
{
    auto mockPlatform = std::make_unique<StrictMock<MockWindowPlatform>>("TestWindow", 1024, 768);
    auto* platformPtr = mockPlatform.get();
    
    auto mockFactory = std::make_unique<StrictMock<MockWindowPlatformFactory>>();
    EXPECT_CALL(*mockFactory, Create("TestWindow", 1024, 768))
        .WillOnce(Return(testing::ByMove(std::move(mockPlatform))));
    
    auto mockCtxPlatform = std::make_unique<StrictMock<MockWindowContextPlatform>>();
    EXPECT_CALL(*mockCtxPlatform, RegisterWindow(platformPtr)).Times(1);
    EXPECT_CALL(*mockCtxPlatform, UnregisterWindow(platformPtr)).Times(1);
    EXPECT_CALL(*mockCtxPlatform, Shutdown()).Times(1);
    
    auto factory = std::make_unique<MockWindowContextPlatformFactory>(
        std::move(mockCtxPlatform), std::move(mockFactory));
    
    std::unique_ptr<WindowContext> ctx = std::make_unique<WindowContext>(std::move(factory));
    auto window = ctx->CreateWindow("TestWindow", 1024, 768);
    
    ASSERT_NE(window, nullptr);
    EXPECT_EQ(window->GetTitle(), "TestWindow");
    EXPECT_EQ(window->GetWidth(), 1024);
    EXPECT_EQ(window->GetHeight(), 768);

    window.reset();
    ctx.reset();
}

TEST(WindowContext_Creation, CreatesCustomWindow)
{
    auto mockPlatform = std::make_unique<NiceMock<MockWindowPlatform>>("TestWindow", 1024, 768);
    auto* platformPtr = mockPlatform.get();
    
    auto mockFactory = std::make_unique<NiceMock<MockWindowPlatformFactory>>();
    EXPECT_CALL(*mockFactory, Create("TestWindow", 1024, 768))
    .WillOnce(Return(testing::ByMove(std::move(mockPlatform))));
    
    auto mockCtxPlatform = std::make_unique<NiceMock<MockWindowContextPlatform>>();
    EXPECT_CALL(*mockCtxPlatform, RegisterWindow(platformPtr)).Times(1);
    EXPECT_CALL(*mockCtxPlatform, UnregisterWindow(platformPtr)).Times(1);
    
    auto factory = std::make_unique<MockWindowContextPlatformFactory>(
        std::move(mockCtxPlatform), std::move(mockFactory));
    
    WindowContext ctx(std::move(factory));
    auto window = ctx.CreateWindow<TestableWindow>("TestWindow", 1024, 768);
    
    ASSERT_NE(window, nullptr);
}

TEST(WindowContext_Creation, TracksMultipleWindows)
{
    auto mockPlatform1 = std::make_unique<NiceMock<MockWindowPlatform>>("Window1", 800, 600);
    auto mockPlatform2 = std::make_unique<NiceMock<MockWindowPlatform>>("Window2", 1024, 768);
    
    auto mockFactory = std::make_unique<NiceMock<MockWindowPlatformFactory>>();
    EXPECT_CALL(*mockFactory, Create(_, _, _))
        .WillOnce(Return(testing::ByMove(std::move(mockPlatform1))))
        .WillOnce(Return(testing::ByMove(std::move(mockPlatform2))));
    
    auto mockCtxPlatform = std::make_unique<NiceMock<MockWindowContextPlatform>>();
    
    auto factory = std::make_unique<MockWindowContextPlatformFactory>(
        std::move(mockCtxPlatform), std::move(mockFactory));
    
    WindowContext ctx(std::move(factory));
    
    EXPECT_EQ(ctx.GetWindowCount(), 0);
    
    auto w1 = ctx.CreateWindow("Window1", 800, 600);
    EXPECT_EQ(ctx.GetWindowCount(), 1);
    
    auto w2 = ctx.CreateWindow("Window2", 1024, 768);
    EXPECT_EQ(ctx.GetWindowCount(), 2);
}

// ============================================================================
// WindowContext Event Distribution Tests
// ============================================================================

TEST(WindowContext_Events, DistributesCloseEventToWindow)
{
    auto mockPlatform = std::make_unique<NiceMock<MockWindowPlatform>>("TestWindow", 1024, 768);
    void* handle = mockPlatform->GetNativeHandle();

    EXPECT_CALL(*mockPlatform, IsVisible()).WillRepeatedly(Return(true));
    
    auto mockFactory = std::make_unique<NiceMock<MockWindowPlatformFactory>>();
    EXPECT_CALL(*mockFactory, Create(_, _, _))
        .WillOnce(Return(testing::ByMove(std::move(mockPlatform))));
    
    auto mockCtxPlatform = std::make_unique<NiceMock<MockWindowContextPlatform>>();
    auto* rawCtxPlatform = mockCtxPlatform.get();
    
    EXPECT_CALL(*mockCtxPlatform, PollEvents()).Times(1);
    
    auto factory = std::make_unique<MockWindowContextPlatformFactory>(
        std::move(mockCtxPlatform), std::move(mockFactory));
    
    WindowContext ctx(std::move(factory));
    auto window = ctx.CreateWindow<TestableWindow>("TestWindow", 1024, 768);
    
    // Dispatch close event through context platform
    auto closeEvent = std::make_shared<WindowCloseEvent>(handle);
    rawCtxPlatform->DispatchEventForTest(closeEvent);
    
    // PollEvents distributes events to windows
    ctx.PollEvents();
    
    EXPECT_TRUE(window->IsClosed());
}

TEST(WindowContext_Events, PollEventsDelegatesToPlatform)
{
    auto mockCtxPlatform = std::make_unique<StrictMock<MockWindowContextPlatform>>();
    EXPECT_CALL(*mockCtxPlatform, PollEvents()).Times(1);
    EXPECT_CALL(*mockCtxPlatform, Shutdown()).Times(1);
    
    auto mockFactory = std::make_unique<NiceMock<MockWindowPlatformFactory>>();
    
    auto factory = std::make_unique<MockWindowContextPlatformFactory>(
        std::move(mockCtxPlatform), std::move(mockFactory));
    
    WindowContext ctx(std::move(factory));
    ctx.PollEvents();
}

// ============================================================================
// WindowContext Shutdown Tests
// ============================================================================
TEST(WindowContext_Shutdown, DestroysAllWindowsOnShutdown)
{
    auto mockPlatform1 = std::make_unique<NiceMock<MockWindowPlatform>>("Window1", 800, 600);
    auto mockPlatform2 = std::make_unique<NiceMock<MockWindowPlatform>>("Window2", 1024, 768);
    
    auto mockFactory = std::make_unique<NiceMock<MockWindowPlatformFactory>>();
    EXPECT_CALL(*mockFactory, Create(_, _, _))
        .WillOnce(Return(testing::ByMove(std::move(mockPlatform1))))
        .WillOnce(Return(testing::ByMove(std::move(mockPlatform2))));
    
    auto mockCtxPlatform = std::make_unique<NiceMock<MockWindowContextPlatform>>();
    EXPECT_CALL(*mockCtxPlatform, Shutdown()).Times(1);
    
    auto factory = std::make_unique<MockWindowContextPlatformFactory>(
        std::move(mockCtxPlatform), std::move(mockFactory));
    
    WindowContext ctx(std::move(factory));
    auto w1 = ctx.CreateWindow("Window1", 800, 600);
    auto w2 = ctx.CreateWindow("Window2", 1024, 768);
    
    EXPECT_EQ(ctx.GetWindowCount(), 2);
    
    ctx.Shutdown();
    
    EXPECT_EQ(ctx.GetWindowCount(), 0);
    EXPECT_TRUE(w1->IsClosed());
    EXPECT_TRUE(w2->IsClosed());
}

TEST(WindowContext_Shutdown, ShutdownIsIdempotent)
{
    auto mockCtxPlatform = std::make_unique<StrictMock<MockWindowContextPlatform>>();
    EXPECT_CALL(*mockCtxPlatform, Shutdown()).Times(1); // Only once
    
    auto mockFactory = std::make_unique<NiceMock<MockWindowPlatformFactory>>();
    
    auto factory = std::make_unique<MockWindowContextPlatformFactory>(
        std::move(mockCtxPlatform), std::move(mockFactory));
    
    WindowContext ctx(std::move(factory));
    
    ctx.Shutdown();
    ctx.Shutdown(); // Second call should be no-op
}

TEST(WindowContext_Shutdown, DestructorCallsShutdown)
{
    auto mockPlatform = std::make_unique<NiceMock<MockWindowPlatform>>("TestWindow", 1024, 768);
    auto* platformPtr = mockPlatform.get();
    
    auto mockFactory = std::make_unique<NiceMock<MockWindowPlatformFactory>>();
    EXPECT_CALL(*mockFactory, Create(_, _, _))
        .WillOnce(Return(testing::ByMove(std::move(mockPlatform))));
    
    auto mockCtxPlatform = std::make_unique<StrictMock<MockWindowContextPlatform>>();
    EXPECT_CALL(*mockCtxPlatform, Shutdown()).Times(1); // Called by destructor
    EXPECT_CALL(*mockCtxPlatform, RegisterWindow(platformPtr)).Times(1);
    EXPECT_CALL(*mockCtxPlatform, UnregisterWindow(platformPtr)).Times(1);
    
    auto factory = std::make_unique<MockWindowContextPlatformFactory>(
        std::move(mockCtxPlatform), std::move(mockFactory));
    
    {
        WindowContext ctx(std::move(factory));
        auto window = ctx.CreateWindow("TestWindow", 1024, 768);
    } // Destructor calls Shutdown
}

TEST(WindowContext_Shutdown, ManualWindowDestroyReducesCount)
{
    auto mockPlatform = std::make_unique<NiceMock<MockWindowPlatform>>("TestWindow", 1024, 768);
    
    auto mockFactory = std::make_unique<NiceMock<MockWindowPlatformFactory>>();
    EXPECT_CALL(*mockFactory, Create(_, _, _))
        .WillOnce(Return(testing::ByMove(std::move(mockPlatform))));
    
    auto mockCtxPlatform = std::make_unique<NiceMock<MockWindowContextPlatform>>();
    
    auto factory = std::make_unique<MockWindowContextPlatformFactory>(
        std::move(mockCtxPlatform), std::move(mockFactory));
    
    WindowContext ctx(std::move(factory));
    auto window = ctx.CreateWindow("TestWindow", 1024, 768);
    
    EXPECT_EQ(ctx.GetWindowCount(), 1);
    
    window->Destroy();
    
    EXPECT_EQ(ctx.GetWindowCount(), 0);
}