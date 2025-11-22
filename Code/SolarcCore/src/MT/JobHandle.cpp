#include "MT/JobHandle.h"

JobHandle::JobHandle(std::shared_ptr<std::atomic<bool>> completionFlag)
    : m_Completed(completionFlag)
{
}

bool JobHandle::IsComplete() const 
{
    if (!m_Completed) {
        return true; // Invalid handles are considered "complete"
    }
    return m_Completed->load(std::memory_order_acquire);
}

void JobHandle::Wait() const 
{
    if (!m_Completed) {
        return;
    }

    // Spin-wait with exponential backoff for better performance
    constexpr int SPIN_COUNT = 4000;
    constexpr int YIELD_COUNT = 100;

    int spinCount = 0;
    int yieldCount = 0;

    while (!m_Completed->load(std::memory_order_acquire)) {
        if (spinCount < SPIN_COUNT) {
            // Tight spin loop - best for very short waits
            ++spinCount;
        }
        else if (yieldCount < YIELD_COUNT) {
            // Yield to other threads - good for short-medium waits
            std::this_thread::yield();
            ++yieldCount;
        }
        else {
            // Sleep for longer waits to avoid burning CPU
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    }
}