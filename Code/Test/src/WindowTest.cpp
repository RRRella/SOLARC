#include "FreqUsedSymbolsOfTesting.h"
#include "Window.h"

class MockWindowPlatform : public WindowPlatform
{
public:
	MOCK_METHOD(bool , PeekNextMessage, () , ());

	MOCK_METHOD(void , ProcessMessage, (), ());

	MOCK_METHOD(void, Update, (), ());

	void GetMessageInternal()
	{
		m_MessageCounter++;
	}
	void ProcessMessageInternal()
	{
		m_ProcessedMessage += m_MessageCounter;
	}
	void UpdateInternal()
	{
		m_UpdateCounter++;
	}

	uint64_t GetProcessedMessageState() const { return m_ProcessedMessage; }
	uint64_t GetUpdateState() const { return m_UpdateCounter; }

private:
	uint64_t m_MessageCounter = 0;

	uint64_t m_ProcessedMessage = 0;

	uint64_t m_UpdateCounter = 0;
};

class FakeWindow : public Window
{
public:
	FakeWindow(std::unique_ptr<WindowPlatform> platform) :
		Window(std::move(platform))
	{
	}
	~FakeWindow() = default;
};

class WindowTest : public ::testing::Test
{
protected:

	WindowTest()
	{
		platform = std::make_unique<MockWindowPlatform>();
	}

	~WindowTest()
	{

	}

	void SetUp() override
	{

	}

	void TearDown() override
	{

	}
	
	std::unique_ptr<Window> window;
	std::unique_ptr<MockWindowPlatform> platform;
};

TEST_F(WindowTest, UpdatesMessegesUsingItsPipe)
{
	MockWindowPlatform* platformPtr = platform.get();

	testing::Mock::AllowLeak(platformPtr);

	EXPECT_CALL(*platformPtr, PeekNextMessage()).Times(3).WillRepeatedly(testing::Invoke(
		[&]()->bool 
		{
			platformPtr->GetMessageInternal();
			return true; 
		}));

	EXPECT_CALL(*platformPtr, ProcessMessage()).Times(3).WillRepeatedly(testing::Invoke(
		[&]()->void
		{
			platformPtr->ProcessMessageInternal();
		}));

	EXPECT_CALL(*platformPtr, Update()).Times(3).WillRepeatedly(testing::Invoke(
		[&]()->void
		{
			platformPtr->UpdateInternal();
		}));

	window = std::make_unique<FakeWindow>(std::move(platform));

	for(int i = 0 ; i < 3;++i)
		window->Update();

	ASSERT_EQ(static_cast<MockWindowPlatform*>(window->GetMessagePipe())->GetProcessedMessageState() , static_cast<uint64_t>(6));
	ASSERT_EQ(static_cast<MockWindowPlatform*>(window->GetMessagePipe())->GetUpdateState() , static_cast<uint64_t>(3));
}
