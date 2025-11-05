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
    enum class DISPATCH_TYPE
    {
        QUEUED,
        DIRECT
    };

    // Dispatcher abstract
    class Dispatcher : public std::enable_shared_from_this<Dispatcher>
    {
    public:
        virtual void operator()(std::shared_ptr<const EVENT_TYPE> e) = 0;
        virtual ~Dispatcher() = default;

    protected:
        Dispatcher(DISPATCH_TYPE type) : m_DispatchType(type) {}
        DISPATCH_TYPE m_DispatchType;
        mutable std::mutex m_Mtx;
    };

    // DirectDispatcher invokes the callback synchronously under mutex
    class DirectDispatcher : public Dispatcher
    {
    public:
        using EventCallBack = std::function<void(std::shared_ptr<const EVENT_TYPE>)>;

        DirectDispatcher(const EventCallBack& cb)
            : Dispatcher(DISPATCH_TYPE::DIRECT)
            , m_CB(cb)
        {
        }

        void operator()(std::shared_ptr<const EVENT_TYPE> e) override
        {
            std::lock_guard lk(this->m_Mtx);
            if (m_CB) m_CB(std::move(e));
        }

    private:
        EventCallBack m_CB;
    };

    // QueueDispatcher enqueues into listener queue using integrated ThreadSafeQueue
    class QueueDispatcher : public Dispatcher
    {
    public:
        QueueDispatcher(EventQueue<EVENT_TYPE>* queue)
            : Dispatcher(DISPATCH_TYPE::QUEUED)
            , m_Queue(queue)
        {
            assert(m_Queue != nullptr);
        }

        void operator()(std::shared_ptr<const EVENT_TYPE> e) override
        {
            std::lock_guard lk(this->m_Mtx);
            m_Queue->Push(std::move(e));
        }

    private:
        EventQueue<EVENT_TYPE>* m_Queue;
    };

    // ---------------- DispatchLifetimeToken ----------------
    // Generic lifetime/accounting token owned by whatever needs in-flight protection
    // (listener registration, bus-level dispatcher owner, aggregator, etc).
    class DispatchLifetimeToken : public std::enable_shared_from_this<DispatchLifetimeToken>
    {
    public:
        DispatchLifetimeToken()
            : m_unregistered(false), m_inflight(0)
        {
        }

        // Attempt to enter a dispatch. Returns false if the owner is already unregistered.
        bool TryEnterDispatch()
        {
            std::lock_guard lk(mtx);
            if (m_unregistered) return false;
            ++m_inflight;
            return true;
        }

        // Exit a dispatch previously entered.
        void ExitDispatch()
        {
            std::lock_guard lk(mtx);
            --m_inflight;
            if (m_unregistered && m_inflight == 0)
                m_cv.notify_all();
        }

        // Owner calls this to mark unregistered and wait for inflight to become zero.
        void UnregisterWait()
        {
            std::unique_lock lk(mtx);
            if (m_unregistered) return;
            m_unregistered = true;
            // Wait until inflight drops to zero
            m_cv.wait(lk, [this] { return m_inflight == 0; });
        }

        void DisableUnregisterCallback()
        {
            std::lock_guard lk(mtx);
            // no explicit callback here; provided for API symmetry
            // this function exists for symmetry with Registrations if needed.
        }

    private:
        std::mutex mtx;
        std::condition_variable m_cv;
        bool m_unregistered;
        int m_inflight;
    };

    // ---------------- ProducerRegistration ----------------
    // Holds weak references to dispatchers; used by producers to forward events.
    class ProducerRegistration : public std::enable_shared_from_this<ProducerRegistration>
    {
        friend class EventBus<EVENT_TYPE>;
    public:
        explicit ProducerRegistration(std::function<void()> unregisterCBIn)
            : unregisterCB(std::move(unregisterCBIn)), m_unregistered(false), m_inflight(0) {
        }

        ~ProducerRegistration() = default;

        // Dispatch forwards to each attached dispatcher.
        void Dispatch(std::shared_ptr<const EVENT_TYPE> e);

        // Synchronous and idempotent unregistration. Will call unregisterCB unless disabled.
        void Unregister();

        // Prevent Unregister() from calling the external callback.
        void DisableUnregisterCallback();

        void AddDispatcher(std::weak_ptr<Dispatcher> dispatcher);

        void RemoveDispatcher(const std::shared_ptr<Dispatcher>& dispatcher);

        std::function<void()> unregisterCB;

    private:
        mutable std::mutex mtx;
        mutable std::condition_variable m_cv;
        bool m_unregistered;
        int m_inflight;
        std::list<std::weak_ptr<Dispatcher>> dispatchers;
    };

    // ---------------- ListenerRegistration ----------------
    // Holds single shared dispatcher (now a DispatcherBridge) to deliver events to a listener.
    // Uses a DispatchLifetimeToken for in-flight accounting.
    class ListenerRegistration : public std::enable_shared_from_this<ListenerRegistration>
    {
        friend class EventBus<EVENT_TYPE>;
    public:
        ListenerRegistration(std::function<void()> unregisterCBIn, std::shared_ptr<Dispatcher> dispatcherIn, std::shared_ptr<DispatchLifetimeToken> tokenIn)
            : unregisterCB(std::move(unregisterCBIn)), dispatcher(std::move(dispatcherIn)), lifetimeToken(std::move(tokenIn))
        {
        }

        ~ListenerRegistration() = default;

        // Dispatch forwards to the dispatcher (the dispatcher bridge performs accounting).
        void Dispatch(std::shared_ptr<const EVENT_TYPE> e)
        {
            if (dispatcher)
                (*dispatcher)(std::move(e));
        }

        // Synchronous and idempotent unregistration. Will call unregisterCB unless disabled.
        void Unregister()
        {
            std::function<void()> cb;
            {
                std::lock_guard lk(mtx);
                if (!unregisterCB && !dispatcher) return;
                cb = unregisterCB;
            }

            // Call external callback without holding mtx to avoid deadlocks / re-entry
            if (cb)
            {
                cb();
            }

            // reset dispatcher so no further calls can be invoked via this registration
            {
                std::lock_guard lk(mtx);
                dispatcher.reset();
                unregisterCB = nullptr;
            }

            // Wait for any in-flight dispatches handled by the lifetimeToken
            if (lifetimeToken)
                lifetimeToken->UnregisterWait();
        }

        // Prevent Unregister() from calling the external callback.
        void DisableUnregisterCallback()
        {
            std::lock_guard lk(mtx);
            unregisterCB = nullptr;
        }

        std::function<void()> unregisterCB;
        std::shared_ptr<Dispatcher> dispatcher;
        std::shared_ptr<DispatchLifetimeToken> lifetimeToken;

    private:
        std::mutex mtx;
    };

    // ---------------- DispatcherBridge ----------------
    // Bridge wrapper handed out by the bus. It wraps an underlying Dispatcher (Direct/Queue)
    // and a DispatchLifetimeToken so that any caller of the dispatcher participates
    // in in-flight accounting uniformly (listener, bus-level dispatchers, producers).
    class DispatcherBridge : public Dispatcher
    {
    public:
        DispatcherBridge(std::weak_ptr<DispatchLifetimeToken> token, std::shared_ptr<Dispatcher> underlying)
            : Dispatcher(DISPATCH_TYPE::DIRECT)
            , m_Token(std::move(token))
            , m_Underlying(std::move(underlying))
        {
        }

        void operator()(std::shared_ptr<const EVENT_TYPE> e) override
        {
            auto token = m_Token.lock();
            if (!token) return;

            if (!token->TryEnterDispatch()) return;

            // Call underlying dispatcher implementation (may call listener, mutate bus state, enqueue, etc.)
            if (m_Underlying)
            {
                (*m_Underlying)(std::move(e));
            }

            token->ExitDispatch();
        }

    private:
        std::weak_ptr<DispatchLifetimeToken> m_Token;
        std::shared_ptr<Dispatcher> m_Underlying;
    };

    // ------------------ Helper templates to reduce repetition ------------------
    template<typename WeakListT, typename SharedT>
    static void PruneAndRemove(WeakListT& list, const std::shared_ptr<SharedT>& match);

    template<typename WeakListT, typename SharedT>
    static std::vector<std::shared_ptr<SharedT>> CollectLive(const WeakListT& list);

    // ------------------ Registration helper methods ------------------
    std::shared_ptr<ProducerRegistration> RegisterProducerHelper(
        EventProducer<EVENT_TYPE>* producer,
        std::function<void()> unregisterCallBack);

    void UnregisterProducerHelper(
        EventProducer<EVENT_TYPE>* producer,
        std::shared_ptr<ProducerRegistration> reg);

    std::shared_ptr<ListenerRegistration> RegisterListenerHelper(
        EventListener<EVENT_TYPE>* listener,
        std::function<void()> unregisterCallBack,
        DISPATCH_TYPE dispatchType);

    void UnregisterListenerHelper(
        EventListener<EVENT_TYPE>* listener,
        std::shared_ptr<ListenerRegistration> reg);

