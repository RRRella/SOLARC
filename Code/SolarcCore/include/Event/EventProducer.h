#pragma once
#include "Preprocessor/API.h"
#include "Event/Event.h"
#include "Event/EventBus.h"

// ------------------------ EventProducer ------------------------

template<event_type EVENT_TYPE>
class EventProducer
{
    friend class EventBus<EVENT_TYPE>;
    using ProducerRegistration = typename EventBus<EVENT_TYPE>::ProducerRegistration;

public:
    EventProducer() = default;
    virtual ~EventProducer()
    {
        try { UnregisterEventConnections(); }
        catch (...) {}
    }

protected:
    void DispatchEvent(std::shared_ptr<const EVENT_TYPE> e);

    //MUST be called on top of every derived class's destructor of this
    void UnregisterEventConnections();

private:
    mutable std::mutex m_RegisterMtx;
    std::list<std::weak_ptr<ProducerRegistration>> m_Registration;
};

template<event_type EVENT_TYPE>
inline void EventProducer<EVENT_TYPE>::DispatchEvent(std::shared_ptr<const EVENT_TYPE> e)
{
    std::vector<std::shared_ptr<ProducerRegistration>> strongRef;
    {
        std::lock_guard lock(m_RegisterMtx);
        strongRef = EventBus<EVENT_TYPE>::template CollectLive<std::list<std::weak_ptr<ProducerRegistration>>, ProducerRegistration>(m_Registration);
        EventBus<EVENT_TYPE>::template PruneAndRemove<std::list<std::weak_ptr<ProducerRegistration>>, ProducerRegistration>(m_Registration, nullptr);
    }

    for (auto&& d : strongRef)
        d->Dispatch(e);
}

template<event_type EVENT_TYPE>
inline void EventProducer<EVENT_TYPE>::UnregisterEventConnections()
{
    std::vector<std::shared_ptr<ProducerRegistration>> strongRef;
    {
        std::lock_guard lock(m_RegisterMtx);
        strongRef = EventBus<EVENT_TYPE>::template CollectLive<std::list<std::weak_ptr<ProducerRegistration>>, ProducerRegistration>(m_Registration);
        EventBus<EVENT_TYPE>::template PruneAndRemove<std::list<std::weak_ptr<ProducerRegistration>>, ProducerRegistration>(m_Registration, nullptr);
    }

    for (auto&& d : strongRef)
        d->Unregister();
}
