// Copyright 2020-2022, Sophos Limited.  All rights reserved.

#include <chrono>
#include <future>

#include <gmock/gmock.h>

using namespace std::chrono_literals;

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

    void wait(std::chrono::milliseconds duration = 30s)
    {
        if (m_future.wait_for(duration) != std::future_status::ready)
        {
            FAIL() << "WaitForEvent::wait(" << duration.count() << "ms) timed out";
        }
    }

    void waitDefault()
    {
        wait();
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

ACTION_P(triggerEvent, waitForEvent) { waitForEvent->onEventNoArgs(); }
ACTION_P(waitForEvent, waitForEvent) { waitForEvent->wait(); }
ACTION_P2(waitForEvent, waitForEvent, duration) { waitForEvent->wait(duration); }