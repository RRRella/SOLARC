#include <gtest/gtest.h>
#include <memory>
#include <string>

// Include the headers under test
#include "Window/Window.h"
#include "Event/WindowEvent.h"
#include "Input/InputFrame.h"
#include "Window/MockWindowPlatform.h"


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

    // Request Show
    window->Show();
    window->Update();

    EXPECT_TRUE(window->IsVisible());
}

TEST_F(WindowTest, Hide_CallsPlatformHide_AndUpdatesStateViaEvent)
{
    MockWindowPlatform* mock = nullptr;
    auto window = CreateWindowAndMock(&mock, nullptr);

    window->Show();
    window->Update();
    ASSERT_TRUE(window->IsVisible());

    // Request Hide
    window->Hide();
    window->Update();

    EXPECT_FALSE(window->IsVisible());
}

TEST_F(WindowTest, Resize_CallsPlatform_AndUpdatesStateViaEvent)
{
    MockWindowPlatform* mock = nullptr;
    auto window = CreateWindowAndMock(&mock, nullptr);

    // Request Resize
    window->Resize(1280, 720);
    window->Update();

    // Verify state update
    EXPECT_EQ(window->GetWidth(), 1280);
    EXPECT_EQ(window->GetHeight(), 720);
}

TEST_F(WindowTest, MinimizeCommand_CallsPlatform_AndUpdatesState)
{
    MockWindowPlatform* mock = nullptr;
    auto window = CreateWindowAndMock(&mock, nullptr);

    EXPECT_FALSE(window->IsMinimized());

    // Request Minimize via Window API
    window->Minimize();
    window->Update();

    EXPECT_TRUE(window->IsMinimized());
}

TEST_F(WindowTest, RestoreCommand_CallsPlatform_AndUpdatesState)
{
    MockWindowPlatform* mock = nullptr;
    auto window = CreateWindowAndMock(&mock, nullptr);

    // Set to minimized first
    window->Minimize();
    window->Update();
    ASSERT_TRUE(window->IsMinimized());

    // Request Restore
    window->Restore();
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