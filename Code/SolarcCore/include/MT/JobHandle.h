#pragma once
#include "Preprocessor/API.h"
#include <atomic>
#include <memory>
#include <thread>


class SOLARC_CORE_API JobHandle 
{
public:

    explicit JobHandle(std::shared_ptr<std::atomic<bool>> completionFlag);

    JobHandle() = default;

    // Check if job has completed (non-blocking)
    bool IsComplete() const;

    // Block until job completes
    void Wait() const;

    // Check if this handle is valid
    bool IsValid() const { return m_Completed != nullptr; }

private:

    std::shared_ptr<std::atomic<bool>> m_Completed;
};