#pragma once
#include "Preprocessor/API.h"
#include "Event/Event.h"
#include "Event/EventBus.h"

// ------------------------ EventListener ------------------------

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
            // Unregister connections automatically in the destructor
            UnregisterEventConnections();
        }
        catch (...) {}
    }

protected:
    virtual void OnEvent(const std::shared_ptr<const EVENT_TYPE>& e) = 0;

    EventQueue<EVENT_TYPE> m_EventQueue;

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