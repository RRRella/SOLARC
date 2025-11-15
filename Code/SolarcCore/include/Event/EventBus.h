#pragma once
#include "Preprocessor/API.h"
#include "Event/Event.h"

// ------------------------ EventBus ------------------------

template<event_type EVENT_TYPE>
class EventProducer;
template<event_type EVENT_TYPE>
class EventListener;

template<event_type EVENT_TYPE>
class EventBus
{
    friend class EventListener<EVENT_TYPE>;
    friend class EventProducer<EVENT_TYPE>;

public:
    virtual ~EventBus() = default;
    virtual void Communicate() noexcept = 0;

protected:
    // ---------------- EventRegistration ----------------
    // Unified registration that holds a single queue for dispatching.
    class EventRegistration : public std::enable_shared_from_this<EventRegistration>
    {
        friend class EventBus<EVENT_TYPE>;
    public:
        explicit EventRegistration(std::function<void()> unregisterCBIn)
            : unregisterCB(std::move(unregisterCBIn)), m_Queue(nullptr), m_unregistered(false), m_inflight(0) {
        }

        ~EventRegistration() = default;

        // Dispatch to the held queue.
        void Dispatch(std::shared_ptr<const EVENT_TYPE> e);

        // Synchronous and idempotent unregistration. Will call unregisterCB unless disabled.
        void Unregister();

        // Prevent Unregister() from calling the external callback.
        void DisableUnregisterCallback();

        // Set the queue for this registration
        void SetQueue(EventQueue<EVENT_TYPE>* queue);

        std::function<void()> unregisterCB;

    private:
        mutable std::mutex mtx;
        mutable std::condition_variable m_cv;
        bool m_unregistered;
        int m_inflight;
        EventQueue<EVENT_TYPE>* m_Queue; // Raw pointer to queue (safe: queue outlives registration)
    };

    // ------------------ Helper templates to reduce repetition ------------------
    template<typename WeakListT, typename SharedT>
    static void PruneAndRemove(WeakListT& list, const std::shared_ptr<SharedT>& match);

    template<typename WeakListT, typename SharedT>
    static std::vector<std::shared_ptr<SharedT>> CollectLive(const WeakListT& list);

    // ------------------ Registration helper methods ------------------
    std::shared_ptr<EventRegistration> RegisterProducerHelper(
        EventProducer<EVENT_TYPE>* producer,
        std::function<void()> unregisterCallBack);

    void UnregisterProducerHelper(
        EventProducer<EVENT_TYPE>* producer,
        std::shared_ptr<EventRegistration> reg);

    std::shared_ptr<EventRegistration> RegisterListenerHelper(
        EventListener<EVENT_TYPE>* listener,
        std::function<void()> unregisterCallBack);

    void UnregisterListenerHelper(
        EventListener<EVENT_TYPE>* listener,
        std::shared_ptr<EventRegistration> reg);

private:
};

// ---------------- Implementation ----------------

template<event_type EVENT_TYPE>
inline void EventBus<EVENT_TYPE>::EventRegistration::Dispatch(std::shared_ptr<const EVENT_TYPE> e)
{
    std::unique_lock lk(mtx);
    if (m_unregistered) return;
    if (!m_Queue) return; // No queue to dispatch to
    ++m_inflight;
    // Unlock while we access the queue to avoid holding the lock during the push
    lk.unlock();

    // Push to the held queue
    m_Queue->Push(std::move(e));

    // Lock again to decrement inflight
    lk.lock();
    --m_inflight;
    if (m_unregistered && m_inflight == 0)
        m_cv.notify_all();
}

template<event_type EVENT_TYPE>
inline void EventBus<EVENT_TYPE>::EventRegistration::Unregister()
{
    std::unique_lock lk(mtx);
    if (m_unregistered) return;
    m_unregistered = true;

    // call external callback without holding mutex to avoid deadlocks
    if (unregisterCB)
    {
        auto cb = unregisterCB; // copy
        lk.unlock();
        cb();
        lk.lock();
    }

    m_cv.wait(lk, [this] { return m_inflight == 0; });
}

template<event_type EVENT_TYPE>
inline void EventBus<EVENT_TYPE>::EventRegistration::DisableUnregisterCallback()
{
    std::lock_guard lk(mtx);
    unregisterCB = nullptr;
}

template<event_type EVENT_TYPE>
inline void EventBus<EVENT_TYPE>::EventRegistration::SetQueue(EventQueue<EVENT_TYPE>* queue)
{
    std::lock_guard lk(mtx);
    if (m_unregistered) return;
    m_Queue = queue;
}

template<event_type EVENT_TYPE>
template<typename WeakListT, typename SharedT>
inline void EventBus<EVENT_TYPE>::PruneAndRemove(WeakListT& list, const std::shared_ptr<SharedT>& match)
{
    for (auto it = list.begin(); it != list.end();)
    {
        if (auto s = it->lock())
        {
            if (match && s == match)
            {
                it = list.erase(it);
            }
            else
            {
                ++it;
            }
        }
        else
        {
            it = list.erase(it);
        }
    }
}

template<event_type EVENT_TYPE>
template<typename WeakListT, typename SharedT>
inline std::vector<std::shared_ptr<SharedT>> EventBus<EVENT_TYPE>::CollectLive(const WeakListT& list)
{
    std::vector<std::shared_ptr<SharedT>> out;
    for (auto const& w : list)
        if (auto s = w.lock()) out.push_back(s);
    return out;
}

template<event_type EVENT_TYPE>
inline std::shared_ptr<typename EventBus<EVENT_TYPE>::EventRegistration> EventBus<EVENT_TYPE>::RegisterProducerHelper(EventProducer<EVENT_TYPE>* producer, std::function<void()> unregisterCallBack)
{
    std::lock_guard lk(producer->m_RegisterMtx);
    // Create registration for producer (queue will be set by the bus)
    auto reg = std::make_shared<EventRegistration>(std::move(unregisterCallBack));
    producer->m_Registration.push_back(reg);
    return reg;
}

template<event_type EVENT_TYPE>
inline void EventBus<EVENT_TYPE>::UnregisterProducerHelper(EventProducer<EVENT_TYPE>* producer, std::shared_ptr<EventRegistration> reg)
{
    std::lock_guard lk(producer->m_RegisterMtx);
    PruneAndRemove<std::list<std::weak_ptr<EventRegistration>>, EventRegistration>(producer->m_Registration, reg);
}

template<event_type EVENT_TYPE>
inline std::shared_ptr<typename EventBus<EVENT_TYPE>::EventRegistration> EventBus<EVENT_TYPE>::RegisterListenerHelper(EventListener<EVENT_TYPE>* listener, std::function<void()> unregisterCallBack)
{
    std::lock_guard lk(listener->m_RegisterMtx);

    // Create the registration 
    auto reg = std::make_shared<EventRegistration>(std::move(unregisterCallBack));

    // Set the listener's own queue
    reg->SetQueue(&listener->m_EventQueue);

    listener->m_Registration.push_back(reg);
    return reg;
}

template<event_type EVENT_TYPE>
inline void EventBus<EVENT_TYPE>::UnregisterListenerHelper(EventListener<EVENT_TYPE>* listener, std::shared_ptr<EventRegistration> reg)
{
    std::lock_guard lk(listener->m_RegisterMtx);
    PruneAndRemove<std::list<std::weak_ptr<EventRegistration>>, EventRegistration>(listener->m_Registration, reg);
}