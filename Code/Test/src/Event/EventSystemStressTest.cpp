#include "FreqUsedSymbolsOfTesting.h"
#include "Event/Event.h"
#include "Event/EventBus.h"
#include "Event/EventProducer.h"
#include "Event/EventListener.h"
#include "Event/ObserverBus.h"
#include <atomic>
#include <thread>
#include <vector>
#include <chrono>
#include <barrier>

// ============================================================================
// Test Event Types
// ============================================================================

class TestEvent : public Event
{
public:
    enum class TYPE {
        SIMPLE,
        WITH_DATA
    };

    TestEvent(TYPE type, int value = 0)
        : Event(Event::TYPE::STUB_EVENT)
        , m_TestEventType(type)
        , m_Value(value)
    {
    }

    TYPE GetTestEventType() const { return m_TestEventType; }
    int GetValue() const { return m_Value; }

private:
    TYPE m_TestEventType;
    int m_Value;
};

class SimpleTestEvent : public TestEvent
{
public:
    SimpleTestEvent() : TestEvent(TYPE::SIMPLE) {}
};

class DataTestEvent : public TestEvent
{
public:
    explicit DataTestEvent(int value) : TestEvent(TYPE::WITH_DATA, value) {}
};

// ============================================================================
// Test Fixtures
// ============================================================================

class TestProducer : public EventProducer<TestEvent>
{
public:
    void EmitEvent(std::shared_ptr<const TestEvent> event)
    {
        DispatchEvent(event);
    }

    void EmitSimple()
    {
        EmitEvent(std::make_shared<SimpleTestEvent>());
    }

    void EmitData(int value)
    {
        EmitEvent(std::make_shared<DataTestEvent>(value));
    }
};

class TestListener : public EventListener<TestEvent>
{
public:
    TestListener() = default;

    void OnEvent(const std::shared_ptr<const TestEvent>& e) override
    {
        m_EventsReceived.fetch_add(1, std::memory_order_relaxed);

        if (e->GetTestEventType() == TestEvent::TYPE::WITH_DATA)
        {
            auto dataEvent = std::static_pointer_cast<const DataTestEvent>(e);
            m_TotalValue.fetch_add(dataEvent->GetValue(), std::memory_order_relaxed);
        }

        if (m_Callback)
        {
            m_Callback(e);
        }
    }

    void ProcessEvents()
    {
        std::optional<std::shared_ptr<const TestEvent>> event;
        while (event = m_EventQueue.TryNext())
        {
            OnEvent(event.value());
        }
    }

    int GetEventsReceived() const
    {
        return m_EventsReceived.load(std::memory_order_acquire);
    }

    int GetTotalValue() const
    {
        return m_TotalValue.load(std::memory_order_acquire);
    }

    void SetCallback(std::function<void(std::shared_ptr<const TestEvent>)> cb)
    {
        m_Callback = std::move(cb);
    }

    EventQueue<TestEvent>& GetEventQueue() { return m_EventQueue; }

private:
    std::atomic<int> m_EventsReceived{ 0 };
    std::atomic<int> m_TotalValue{ 0 };
    std::function<void(std::shared_ptr<const TestEvent>)> m_Callback;
};

// ============================================================================
// Basic Functionality Tests
// ============================================================================

TEST(EventSystem, ProducerCanDispatchToListener)
{
    ObserverBus<TestEvent> bus;
    TestProducer producer;
    TestListener listener;

    bus.RegisterProducer(&producer);
    bus.RegisterListener(&listener);

    producer.EmitSimple();
    bus.Communicate();
    listener.ProcessEvents();

    ASSERT_EQ(listener.GetEventsReceived(), 1);
}

TEST(EventSystem, MultipleListenersReceiveSameEvent)
{
    ObserverBus<TestEvent> bus;
    TestProducer producer;
    TestListener listener1, listener2, listener3;

    bus.RegisterProducer(&producer);
    bus.RegisterListener(&listener1);
    bus.RegisterListener(&listener2);
    bus.RegisterListener(&listener3);

    producer.EmitSimple();
    bus.Communicate();

    listener1.ProcessEvents();
    listener2.ProcessEvents();
    listener3.ProcessEvents();

    ASSERT_EQ(listener1.GetEventsReceived(), 1);
    ASSERT_EQ(listener2.GetEventsReceived(), 1);
    ASSERT_EQ(listener3.GetEventsReceived(), 1);
}TEST(EventSystem, EventDataIsPreserved)
{
    ObserverBus<TestEvent> bus;
    TestProducer producer;
    TestListener listener;

    bus.RegisterProducer(&producer);
    bus.RegisterListener(&listener);

    for (int i = 1; i <= 10; ++i)
    {
        producer.EmitData(i);
    }

    bus.Communicate();
    listener.ProcessEvents();

    ASSERT_EQ(listener.GetEventsReceived(), 10);
    ASSERT_EQ(listener.GetTotalValue(), 55); // Sum of 1..10
}

