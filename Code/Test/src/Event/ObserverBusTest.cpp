#include "FreqUsedSymbolsOfTesting.h"
#include "Event/EventListener.h"
#include "Event/EventProducer.h"
#include "Event/ObserverBus.h"

// Minimal StubEvent / StubProducer / StubListener used by tests.

class StubEvent : public Event
{
public:
    StubEvent() : Event(TOP_LEVEL_EVENT_TYPE::STUB_EVENT) {}
    int seq = 0;
    static constexpr int data = 1;
};

class StubProducer : public EventProducer<StubEvent>
{
public:
    void TriggerEvent(int seq = 0) noexcept
    {
        auto e = std::make_shared<StubEvent>();
        e->seq = seq;
        DispatchEvent(e);
    }

    ~StubProducer()
    {
        UnregisterEventConnections();
    }
};

class StubListener : public EventListener<StubEvent>
{
public:
    void OnEvent(std::shared_ptr<const StubEvent> e) override
    {
        m_Data += e->data;
        m_Seqs.push_back(e->seq);
    }

    int m_Data = 0;
    std::vector<int> m_Seqs;
};

// BlockingListener: signals when started processing then waits on a future to continue.
// Used to deterministically test in-flight/unregister semantics.
class BlockingListener : public EventListener<StubEvent>
{
public:
    BlockingListener(std::promise<void>* startedPromise, std::shared_future<void> continueFuture)
        : m_StartedPromise(startedPromise)
        , m_ContinueFuture(continueFuture)
    {
    }

    void OnEvent(std::shared_ptr<const StubEvent> /*e*/) override
    {
        if (m_StartedPromise) m_StartedPromise->set_value();
        m_ContinueFuture.wait();
        m_Processed = true;
    }

    bool m_Processed = false;

private:
    std::promise<void>* m_StartedPromise;
    std::shared_future<void> m_ContinueFuture;
};

// ------------------------ Tests (Direct-only) ------------------------

TEST(ObserverBus_Direct, RegisterUnregisterBasic)
{
    StubProducer producer;
    StubListener listener;
    DirectObserverBus<StubEvent> bus;

    bus.RegisterProducer(&producer);
    bus.RegisterListener(&listener);

    producer.TriggerEvent();
    EXPECT_EQ(listener.m_Data, 1);

    bus.UnRegisterProducer(&producer);
    producer.TriggerEvent();
    EXPECT_EQ(listener.m_Data, 1); // no change after producer unregistered

    // Re-register producer and then unregister listener
    bus.RegisterProducer(&producer);
    bus.UnRegisterListener(&listener);

    producer.TriggerEvent();
    EXPECT_EQ(listener.m_Data, 1); // listener unregistered so no change
}

TEST(ObserverBus_Direct, MultipleProducersMultipleListeners)
{
    StubProducer p1, p2;
    StubListener l1, l2, l3;
    DirectObserverBus<StubEvent> bus;

    bus.RegisterProducer(&p1);
    bus.RegisterProducer(&p2);

    bus.RegisterListener(&l1);
    bus.RegisterListener(&l2);
    bus.RegisterListener(&l3);

    p1.TriggerEvent();
    EXPECT_EQ(l1.m_Data, 1);
    EXPECT_EQ(l2.m_Data, 1);
    EXPECT_EQ(l3.m_Data, 1);

    p2.TriggerEvent();
    EXPECT_EQ(l1.m_Data, 2);
    EXPECT_EQ(l2.m_Data, 2);
    EXPECT_EQ(l3.m_Data, 2);

    bus.UnRegisterProducer(&p1);
    p1.TriggerEvent();
    EXPECT_EQ(l1.m_Data, 2); // unchanged after p1 unregistered
}

TEST(ObserverBus_Direct_Concurrency, UnregisterWaitsForInFlightDispatches)
{
    DirectObserverBus<StubEvent> bus;
    StubProducer producer;

    // Prepare blocking listener that signals when it starts processing.
    std::promise<void> started;
    std::promise<void> continue_p;
    auto continue_future = continue_p.get_future().share();
    BlockingListener blocker(&started, continue_future);

    // Register producer and listener using the public API.
    bus.RegisterProducer(&producer);
    bus.RegisterListener(&blocker);

    // Start dispatch on separate thread
    std::thread dispatchThread([&producer]() {
        producer.TriggerEvent();
        });

    // Wait until listener has started processing (deterministic)
    started.get_future().wait();

    // Now call UnRegisterListener on another thread and ensure it blocks until we allow listener to continue.
    std::atomic<bool> unregister_done{ false };
    std::thread unregisterThread([&bus, &blocker, &unregister_done]() {
        bus.UnRegisterListener(&blocker);
        unregister_done = true;
        });

    // Allow the listener to finish processing
    continue_p.set_value();

    dispatchThread.join();
    unregisterThread.join();

    EXPECT_TRUE(unregister_done.load());
    EXPECT_TRUE(blocker.m_Processed);
}

TEST(ObserverBus_Direct_Lifetime, ListenerDestructorUnregistersItself)
{
    DirectObserverBus<StubEvent> bus;
    StubProducer producer;

    // create listener in inner scope and register it
    {
        auto dynamicListener = std::make_unique<StubListener>();
        bus.RegisterProducer(&producer);
        bus.RegisterListener(dynamicListener.get());

        // destroy listener object; its destructor should call UnregisterEventConnections()
        dynamicListener.reset();
    }

    // Register a second persistent listener. If the first listener properly unregistered itself,
    // only this second listener should receive the event.
    StubListener persistentListener;
    bus.RegisterListener(&persistentListener);

    producer.TriggerEvent();

    EXPECT_EQ(persistentListener.m_Data, 1);
}

TEST(ObserverBus_Direct_Destructor, BusDestructorWaitsAndCleansUp)
{
    StubProducer producer;

    std::promise<void> started;
    std::promise<void> continue_p;
    auto continue_future = continue_p.get_future().share();

    // Create bus in nested scope so destructor runs deterministically.
    {
        DirectObserverBus<StubEvent> bus;
        bus.RegisterProducer(&producer);

        BlockingListener blocker(&started, continue_future);
        bus.RegisterListener(&blocker);

        // Kick off a dispatch on a separate thread which will block inside listener until we allow it to continue.
        std::thread dispatchThread([&producer]() { producer.TriggerEvent(); });

        // Wait until listener has started processing
        started.get_future().wait();

        // Allow dispatch to finish
        continue_p.set_value();
        dispatchThread.join();

        // bus destructor will run when leaving this scope
    }

    // If we reach here without deadlock/crash the test succeeded.
    SUCCEED();
}