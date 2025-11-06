//#include "FreqUsedSymbolsOfTesting.h"
//#include "Window.h"
//#include "Event/EventFakes.h"
//
//class MockWindowPlatform : public WindowPlatform
//{
//public:
//
//	using WindowPlatform::m_CallBack;
//
//	MockWindowPlatform(const WindowEventCallBack& callBack , const WindowMetaData& metaData)
//		: WindowPlatform(callBack)
//	{
//
//	}
//
//	MOCK_METHOD(void , PollEvents, (), ());
//
//	MOCK_METHOD(bool, Show, (), ());
//
//	MOCK_METHOD(bool, Hide, (), ());
//
//	void PollEventsInternal()
//	{
//		m_NumberOfPollingAttempt++;
//	}
//	bool ShowInternal()
//	{
//		bool prevState = m_ShowState;
//		m_ShowState = true;
//		return prevState;
//	}
//	bool HideInternal()
//	{
//		bool prevState = m_ShowState;
//		m_ShowState = false;
//		return prevState;
//	}
//
//	uint64_t GetPolledEventState() const { return m_NumberOfPollingAttempt; }
//	bool GetShowState() const { return m_ShowState; }
//
//private:
//	uint64_t m_NumberOfPollingAttempt = 0;
//
//	bool m_ShowState = false;
//};
//
//class MockWindowPlatformFactory : public WindowPlatformFactory
//{
//public:
//	MockWindowPlatformFactory() = default;
//	~MockWindowPlatformFactory() = default;
//
//	MOCK_METHOD(std::unique_ptr<WindowPlatform>, Create, (const WindowEventCallBack& , const WindowMetaData&), ());
//
//	//the platform has to get mocked too
//	//but we have to create it inside the window
//	//we need it's callback object passed into platform
//	WindowPlatform* platformPtr;
//};
//
//class WindowTest : public ::testing::Test
//{
//protected:
//
//	WindowTest()
//	{
//		platformFactory = std::make_shared<MockWindowPlatformFactory>();
//	}
//
//	~WindowTest()
//	{
//
//	}
//
//	void SetUp() override
//	{
//		EXPECT_CALL(*platformFactory, Create(testing::_,
//			testing::AllOf(
//				testing::Field(&WindowMetaData::width, 1920),
//				testing::Field(&WindowMetaData::height, 1080))
//		)
//		).WillOnce(testing::Invoke(
//			[&](const WindowEventCallBack& callBack, const WindowMetaData& metaData)->std::unique_ptr<WindowPlatform>
//			{
//				std::unique_ptr<MockWindowPlatform> platform = std::make_unique<MockWindowPlatform>(callBack, metaData);
//				platformFactory->platformPtr = platform.get();
//				return std::move(platform);
//			}));
//
//		window = std::make_unique<Window>(windowData, platformFactory);
//
//		platformPtr = static_cast<MockWindowPlatform*>(platformFactory->platformPtr);
//	}
//
//	void TearDown() override
//	{
//
//	}
//	
//	std::unique_ptr<Window> window;
//	std::shared_ptr<MockWindowPlatformFactory> platformFactory;
//	MockWindowPlatform* platformPtr;
//	WindowMetaData windowData = { "Test Window", 0 , 0 , 1920 , 1080 };
//};
//
//TEST_F(WindowTest, PassesPlatformSpecificCallsToItsInternalImpl)
//{
//	EXPECT_CALL(*platformPtr, PollEvents()).WillOnce(testing::Invoke(
//		[&]()->void
//		{
//			platformPtr->PollEventsInternal();
//		}));
//
//	EXPECT_CALL(*platformPtr, Show()).WillOnce(testing::Invoke(
//		[&]()->bool
//		{
//			return platformPtr->ShowInternal();
//		}));
//
//	EXPECT_CALL(*platformPtr, Hide()).WillOnce(testing::Invoke(
//		[&]()->bool
//		{
//			return platformPtr->HideInternal();
//		}));
//
//	ASSERT_EQ(static_cast<MockWindowPlatform*>(platformPtr)->GetPolledEventState(), static_cast<uint64_t>(0));
//	ASSERT_EQ(static_cast<MockWindowPlatform*>(platformPtr)->GetShowState(), false);
//
//	window->PollEvents();
//	window->Show();
//
//	ASSERT_EQ(static_cast<MockWindowPlatform*>(platformPtr)->GetPolledEventState() , static_cast<uint64_t>(1));
//	ASSERT_EQ(static_cast<MockWindowPlatform*>(platformPtr)->GetShowState(), true);
//
//	window->Hide();
//
//	ASSERT_EQ(static_cast<MockWindowPlatform*>(platformPtr)->GetShowState(), false);
//}
//
//
//#ifdef WIN32
//
//#include "WindowsWindowPlatform.h"
//
//class FakeWindowsWindowPlatformFactory : public WindowPlatformFactory
//{
//public:
//	FakeWindowsWindowPlatformFactory() = default;
//	~FakeWindowsWindowPlatformFactory() = default;
//
//	std::unique_ptr<WindowPlatform> Create(const WindowEventCallBack& callBack, const WindowMetaData& metaData) override
//	{
//		std::unique_ptr<WindowsWindowPlatform> platform = std::make_unique<WindowsWindowPlatform>(callBack, metaData);
//		platformPtr = platform.get();
//		return std::move(platform);
//	}
//
//	//the platform has to get mocked too
//	//but we have to create it inside the window
//	//we need it's callback object passed into platform
//	WindowsWindowPlatform* platformPtr;
//};
//
//class WindowsWindowIntegration : public ::testing::Test
//{
//protected:
//
//	WindowsWindowIntegration()
//	{
//		platformFactory = std::make_shared<FakeWindowsWindowPlatformFactory>();
//	}
//
//	~WindowsWindowIntegration()
//	{
//
//	}
//
//	void SetUp() override
//	{
//		window = std::make_unique<Window>(windowData, platformFactory);
//
//		platformPtr = platformFactory->platformPtr;
//	}
//
//	void TearDown() override
//	{
//
//	}
//
//	std::unique_ptr<Window> window;
//	std::shared_ptr<FakeWindowsWindowPlatformFactory> platformFactory;
//	WindowsWindowPlatform* platformPtr;
//	WindowMetaData windowData = { "Test Window", 0 , 0 , 1920 , 1080 };
//};
//
//TEST_F(WindowsWindowIntegration, DispatchedEventsGoThroughEventCommunicationSystem)
//{
//	FakeEventCell eventCell;
//	FakeEventComponent otherEventComponent;
//
//	eventCell.RegisterProducer(*window.get());
//	eventCell.RegisterConsumer(otherEventComponent);
//
//	SendMessage(platformPtr->GetHWND() , WM_SIZE , SIZE_RESTORED ,  MAKELPARAM( 1920 , 1080 ));
//
//	auto resizeEvent = std::static_pointer_cast<const WindowResizeEvent>(otherEventComponent.ReturnEvent());
//
//	ASSERT_NE(resizeEvent, nullptr);
//
//	ASSERT_EQ(resizeEvent->GetWidth() , 1920);
//	ASSERT_EQ(resizeEvent->GetHeight(), 1080);
//}
//
//#endif
