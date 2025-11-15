#include "FreqUsedSymbolsOfTesting.h"
#include "Event/EventListener.h"
#include "Event/EventProducer.h"
#include "Event/ObserverBus.h"

// Minimal StubEvent / StubProducer / StubListener used by tests.

class StubEvent : public Event
{
public:
    StubEvent() : Event(Event::TYPE::STUB_EVENT) {}
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
        // No need to call UnregisterEventConnections() anymore - it's called automatically in destructor
    }
};

class StubListener : public EventListener<StubEvent>
{
public:
    void OnEvent(const std::shared_ptr<const StubEvent>& e) override
    {
        m_Data += e->data;
        m_Seqs.push_back(e->seq);
    }

    int m_Data = 0;
    std::vector<int> m_Seqs;
};

// For queued listeners we will implement a ListenThroughQueue helper so tests can pop events
class QueueListener : public EventListener<StubEvent>
{
public:
    // The listener won't be called via OnEvent for queued mode; instead, consumers call this
    // helper to pop events from the internal queue and process them.
    void ListenThroughQueueAndConsumeOnce()
    {
        auto e = m_EventQueue.WaitOnNext();
        if (e) {
            OnEvent(e);
        }
    }

    // Provide OnEvent too so direct registration can use it in other tests (not used here).
    void OnEvent(const std::shared_ptr<const StubEvent>& e) override
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

    void OnEvent(const std::shared_ptr<const StubEvent>& e) override
    {
        if (m_StartedPromise) m_StartedPromise->set_value();
        m_ContinueFuture.wait();
        m_Processed = true;
    }

    bool m_Processed = false;

protected:
    std::promise<void>* m_StartedPromise;
    std::shared_future<void> m_ContinueFuture;
};

// Blocking queue listener for concurrency tests; it will wait until signaled to consume
class BlockingQueueListener : public BlockingListener
{
public:
    BlockingQueueListener(std::promise<void>* startedPromise, std::shared_future<void> continueFuture)
        : BlockingListener(startedPromise, continueFuture)
    {
    }

    // Consumer will wait on queue, signal started, then wait on continueFuture before consuming.
    void ListenThroughQueueAndBlockOnce()
    {
        auto e = m_EventQueue.WaitOnNext();
        if (m_StartedPromise) m_StartedPromise->set_value();
        m_ContinueFuture.wait();

        OnEvent(e);
    }

    void OnEvent(const std::shared_ptr<const StubEvent>& e) override
    {
        if (e)
            m_Processed = true;
    }
};

// ------------------------ Tests ------------------------

TEST(ObserverBus, RegisterUnregisterBasic)
{
    StubProducer producer;
    QueueListener listener;
    ObserverBus<StubEvent> bus;

    bus.RegisterProducer(&producer);
    bus.RegisterListener(&listener);

    producer.TriggerEvent();

    // nothing processed until bus communicates and listener pops from queue
    EXPECT_EQ(listener.m_Data, 0);

    // First, communicate to move events from bus queue to listener queue
    bus.Communicate();
    // Then consume queued event
    listener.ListenThroughQueueAndConsumeOnce();
    EXPECT_EQ(listener.m_Data, 1);

    bus.UnregisterProducer(&producer);
    producer.TriggerEvent();
    bus.Communicate(); // Try to communicate the event that shouldn't reach listener
    EXPECT_EQ(listener.m_Data, 1); // no change after producer unregistered

    // Re-register producer and then unregister listener
    bus.RegisterProducer(&producer);
    bus.UnregisterListener(&listener);

    producer.TriggerEvent();
    bus.Communicate();
    EXPECT_EQ(listener.m_Data, 1); // listener unregistered so no change
}

