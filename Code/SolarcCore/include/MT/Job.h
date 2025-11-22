#pragma once
#include <functional>
#include <vector>
#include <atomic>
#include <memory>
#include "MT/JobHandle.h"


struct Job 
{
    std::function<void()> task;
    std::shared_ptr<std::atomic<bool>> completionFlag;
    std::vector<JobHandle> dependencies;

    // For debugging/profiling
    const char* debugName = nullptr;

    Job() = default;

    Job(std::function<void()> t,
        std::shared_ptr<std::atomic<bool>> flag,
        std::vector<JobHandle> deps = {},
        const char* name = nullptr)
        : task(std::move(t))
        , completionFlag(flag)
        , dependencies(std::move(deps))
        , debugName(name)
    {}

    // Check if all dependencies are satisfied
    bool AreDependenciesSatisfied() const 
    {
        for (const auto& dep : dependencies) {
            if (!dep.IsComplete()) {
                return false;
            }
        }
        return true;
    }

    bool IsValid() const 
    {
        return task != nullptr && completionFlag != nullptr;
    }
};