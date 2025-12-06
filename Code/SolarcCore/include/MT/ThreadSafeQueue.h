#pragma once
#include "Preprocessor/API.h"
#include <optional>
#include <mutex>
#include <queue>
#include <condition_variable>

template<typename T>
class ThreadSafeQueue
{
public:
    ThreadSafeQueue() = default;
    virtual ~ThreadSafeQueue() = default;

    void Push(const T& value)
    {
        std::lock_guard lock(m_Mtx);
        m_Queue.push(value);
        m_CV.notify_one();
    }

    void Push(T&& value)
    {
        std::lock_guard lock(m_Mtx);
        m_Queue.push(std::move(value));
        m_CV.notify_one();
    }

    std::optional<T> TryNext()
    {
        std::lock_guard lock(m_Mtx);
        if (m_Queue.empty()) return std::nullopt;
        T item = std::move(m_Queue.front());
        m_Queue.pop();
        return item;
    }

    T WaitOnNext()
    {
        std::unique_lock lock(m_Mtx);
        m_CV.wait(lock, [this]() { return !m_Queue.empty(); });
        T item = std::move(m_Queue.front());
        m_Queue.pop();
        return item;
    }

    bool IsEmpty()
    {
        std::lock_guard lock(m_Mtx);
        return m_Queue.empty();
    }

protected:
    std::queue<T> m_Queue;
    mutable std::mutex m_Mtx;
    mutable std::condition_variable m_CV;
};