#pragma once
#include "Preprocessor/API.h"
#include "Logging/LogMacros.h"
#include <thread>
#include <cstdlib>

/**
 * @brief Helper class to enforce thread ownership
 *
 * Usage:
 * class MyClass {
 *     ThreadChecker m_ThreadChecker;
 * public:
 *     void Update() {
 *         m_ThreadChecker.AssertOnOwnerThread("MyClass::Update");
 *         // ... rest of implementation
 *     }
 * };
 *
 * The thread that constructs MyClass becomes the "owner thread".
 * Any subsequent calls to Update() from other threads will abort.
 */

class SOLARC_CORE_API ThreadChecker
{
public:
    ThreadChecker()
        : m_OwnerThreadId(std::this_thread::get_id())
    {
    }

    /**
     * @brief Check if currently on the owner thread
     * @return true if on owner thread, false otherwise
     */
    bool IsOnOwnerThread() const
    {
        return std::this_thread::get_id() == m_OwnerThreadId;
    }

    /**
     * @brief Assert that we're on the owner thread, abort if not
     * @param context Optional context string for error message
     */
    void AssertOnOwnerThread(const char* context = nullptr) const
    {
        if (!IsOnOwnerThread())
        {
            if (context)
            {
                SOLARC_CRITICAL("Thread violation: {} must be called from owner thread!", context);
            }
            else
            {
                SOLARC_CRITICAL("Thread violation: Function must be called from owner thread!");
            }

            SOLARC_CRITICAL("Owner thread ID: {}", GetThreadIdString(m_OwnerThreadId));
            SOLARC_CRITICAL("Current thread ID: {}", GetThreadIdString(std::this_thread::get_id()));

            // Flush logs before aborting
            Log::FlushAll();

            std::abort();
        }
    }

    /**
     * @brief Get the owner thread ID
     * @return Thread ID of the owner thread
     */
    std::thread::id GetOwnerThreadId() const
    {
        return m_OwnerThreadId;
    }

private:
    static std::string GetThreadIdString(std::thread::id id)
    {
        std::ostringstream oss;
        oss << id;
        return oss.str();
    }

    std::thread::id m_OwnerThreadId;
};