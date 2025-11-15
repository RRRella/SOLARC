#pragma once
#include "Preprocessor/API.h"
#include "Event/Event.h"
#include "Event/EventBus.h"

// ------------------------ ObserverBus (only queued implementation now) ------------------------

template<event_type EVENT_TYPE>
class ObserverBus : public EventBus<EVENT_TYPE>
{
    using EventRegistration = typename EventBus<EVENT_TYPE>::EventRegistration;

public:
    ObserverBus() = default;

    // Important: to prevent registration callbacks re-entering the bus while
    // the bus is tearing down, we disable those callbacks before calling
    // Registration::Unregister(). This preserves the behavior you wanted:
    // the bus waits for unregisters to complete, but avoids callback re-entry.
    virtual ~ObserverBus()
    {
        std::vector<std::shared_ptr<EventRegistration>> listeners;
        std::vector<std::shared_ptr<EventRegistration>> producers;

        {
            std::lock_guard lk(m_Mtx);
            for (auto& p : m_ListenersMap) listeners.push_back(p.second);
            for (auto& p : m_ProducersMap) producers.push_back(p.second);

            // Clear the maps so external lookups by ID won't find registrations anymore.
            m_ListenersMap.clear();
            m_ProducersMap.clear();

            // Disable unregister callbacks while still holding the bus lock so that
            // registrations won't call back into this bus during Unregister().
            for (auto& lr : listeners) if (lr) lr->DisableUnregisterCallback();
            for (auto& pr : producers) if (pr) pr->DisableUnregisterCallback();
        }

        // Call Unregister outside of the bus lock. Unregister will wait for in-flight
        // dispatches to finish but will not attempt to call back into the bus.
        for (auto& lr : listeners) if (lr) lr->Unregister();
        for (auto& pr : producers) if (pr) pr->Unregister();
    }

    void Communicate() noexcept override
    {
        // Move events from the bus's internal queue to all listener queues
        std::optional<std::shared_ptr<const EVENT_TYPE>> event;
        while (event = m_BusQueue.TryNext())
        {
            // Forward to all listeners using their registrations
            std::vector<std::shared_ptr<EventRegistration>> listenerRegs;
            {
                std::lock_guard lk(m_Mtx);
                for (auto& pair : m_ListenersMap)
                {
                    listenerRegs.push_back(pair.second);
                }
            }

            for (auto& reg : listenerRegs)
            {
                // Dispatch the same event to each listener's registration
                reg->Dispatch(event.value());
            }
        }
    }

    void RegisterProducer(EventProducer<EVENT_TYPE>* producer) noexcept
    {
        std::shared_ptr<EventRegistration> reg;
        {
            std::lock_guard lk(m_Mtx);
            auto unregisterCB = [this, producer]() { this->UnregisterProducer(producer); };
            reg = this->RegisterProducerHelper(producer, unregisterCB);

            // Set the queue to the bus's internal queue
            reg->SetQueue(&m_BusQueue);

            m_ProducersMap[producer] = reg;
        }
    }

    void RegisterListener(EventListener<EVENT_TYPE>* listener) noexcept
    {
        std::shared_ptr<EventRegistration> reg;
        {
            std::lock_guard lk(m_Mtx);
            auto unregisterCB = [this, listener]() { this->UnregisterListener(listener); };
            reg = this->RegisterListenerHelper(listener, unregisterCB);

            // The listener's registration already points to its own queue
            m_ListenersMap[listener] = reg;
        }
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

        if (reg) reg->Unregister();
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

        if (reg) reg->Unregister();
    }

private:
    EventQueue<EVENT_TYPE> m_BusQueue;
    std::unordered_map<EventProducer<EVENT_TYPE>*, std::shared_ptr<EventRegistration>> m_ProducersMap;
    std::unordered_map<EventListener<EVENT_TYPE>*, std::shared_ptr<EventRegistration>> m_ListenersMap;
    mutable std::mutex m_Mtx;
};