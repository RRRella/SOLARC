#pragma once
#include "Preprocessor/API.h"
#include "Event/Event.h"
#include "Event/EventBus.h"

// ------------------------ ObserverBus ------------------------

template<event_type EVENT_TYPE>
class ObserverBus : public EventBus<EVENT_TYPE>
{
    using ListenerRegistration = typename EventBus<EVENT_TYPE>::ListenerRegistration;
    using ProducerRegistration = typename EventBus<EVENT_TYPE>::ProducerRegistration;
    using Dispatcher = typename EventBus<EVENT_TYPE>::Dispatcher;

public:
    ObserverBus() = default;

    // Important: to prevent registration callbacks re-entering the bus while
    // the bus is tearing down, we disable those callbacks before calling
    // Registration::Unregister(). This preserves the behavior you wanted:
    // the bus waits for unregisters to complete, but avoids callback re-entry.
    virtual ~ObserverBus()
    {
        std::vector<std::shared_ptr<ListenerRegistration>> listeners;
        std::vector<std::shared_ptr<ProducerRegistration>> producers;

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

    virtual void RegisterProducer(EventProducer<EVENT_TYPE>* producer) noexcept = 0;
    virtual void RegisterListener(EventListener<EVENT_TYPE>* listener) noexcept = 0;

    virtual void UnRegisterProducer(EventProducer<EVENT_TYPE>* producer) = 0;
    virtual void UnRegisterListener(EventListener<EVENT_TYPE>* listener) = 0;

protected:
    std::unordered_map<EventProducer<EVENT_TYPE>*, std::shared_ptr<ProducerRegistration>> m_ProducersMap;
    std::unordered_map<EventListener<EVENT_TYPE>*, std::shared_ptr<ListenerRegistration>> m_ListenersMap;
    mutable std::mutex m_Mtx;
};

// ------------------------ DirectObserverBus ------------------------

template<event_type EVENT_TYPE>
class DirectObserverBus : public ObserverBus<EVENT_TYPE>
{
    using ListenerRegistration = typename EventBus<EVENT_TYPE>::ListenerRegistration;
    using ProducerRegistration = typename EventBus<EVENT_TYPE>::ProducerRegistration;
    using Dispatcher = typename EventBus<EVENT_TYPE>::Dispatcher;
    using DISPATCH_TYPE = typename EventBus<EVENT_TYPE>::DISPATCH_TYPE;

public:
    DirectObserverBus() = default;
    ~DirectObserverBus() override = default;

    void Communicate() noexcept override {}

    void RegisterProducer(EventProducer<EVENT_TYPE>* producer) noexcept override
    {
        std::shared_ptr<ProducerRegistration> reg;
        {
            std::lock_guard lk(this->m_Mtx);
            auto unregisterCB = [this, producer]() { this->UnRegisterProducer(producer); };
            reg = this->RegisterProducerHelper(producer, unregisterCB);

            // Wire existing listener dispatchers into this producer's registration.
            for (auto& kv : this->m_ListenersMap)
            {
                auto& listenerReg = kv.second;
                if (listenerReg && listenerReg->dispatcher)
                    reg->AddDispatcher(listenerReg->dispatcher);
            }

            this->m_ProducersMap[producer] = reg;
        }
    }

    void RegisterListener(EventListener<EVENT_TYPE>* listener) noexcept override
    {
        std::shared_ptr<ListenerRegistration> reg;
        {
            std::lock_guard lk(this->m_Mtx);
            auto unregisterCB = [this, listener]() { this->UnRegisterListener(listener); };
            reg = this->RegisterListenerHelper(listener, unregisterCB, DISPATCH_TYPE::DIRECT);

            // Wire this listener's dispatcher into all existing producers so they will call it.
            for (auto& kv : this->m_ProducersMap)
            {
                auto& producerReg = kv.second;
                if (producerReg && reg->dispatcher)
                    producerReg->AddDispatcher(reg->dispatcher);
            }

            this->m_ListenersMap[listener] = reg;
        }
    }

    void UnRegisterProducer(EventProducer<EVENT_TYPE>* producer) override
    {
        std::shared_ptr<ProducerRegistration> reg;
        {
            std::lock_guard lk(this->m_Mtx);
            auto it = this->m_ProducersMap.find(producer);
            if (it == this->m_ProducersMap.end()) return;
            reg = it->second;
            this->m_ProducersMap.erase(it);
        }

        if (reg) reg->Unregister();
    }

    void UnRegisterListener(EventListener<EVENT_TYPE>* listener) override
    {
        std::shared_ptr<ListenerRegistration> reg;
        {
            std::lock_guard lk(this->m_Mtx);
            auto it = this->m_ListenersMap.find(listener);
            if (it == this->m_ListenersMap.end()) return;
            reg = it->second;

            // Remove this listener's dispatcher from all producers
            for (auto& kv : this->m_ProducersMap)
                if (kv.second && reg && reg->dispatcher)
                    kv.second->RemoveDispatcher(reg->dispatcher);

            this->m_ListenersMap.erase(it);
        }

        if (reg) reg->Unregister();
    }

private:
};