TEST(EventSystem, ListenerCanUnregisterSafely)
{
    ObserverBus<TestEvent> bus;
    TestProducer producer;
    auto listener = std::make_unique<TestListener>();

    bus.RegisterProducer(&producer);
    bus.RegisterListener(listener.get());

    producer.EmitSimple();
    bus.Communicate();
    listener->ProcessEvents();

    ASSERT_EQ(listener->GetEventsReceived(), 1);

    // Unregister and destroy listener
    bus.UnregisterListener(listener.get());
    listener.reset();

    // Producer emits more events
    producer.EmitSimple();
    bus.Communicate();

    // Should not crash
}

TEST(EventSystem, ProducerCanUnregisterSafely)
{
    ObserverBus<TestEvent> bus;
    auto producer = std::make_unique<TestProducer>();
    TestListener listener;

    bus.RegisterProducer(producer.get());
    bus.RegisterListener(&listener);

    producer->EmitSimple();
    bus.Communicate();
    listener.ProcessEvents();

    ASSERT_EQ(listener.GetEventsReceived(), 1);

    // Unregister and destroy producer
    bus.UnregisterProducer(producer.get());
    producer.reset();

    // Should not crash
    bus.Communicate();
    listener.ProcessEvents();
}

// ============================================================================
// Stress Test: High Event Volume
// ============================================================================

TEST(EventSystemStress, HighVolumeEventDispatch)
{
    constexpr int NUM_EVENTS = 10000;

    ObserverBus<TestEvent> bus;
    TestProducer producer;
    TestListener listener;

    bus.RegisterProducer(&producer);
    bus.RegisterListener(&listener);

    // Dispatch many events
    for (int i = 0; i < NUM_EVENTS; ++i)
    {
        producer.EmitData(1);
    }

    bus.Communicate();
    listener.ProcessEvents();

    ASSERT_EQ(listener.GetEventsReceived(), NUM_EVENTS);
    ASSERT_EQ(listener.GetTotalValue(), NUM_EVENTS);
}

TEST(EventSystemStress, ManyListeners)
{
    constexpr int NUM_LISTENERS = 100;

    ObserverBus<TestEvent> bus;
    TestProducer producer;
    std::vector<std::unique_ptr<TestListener>> listeners;

    bus.RegisterProducer(&producer);

    for (int i = 0; i < NUM_LISTENERS; ++i)
    {
        listeners.push_back(std::make_unique<TestListener>());
        bus.RegisterListener(listeners.back().get());
    }

    producer.EmitSimple();
    bus.Communicate();

    for (auto& listener : listeners)
    {
        listener->ProcessEvents();
        ASSERT_EQ(listener->GetEventsReceived(), 1);
    }
}

TEST(EventSystemStress, ManyProducers)
{
    constexpr int NUM_PRODUCERS = 100;

    ObserverBus<TestEvent> bus;
    std::vector<std::unique_ptr<TestProducer>> producers;
    TestListener listener;

    bus.RegisterListener(&listener);

    for (int i = 0; i < NUM_PRODUCERS; ++i)
    {
        producers.push_back(std::make_unique<TestProducer>());
        bus.RegisterProducer(producers.back().get());
    }

    for (auto& producer : producers)
    {
        producer->EmitSimple();
    }

    bus.Communicate();
    listener.ProcessEvents();

    ASSERT_EQ(listener.GetEventsReceived(), NUM_PRODUCERS);
}

// ============================================================================
// Thread Safety Tests
// ============================================================================

