// InputSystemUnitTest.cpp
#include <gtest/gtest.h>
#include "Window/Window.h"
#include "Input/InputFrame.h"
#include "Event/InputEvent.h"
#include "Window/MockWindowPlatform.h"

class InputSystemUnitTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto platform = std::make_unique<MockWindowPlatform>("InputTest", 640, 480);
        m_Mock = platform.get();
        m_Window = std::make_shared<TestWindow>(std::move(platform));
    }

    MockWindowPlatform* m_Mock = nullptr;
    std::shared_ptr<TestWindow> m_Window;
};

TEST_F(InputSystemUnitTest, Keyboard_Polling_DetectsTransitions)
{
    // Inject 'W' press
    m_Mock->MutableThisFrameInput().keyTransitions.push_back({ 0x11, true, false }); // scancode 0x11 = W
    m_Window->Update();

    EXPECT_TRUE(m_Window->IsKeyDown(KeyCode::W));
    EXPECT_TRUE(m_Window->WasKeyJustPressed(KeyCode::W));
    EXPECT_EQ(m_Window->GetKeyRepeatCount(KeyCode::W), 1);

    // Next frame: still down, not "just pressed"
    m_Mock->ResetThisFrameInput();
    m_Window->Update();

    EXPECT_TRUE(m_Window->IsKeyDown(KeyCode::W));
    EXPECT_FALSE(m_Window->WasKeyJustPressed(KeyCode::W));
    EXPECT_EQ(m_Window->GetKeyRepeatCount(KeyCode::W), 1);
}

TEST_F(InputSystemUnitTest, Keyboard_FocusLoss_ClearsAllKeys)
{
    // Press W
    m_Mock->MutableThisFrameInput().keyTransitions.push_back({ 0x11, true, false });
    m_Window->Update();
    EXPECT_TRUE(m_Window->IsKeyDown(KeyCode::W));

    // Simulate focus loss
    m_Mock->OnFocusLost();
    m_Mock->ResetThisFrameInput(); // next frame
    m_Window->Update();

    EXPECT_FALSE(m_Window->IsKeyDown(KeyCode::W));
}

TEST_F(InputSystemUnitTest, Mouse_Delta_Accumulates)
{
    // First move
    m_Mock->MutableThisFrameInput().mouseX = 100;
    m_Mock->MutableThisFrameInput().mouseY = 50;
    m_Mock->MutableThisFrameInput().mouseDeltaX = 10;
    m_Mock->MutableThisFrameInput().mouseDeltaY = 5;
    m_Window->Update();

    EXPECT_EQ(m_Window->GetMouseX(), 100);
    EXPECT_EQ(m_Window->GetMouseY(), 50);
    EXPECT_EQ(m_Window->GetMouseDeltaX(), 10);
    EXPECT_EQ(m_Window->GetMouseDeltaY(), 5);

    // Next frame: delta resets
    m_Mock->ResetThisFrameInput();
    m_Window->Update();

    EXPECT_EQ(m_Window->GetMouseDeltaX(), 0);
    EXPECT_EQ(m_Window->GetMouseDeltaY(), 0);
}

TEST_F(InputSystemUnitTest, MouseButton_Events_Emitted)
{
    class Listener : public EventListener<WindowEvent> {
    public:
        using EventListener<WindowEvent>::ProcessEvents;
        bool down = false, up = false;
        MouseButton btn = MouseButton::Left;
    protected:
        void OnEvent(const std::shared_ptr<const WindowEvent>& e) override {
            if (e->GetWindowEventType() == WindowEvent::TYPE::INPUT) {
                auto inputEvent = std::static_pointer_cast<const WindowInputEvent>(e);
                if (inputEvent->GetWindowInputEventType() == WindowInputEvent::TYPE::MOUSE_BUTTON_DOWN) {
                    auto evt = std::static_pointer_cast<const MouseButtonDownEvent>(e);
                    down = true;
                    btn = evt->GetButton();
                }
            }
        }
    };

    ObserverBus<WindowEvent> bus;
    Listener listener;
    bus.RegisterProducer(m_Window.get());
    bus.RegisterListener(&listener);

    // Inject left mouse down
    m_Mock->MutableThisFrameInput().mouseButtonTransitions.push_back({ MouseButton::Left, true });
    m_Window->Update();

    bus.Communicate();
    listener.ProcessEvents();

    EXPECT_TRUE(listener.down);
    EXPECT_EQ(listener.btn, MouseButton::Left);
}