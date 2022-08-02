// Copyright 2020-2022, Sophos Limited.  All rights reserved.

#include <chrono>
#include <future>

#include <gmock/gmock.h>

class WaitForEvent
{
public:
    WaitForEvent()
            : m_promise()
            , m_future(m_promise.get_future())
    {}

    void onEventNoArgs()
    {
        m_promise.set_value(true);
    }

    void wait(std::size_t milliseconds=30*1000)
    {
        std::chrono::milliseconds duration(milliseconds);
        if (m_future.wait_for(duration) != std::future_status::ready)
        {
            FAIL() << "WaitForEvent::wait(" << milliseconds << ") timed out";
        }
    }

    void waitDefault()
    {
        std::size_t milliseconds = 30L * 1000;
        wait(milliseconds);
    }

    void clear()
    {
        // this order of events avoids a "broken promise" error being set in the old m_future
        std::promise<bool> newPromise;
        m_future = newPromise.get_future();
        m_promise.swap(newPromise);
    }

private:
    std::promise<bool> m_promise;
    std::future<bool> m_future;
};