TEST(ObserverBus, MultipleProducersMultipleListeners)
{
    StubProducer p1, p2;
    QueueListener l1, l2, l3;
    ObserverBus<StubEvent> bus;

    bus.RegisterProducer(&p1);
    bus.RegisterProducer(&p2);

    bus.RegisterListener(&l1);
    bus.RegisterListener(&l2);
    bus.RegisterListener(&l3);

    p1.TriggerEvent();
    p2.TriggerEvent();

    // Communicate to move events from bus to all listeners
    bus.Communicate();

    // Each listener should have two queued events to consume
    l1.ListenThroughQueueAndConsumeOnce();
    l1.ListenThroughQueueAndConsumeOnce();
    l2.ListenThroughQueueAndConsumeOnce();
    l2.ListenThroughQueueAndConsumeOnce();
    l3.ListenThroughQueueAndConsumeOnce();
    l3.ListenThroughQueueAndConsumeOnce();

    EXPECT_EQ(l1.m_Data, 2);
    EXPECT_EQ(l2.m_Data, 2);
    EXPECT_EQ(l3.m_Data, 2);
}

TEST(ObserverBus_Concurrency, ListenerConsumesFromQueue)
{
    ObserverBus<StubEvent> bus;
    StubProducer producer;

    std::promise<void> started;
    std::promise<void> continue_p;
    auto continue_future = continue_p.get_future().share();
    BlockingQueueListener blocker(&started, continue_future);

    bus.RegisterProducer(&producer);
    bus.RegisterListener(&blocker);

    // Producer pushes an event (goes to bus queue)
    producer.TriggerEvent();

    // Communicate to move event from bus to listener queue
    bus.Communicate();

    // Start consumer thread which will pop and then block until signaled
    std::thread consumerThread([&blocker]() {
        blocker.ListenThroughQueueAndBlockOnce();
        });

    // Wait until consumer started processing (popped)
    started.get_future().wait();

    // Now call UnRegisterListener on another thread — for queued semantics Unregister will not
    // wait for items already enqueued to be consumed with the current design.
    std::atomic<bool> unregister_done{ false };
    std::thread unregisterThread([&bus, &blocker, &unregister_done]() {
        bus.UnregisterListener(&blocker);
        unregister_done = true;
        });

    // Allow consumer to finish
    continue_p.set_value();

    consumerThread.join();
    unregisterThread.join();

    // consumer should have consumed the queued event, and unregister should have completed.
    EXPECT_TRUE(unregister_done.load());
    EXPECT_TRUE(blocker.m_Processed);
}

TEST(ObserverBus_Lifetime, ListenerDestructorUnregistersItself)
{
    ObserverBus<StubEvent> bus;
    StubProducer producer;

    // create listener in inner scope and register it
    {
        auto dynamicListener = std::make_unique<QueueListener>();
        bus.RegisterProducer(&producer);
        bus.RegisterListener(dynamicListener.get());

        // destroy listener object; its destructor should call UnregisterEventConnections()
        dynamicListener.reset();
    }

    // Register a second persistent listener. If the first listener properly unregistered itself,
    // only this second listener should receive the event.
    QueueListener persistentListener;
    bus.RegisterListener(&persistentListener);

    producer.TriggerEvent();
    bus.Communicate(); // Move event to listener queues

    // Consume the event
    persistentListener.ListenThroughQueueAndConsumeOnce();
    EXPECT_EQ(persistentListener.m_Data, 1);
}

TEST(ObserverBus_Destructor, BusDestructorWaitsAndCleansUp)
{
    StubProducer producer;

    std::promise<void> started;
    std::promise<void> continue_p;
    auto continue_future = continue_p.get_future().share();

    // Create bus in nested scope so destructor runs deterministically.
    {
        ObserverBus<StubEvent> bus;
        bus.RegisterProducer(&producer);

        BlockingQueueListener blocker(&started, continue_future);
        bus.RegisterListener(&blocker);

        // Producer pushes an event (goes to bus queue)
        producer.TriggerEvent();

        // Communicate to move event to listener queue
        bus.Communicate();

        // Kick off a consumer on a separate thread which will pop and block until we allow it to continue.
        std::thread consumerThread([&blocker]() {
            blocker.ListenThroughQueueAndBlockOnce();
            });

        // Wait until listener has started processing
        started.get_future().wait();

        // Allow consumer to finish
        continue_p.set_value();
        consumerThread.join();

        // bus destructor will run when leaving this scope
    }

    // If we reach here without deadlock/crash the test succeeded.
    SUCCEED();
}