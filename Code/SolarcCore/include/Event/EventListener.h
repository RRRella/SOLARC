#pragma once
#include "Preprocessor/API.h"
#include "Event/Event.h"
#include "Event/EventBus.h"

// ------------------------ EventListener ------------------------

template<event_type EVENT_TYPE>
class EventListener
{
    friend class EventBus<EVENT_TYPE>;
    using EventCallBack = std::function<void(std::shared_ptr<const EVENT_TYPE>)>;
    using ListenerRegistration = typename EventBus<EVENT_TYPE>::ListenerRegistration;

public:
    EventListener() = default;
    virtual ~EventListener()
    {
        try { UnregisterEventConnections(); }
        catch (...) {}
    }

protected:
    virtual void OnEvent(std::shared_ptr<const EVENT_TYPE> e) = 0;

    EventCallBack m_EventCallBack = std::bind(&EventListener<EVENT_TYPE>::OnEvent, this, std::placeholders::_1);

    EventQueue<EVENT_TYPE> m_Queue;

    //MUST be called on top of every derived class's destructor of this
    void UnregisterEventConnections();
private:
    mutable std::mutex m_RegisterMtx;
    std::list<std::weak_ptr<ListenerRegistration>> m_Registration;
};

template<event_type EVENT_TYPE>
inline void EventListener<EVENT_TYPE>::UnregisterEventConnections()
{
    std::vector<std::shared_ptr<ListenerRegistration>> strongRef;
    {
        std::lock_guard lock(m_RegisterMtx);
        strongRef = EventBus<EVENT_TYPE>::template CollectLive<std::list<std::weak_ptr<ListenerRegistration>>, ListenerRegistration>(m_Registration);
        EventBus<EVENT_TYPE>::template PruneAndRemove<std::list<std::weak_ptr<ListenerRegistration>>, ListenerRegistration>(m_Registration, nullptr);
    }

    for (auto&& d : strongRef)
        d->Unregister();
}
