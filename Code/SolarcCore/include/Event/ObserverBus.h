#pragma once
#include "Preprocessor/API.h"
#include "Event/Event.h"
#include "Event/EventBus.h"
#include "MT/ThreadChecker.h"

/**
 * Queued event bus implementation
 *
 * Architecture:
 * - Producers dispatch events → Bus internal queue
 * - Communicate() moves events from bus queue → Listener queues
 * - Listeners process events from their own queues
 *
 * Thread Safety:
 * - RegisterProducer/Listener: Thread-safe
 * - UnregisterProducer/Listener: Thread-safe
 * - Communicate(): Main thread only (enforced by ThreadChecker)
 * - DispatchEvent() (from producers): Thread-safe
 *
 * Note: Listeners that unregister between event collection and dispatch
 * will safely ignore the event (Dispatch checks m_unregistered).
 */
template<event_type EVENT_TYPE>
class ObserverBus : public EventBus<EVENT_TYPE>
{
    using EventRegistration = typename EventBus<EVENT_TYPE>::EventRegistration;

public:
    ObserverBus() = default;

    virtual ~ObserverBus()
    {
        SOLARC_TRACE("ObserverBus destructor: Cleaning up registrations");

        std::vector<std::shared_ptr<EventRegistration>> listeners;
        std::vector<std::shared_ptr<EventRegistration>> producers;

        {
            std::lock_guard lk(m_Mtx);

            for (auto& p : m_ListenersMap) listeners.push_back(p.second);
            for (auto& p : m_ProducersMap) producers.push_back(p.second);

            m_ListenersMap.clear();
            m_ProducersMap.clear();

            for (auto& lr : listeners) if (lr) lr->DisableUnregisterCallback();
            for (auto& pr : producers) if (pr) pr->DisableUnregisterCallback();
        }

        for (auto& lr : listeners) if (lr) lr->Unregister();
        for (auto& pr : producers) if (pr) pr->Unregister();

        SOLARC_TRACE("ObserverBus destructor: Complete");
    }

    void Communicate() noexcept override
    {
        std::optional<std::shared_ptr<const EVENT_TYPE>> event;
        while (event = m_BusQueue.TryNext())
        {
            std::vector<std::shared_ptr<EventRegistration>> listenerRegs;
            {
                std::lock_guard lk(m_Mtx);
                listenerRegs.reserve(m_ListenersMap.size());

                for (auto& pair : m_ListenersMap)
                {
                    listenerRegs.push_back(pair.second);
                }
            }

            for (auto& reg : listenerRegs)
            {
                reg->Dispatch(event.value());
            }
        }
    }

    void RegisterProducer(EventProducer<EVENT_TYPE>* producer) noexcept
    {
        assert(producer != nullptr && "Cannot register null producer");

        std::lock_guard lk(m_Mtx);

        if (m_ProducersMap.find(producer) != m_ProducersMap.end())
        {
            SOLARC_WARN("Producer already registered to this bus");
            return;
        }

        auto unregisterCB = [this, producer]() { this->UnregisterProducer(producer); };
        auto reg = this->RegisterProducerHelper(producer, unregisterCB);
        reg->SetQueue(&m_BusQueue);
        m_ProducersMap[producer] = reg;

        SOLARC_TRACE("Producer registered to ObserverBus");
    }

    void RegisterListener(EventListener<EVENT_TYPE>* listener) noexcept
    {
        assert(listener != nullptr && "Cannot register null listener");

        std::lock_guard lk(m_Mtx);

        if (m_ListenersMap.find(listener) != m_ListenersMap.end())
        {
            SOLARC_WARN("Listener already registered to this bus");
            return;
        }

        auto unregisterCB = [this, listener]() { this->UnregisterListener(listener); };
        auto reg = this->RegisterListenerHelper(listener, unregisterCB);
        m_ListenersMap[listener] = reg;

        SOLARC_TRACE("Listener registered to ObserverBus");
    }

    void UnregisterProducer(EventProducer<EVENT_TYPE>* producer)
    {
        std::shared_ptr<EventRegistration> reg;
        {
            std::lock_guard lk(m_Mtx);
            auto it = m_ProducersMap.find(producer);
            if (it == m_ProducersMap.end()) return;

            reg = it->second;
            m_ProducersMap.erase(it);
        }

        if (reg)
        {
            reg->Unregister();
            SOLARC_TRACE("Producer unregistered from ObserverBus");
        }
    }

    void UnregisterListener(EventListener<EVENT_TYPE>* listener)
    {
        std::shared_ptr<EventRegistration> reg;
        {
            std::lock_guard lk(m_Mtx);
            auto it = m_ListenersMap.find(listener);
            if (it == m_ListenersMap.end()) return;

            reg = it->second;
            m_ListenersMap.erase(it);
        }

        if (reg)
        {
            reg->Unregister();
            SOLARC_TRACE("Listener unregistered from ObserverBus");
        }
    }

    size_t GetProducerCount() const
    {
        std::lock_guard lk(m_Mtx);
        return m_ProducersMap.size();
    }

    size_t GetListenerCount() const
    {
        std::lock_guard lk(m_Mtx);
        return m_ListenersMap.size();
    }

private:
    EventQueue<EVENT_TYPE> m_BusQueue;
    std::unordered_map<EventProducer<EVENT_TYPE>*, std::shared_ptr<EventRegistration>> m_ProducersMap;
    std::unordered_map<EventListener<EVENT_TYPE>*, std::shared_ptr<EventRegistration>> m_ListenersMap;
    mutable std::mutex m_Mtx;
};