private:
};

// ---------------- Implementation ----------------

template<event_type EVENT_TYPE>
inline void EventBus<EVENT_TYPE>::ProducerRegistration::Dispatch(std::shared_ptr<const EVENT_TYPE> e)
{
    std::vector<std::shared_ptr<Dispatcher>> strongRefs;
    {
        std::lock_guard lk(mtx);
        if (m_unregistered) return;
        for (auto it = dispatchers.begin(); it != dispatchers.end();)
        {
            if (auto sp = it->lock())
            {
                strongRefs.push_back(sp);
                ++it;
            }
            else
            {
                it = dispatchers.erase(it);
            }
        }
        ++m_inflight;
    }

    for (auto& dispatcher : strongRefs)
        (*dispatcher)(e);

    {
        std::lock_guard lk(mtx);
        --m_inflight;
        if (m_unregistered && m_inflight == 0)
            m_cv.notify_all();
    }
}

template<event_type EVENT_TYPE>
inline void EventBus<EVENT_TYPE>::ProducerRegistration::Unregister()
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

    dispatchers.clear(); // drop weak refs
    m_cv.wait(lk, [this] { return m_inflight == 0; });
}

template<event_type EVENT_TYPE>
inline void EventBus<EVENT_TYPE>::ProducerRegistration::DisableUnregisterCallback()
{
    std::lock_guard lk(mtx);
    unregisterCB = nullptr;
}

