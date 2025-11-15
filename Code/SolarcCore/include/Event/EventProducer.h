#pragma once
#include "Preprocessor/API.h"
#include "Event/Event.h"
#include "Event/EventBus.h"

// ------------------------ EventProducer ------------------------

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
            // Unregister connections automatically in the destructor
            UnregisterEventConnections();
        }
        catch (...) {}
    }

protected:
    void DispatchEvent(std::shared_ptr<const EVENT_TYPE> e);

private:
    void UnregisterEventConnections()
    {
        std::vector<std::shared_ptr<EventRegistration>> strongRef;
        {
            std::lock_guard lock(m_RegisterMtx);
            strongRef = EventBus<EVENT_TYPE>::template CollectLive<std::list<std::weak_ptr<EventRegistration>>, EventRegistration>(m_Registration);
            EventBus<EVENT_TYPE>::template PruneAndRemove<std::list<std::weak_ptr<EventRegistration>>, EventRegistration>(m_Registration, nullptr);
        }

        for (auto&& d : strongRef)
            d->Unregister();
    }

    mutable std::mutex m_RegisterMtx;
    std::list<std::weak_ptr<EventRegistration>> m_Registration;
};

template<event_type EVENT_TYPE>
inline void EventProducer<EVENT_TYPE>::DispatchEvent(std::shared_ptr<const EVENT_TYPE> e)
{
    std::vector<std::shared_ptr<EventRegistration>> strongRef;
    {
        std::lock_guard lock(m_RegisterMtx);
        strongRef = EventBus<EVENT_TYPE>::template CollectLive<std::list<std::weak_ptr<EventRegistration>>, EventRegistration>(m_Registration);
        EventBus<EVENT_TYPE>::template PruneAndRemove<std::list<std::weak_ptr<EventRegistration>>, EventRegistration>(m_Registration, nullptr);
    }

    for (auto&& d : strongRef)
        d->Dispatch(e);
}