TEST(EventSystemThreadSafety, ConcurrentDispatches)
{
    constexpr int NUM_THREADS = 4;
    constexpr int EVENTS_PER_THREAD = 1000;

    ObserverBus<TestEvent> bus;
    std::vector<std::unique_ptr<TestProducer>> producers;
    TestListener listener;

    bus.RegisterListener(&listener); for (int i = 0; i < NUM_THREADS; ++i)
    {
        producers.push_back(std::make_unique<TestProducer>());
        bus.RegisterProducer(producers.back().get());
    }

    std::barrier barrier{ NUM_THREADS + 1 };
    std::vector<std::thread> threads;

    // Spawn producer threads
    for (int i = 0; i < NUM_THREADS; ++i)
    {
        threads.emplace_back([&, i]() {
            barrier.arrive_and_wait();

            for (int j = 0; j < EVENTS_PER_THREAD; ++j)
            {
                producers[i]->EmitData(1);
            }
            });
    }

    barrier.arrive_and_wait(); // Start all threads simultaneously

    for (auto& thread : threads)
    {
        thread.join();
    }

    bus.Communicate();
    listener.ProcessEvents();

    ASSERT_EQ(listener.GetEventsReceived(), NUM_THREADS * EVENTS_PER_THREAD);
    ASSERT_EQ(listener.GetTotalValue(), NUM_THREADS * EVENTS_PER_THREAD);
}

TEST(EventSystemThreadSafety, ConcurrentRegistrationAndDispatch)
{
    constexpr int NUM_PRODUCER_THREADS = 2;
    constexpr int NUM_LISTENER_THREADS = 2;
    constexpr int EVENTS_PER_THREAD = 500;

    ObserverBus<TestEvent> bus;
    std::vector<std::unique_ptr<TestProducer>> producers;
    std::vector<std::unique_ptr<TestListener>> listeners;

    std::atomic<int> totalEventsReceived{ 0 };
    std::atomic<bool> stopFlag{ false };

    // Producer threads
    std::vector<std::thread> threads;
    for (int i = 0; i < NUM_PRODUCER_THREADS; ++i)
    {
        threads.emplace_back([&]() {
            auto producer = std::make_unique<TestProducer>();
            bus.RegisterProducer(producer.get());

            for (int j = 0; j < EVENTS_PER_THREAD; ++j)
            {
                producer->EmitData(1);
                std::this_thread::sleep_for(std::chrono::microseconds(10));
            }

            bus.UnregisterProducer(producer.get());
            });
    }

    // Listener threads
    for (int i = 0; i < NUM_LISTENER_THREADS; ++i)
    {
        threads.emplace_back([&]() {
            auto listener = std::make_unique<TestListener>();
            bus.RegisterListener(listener.get());

            while (!stopFlag.load(std::memory_order_acquire))
            {
                bus.Communicate();
                listener->ProcessEvents();
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }

            totalEventsReceived.fetch_add(listener->GetEventsReceived(),
                std::memory_order_relaxed);
            bus.UnregisterListener(listener.get());
            });
    }

    // Wait for producers
    for (int i = 0; i < NUM_PRODUCER_THREADS; ++i)
    {
        threads[i].join();
    }

    // Give listeners time to process remaining events
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    stopFlag.store(true, std::memory_order_release);

    // Wait for listeners
    for (int i = NUM_PRODUCER_THREADS; i < threads.size(); ++i)
    {
        threads[i].join();
    }

    // Each listener should receive some events
    ASSERT_GT(totalEventsReceived.load(), 0);
}

TEST(EventSystemThreadSafety, ListenerUnregistersDuringDispatch)
{
    constexpr int NUM_EVENTS = 1000;

    ObserverBus<TestEvent> bus;
    TestProducer producer;
    auto listener = std::make_unique<TestListener>();

    std::atomic<bool> shouldUnregister{ false };

    listener->SetCallback([&](std::shared_ptr<const TestEvent> e) {
        if (shouldUnregister.load(std::memory_order_acquire))
        {
            bus.UnregisterListener(listener.get());
        }
        });

    bus.RegisterProducer(&producer);
    bus.RegisterListener(listener.get());

    // Dispatch events
    for (int i = 0; i < NUM_EVENTS; ++i)
    {
        producer.EmitSimple();
    }

    // Process some events
    bus.Communicate();
    for (int i = 0; i < 10; ++i)
    {
        auto event = listener->GetEventQueue().TryNext();
        if (event)
        {
            listener->OnEvent(event.value());
        }
    }

    // Trigger unregister during processing
    shouldUnregister.store(true, std::memory_order_release);// Continue processing - should not crash
    while (auto event = listener->GetEventQueue().TryNext())
    {
        listener->OnEvent(event.value());
    }

    // Listener is now unregistered, more events shouldn't reach it
    int eventsBeforeUnregister = listener->GetEventsReceived();

    producer.EmitSimple();
    bus.Communicate();
    listener->ProcessEvents();

    // Should not receive new events
    ASSERT_EQ(listener->GetEventsReceived(), eventsBeforeUnregister);
}