template<event_type EVENT_TYPE>
inline void EventBus<EVENT_TYPE>::ProducerRegistration::AddDispatcher(std::weak_ptr<Dispatcher> dispatcher)
{
    std::lock_guard lk(mtx);
    if (m_unregistered) return;
    dispatchers.push_back(std::move(dispatcher));
}

template<event_type EVENT_TYPE>
inline void EventBus<EVENT_TYPE>::ProducerRegistration::RemoveDispatcher(const std::shared_ptr<Dispatcher>& dispatcher)
{
    std::lock_guard lk(mtx);
    for (auto it = dispatchers.begin(); it != dispatchers.end();)
    {
        if (auto sp = it->lock())
        {
            if (sp == dispatcher) it = dispatchers.erase(it);
            else ++it;
        }
        else it = dispatchers.erase(it);
    }
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
inline std::shared_ptr<typename EventBus<EVENT_TYPE>::ProducerRegistration> EventBus<EVENT_TYPE>::RegisterProducerHelper(EventProducer<EVENT_TYPE>* producer, std::function<void()> unregisterCallBack)
{
    std::lock_guard lk(producer->m_RegisterMtx);
    auto reg = std::make_shared<ProducerRegistration>(std::move(unregisterCallBack));
    producer->m_Registration.push_back(reg);
    return reg;
}

template<event_type EVENT_TYPE>
inline void EventBus<EVENT_TYPE>::UnregisterProducerHelper(EventProducer<EVENT_TYPE>* producer, std::shared_ptr<ProducerRegistration> reg)
{
    std::lock_guard lk(producer->m_RegisterMtx);
    PruneAndRemove<std::list<std::weak_ptr<ProducerRegistration>>, ProducerRegistration>(producer->m_Registration, reg);
}

template<event_type EVENT_TYPE>
inline std::shared_ptr<typename EventBus<EVENT_TYPE>::ListenerRegistration> EventBus<EVENT_TYPE>::RegisterListenerHelper(EventListener<EVENT_TYPE>* listener, std::function<void()> unregisterCallBack, DISPATCH_TYPE dispatchType)
{
    std::lock_guard lk(listener->m_RegisterMtx);

    // Create the base dispatcher implementation (Direct or Queue).
    std::shared_ptr<Dispatcher> baseDispatcher;
    switch (dispatchType)
    {
    case DISPATCH_TYPE::DIRECT:
        baseDispatcher = std::make_shared<DirectDispatcher>(listener->m_EventCallBack);
        break;
    case DISPATCH_TYPE::QUEUED:
        baseDispatcher = std::make_shared<QueueDispatcher>(&listener->m_Queue);
        break;
    }

    // Create the registration first (dispatcher will be set momentarily)
    // Create a lifetime token owned by the registration; the DispatcherBridge will hold a weak_ptr to it.
    auto token = std::make_shared<DispatchLifetimeToken>();
    auto reg = std::make_shared<ListenerRegistration>(std::move(unregisterCallBack), nullptr, token);

    // Create the accounting-aware bridge that wraps the base dispatcher and references the token
    auto bridge = std::make_shared<DispatcherBridge>(std::weak_ptr<DispatchLifetimeToken>(token), baseDispatcher);

    // Install the bridge into the registration and register the registration with the listener
    reg->dispatcher = bridge;
    listener->m_Registration.push_back(reg);
    return reg;
}

template<event_type EVENT_TYPE>
inline void EventBus<EVENT_TYPE>::UnregisterListenerHelper(EventListener<EVENT_TYPE>* listener, std::shared_ptr<ListenerRegistration> reg)
{
    std::lock_guard lk(listener->m_RegisterMtx);
    PruneAndRemove<std::list<std::weak_ptr<ListenerRegistration>>, ListenerRegistration>(listener->m_Registration, reg);
}
