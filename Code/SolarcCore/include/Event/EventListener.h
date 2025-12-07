#pragma once
#include "Preprocessor/API.h"
#include "Event/Event.h"
#include "Event/EventBus.h"
#include "Logging/LogMacros.h"

#include <mutex>
#include <list>
#include <memory>

/**
 * Base class for objects that listen to events
 *
 * Usage:
 * 1. Inherit from EventListener<YourEventType>
 * 2. Implement OnEvent() to handle events
 * 3. Call ProcessEvents() periodically to drain the queue
 *
 * Thread Safety:
 * - Event queue: Thread-safe (events can be dispatched from any thread)
 * - OnEvent(): Called only from the thread that calls ProcessEvents()
 * - Destructor: Thread-safe, blocks until all registrations are cleaned up
 */
template<event_type EVENT_TYPE>
class EventListener
{
    friend class EventBus<EVENT_TYPE>;
    using EventRegistration = typename EventBus<EVENT_TYPE>::EventRegistration;

public:
    EventListener() = default;

    virtual ~EventListener()
    {
        try {
            UnregisterEventConnections();
        }
        catch (const std::exception& e) {
            SOLARC_ERROR("Exception in EventListener destructor: {}", e.what());
        }
        catch (...) {
            SOLARC_ERROR("Unknown exception in EventListener destructor");
        }
    }

protected:
    /**
     * Called when an event is processed
     * param e: Event to handle (never null)
     * note: Override this in derived classes
     * note: Called from the thread that invokes ProcessEvents()
     */
    virtual void OnEvent(const std::shared_ptr<const EVENT_TYPE>& e) = 0;

    /**
     * Process all queued events
     * note: Call this periodically to drain the event queue
     * note: Should be called from a single thread (typically main thread)
     */
    void ProcessEvents()
    {
        std::optional<std::shared_ptr<const EVENT_TYPE>> event;
        while (event = m_EventQueue.TryNext())
        {
            OnEvent(event.value());
        }
    }

    /**
     * Check if there are pending events in the queue
     * return true if queue has events, false otherwise
     */
    bool HasPendingEvents()
    {
        return !m_EventQueue.IsEmpty();
    }

    EventQueue<EVENT_TYPE> m_EventQueue;

private:
    void UnregisterEventConnections();

    mutable std::mutex m_RegisterMtx;
    std::list<std::weak_ptr<EventRegistration>> m_Registration;
};

// ---------------- Implementation ----------------

template<event_type EVENT_TYPE>
inline void EventListener<EVENT_TYPE>::UnregisterEventConnections()
{
     std::vector<std::shared_ptr<EventRegistration>> strongRef;
        {
            std::lock_guard lock(m_RegisterMtx);
            strongRef = EventBus<EVENT_TYPE>::template CollectLive<
                std::list<std::weak_ptr<EventRegistration>>,
                EventRegistration>(m_Registration);

            EventBus<EVENT_TYPE>::template PruneAndRemove<
                std::list<std::weak_ptr<EventRegistration>>,
                EventRegistration>(m_Registration, nullptr);
        }

        for (auto&& reg : strongRef)
        {
            if (reg)
            {
                reg->Unregister();
            }
        }
};
