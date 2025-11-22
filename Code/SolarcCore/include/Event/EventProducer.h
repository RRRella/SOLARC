#pragma once
#include "Preprocessor/API.h"
#include "Event/Event.h"
#include "Event/EventBus.h"
#include "Logging/LogMacros.h"

/**
 * Base class for objects that produce events
 *
 * Thread Safety:
 * - DispatchEvent(): Thread-safe, can be called from any thread
 * - Destructor: Thread-safe, blocks until all registrations are cleaned up
 */
template<event_type EVENT_TYPE>
class EventProducer
{
    friend class EventBus<EVENT_TYPE>;
    using EventRegistration = typename EventBus<EVENT_TYPE>::EventRegistration;

public:
    EventProducer() = default;

    virtual ~EventProducer()
    {
        try {
            UnregisterEventConnections();
        }
        catch (const std::exception& e) {
            SOLARC_ERROR("Exception in EventProducer destructor: {}", e.what());
        }
        catch (...) {
            SOLARC_ERROR("Unknown exception in EventProducer destructor");
        }
    }

protected:
    /**
     * Dispatch an event to all registered buses
     * param e: Event to dispatch (must not be null)
     * note: Thread-safe, can be called from any thread
     */
    void DispatchEvent(std::shared_ptr<const EVENT_TYPE> e)
    {
        assert(e != nullptr && "Cannot dispatch null event");

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
            if (reg) // Extra safety check
            {
                reg->Dispatch(e);
            }
        }
    }

private:
    void UnregisterEventConnections()
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
    }

    mutable std::mutex m_RegisterMtx;
    std::list<std::weak_ptr<EventRegistration>> m_Registration;
};