TEST(EventSystemThreadSafety, BusDestructionWithActiveProducersAndListeners)
{
    TestProducer producer;
    TestListener listener;

    {
        ObserverBus<TestEvent> bus;
        bus.RegisterProducer(&producer);
        bus.RegisterListener(&listener);

        producer.EmitSimple();
        bus.Communicate();

        // Bus destructor should safely unregister everything
    }

    // Producer and listener should still be alive and safe
    // Their destructors will handle cleanup
}

// ============================================================================
// Edge Case Tests
// ============================================================================

TEST(EventSystemEdgeCases, EmptyBusCommunicate)
{
    ObserverBus<TestEvent> bus;

    // Should not crash
    ASSERT_NO_THROW({
        bus.Communicate();
        bus.Communicate();
        bus.Communicate();
        });
}

TEST(EventSystemEdgeCases, DispatchWithNoListeners)
{
    ObserverBus<TestEvent> bus;
    TestProducer producer;

    bus.RegisterProducer(&producer);

    // Should not crash
    ASSERT_NO_THROW({
        producer.EmitSimple();
        producer.EmitSimple();
        bus.Communicate();
        });
}

TEST(EventSystemEdgeCases, ListenerWithNoProducers)
{
    ObserverBus<TestEvent> bus;
    TestListener listener;

    bus.RegisterListener(&listener);

    // Should not crash
    ASSERT_NO_THROW({
        bus.Communicate();
        listener.ProcessEvents();
        });

    ASSERT_EQ(listener.GetEventsReceived(), 0);
}

TEST(EventSystemEdgeCases, DoubleUnregister)
{
    ObserverBus<TestEvent> bus;
    TestProducer producer;
    TestListener listener;

    bus.RegisterProducer(&producer);
    bus.RegisterListener(&listener);

    // First unregister
    bus.UnregisterProducer(&producer);
    bus.UnregisterListener(&listener);

    // Second unregister - should be safe (no-op)
    ASSERT_NO_THROW({
        bus.UnregisterProducer(&producer);
        bus.UnregisterListener(&listener);
        });
}

TEST(EventSystemEdgeCases, ReregisterAfterUnregister)
{
    ObserverBus<TestEvent> bus;
    TestProducer producer;
    TestListener listener;

    bus.RegisterProducer(&producer);
    bus.RegisterListener(&listener);

    producer.EmitSimple();
    bus.Communicate();
    listener.ProcessEvents();

    ASSERT_EQ(listener.GetEventsReceived(), 1);

    // Unregister
    bus.UnregisterProducer(&producer);
    bus.UnregisterListener(&listener);

    // Re-register
    bus.RegisterProducer(&producer);
    bus.RegisterListener(&listener);

    producer.EmitSimple();
    bus.Communicate();
    listener.ProcessEvents();

    ASSERT_EQ(listener.GetEventsReceived(), 2);
}

// ============================================================================
// Performance Benchmark (Not a test, just for measurement)
// ============================================================================

TEST(EventSystemBenchmark, MeasureEventThroughput)
{
    constexpr int NUM_EVENTS = 100000;

    ObserverBus<TestEvent> bus;
    TestProducer producer;
    TestListener listener;

    bus.RegisterProducer(&producer);
    bus.RegisterListener(&listener);

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < NUM_EVENTS; ++i)
    {
        producer.EmitData(i);
    }

    bus.Communicate();
    listener.ProcessEvents();

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    ASSERT_EQ(listener.GetEventsReceived(), NUM_EVENTS);

    std::cout << "Processed " << NUM_EVENTS << " events in "
        << duration.count() << "ms" << std::endl;
    std::cout << "Throughput: " << (NUM_EVENTS * 1000.0 / duration.count())
        << " events/sec" << std::endl;
}