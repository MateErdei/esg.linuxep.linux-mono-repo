/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#ifndef SRC_COMMON_INCLUDE_TEST_WAITFOREVENT_H_
#define SRC_COMMON_INCLUDE_TEST_WAITFOREVENT_H_

#include <chrono>
#include <future>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:6011 6326 28182)
#endif
#include <gmock/gmock.h>
#ifdef _MSC_VER
#pragma warning(pop)
#pragma warning(disable:6326)
#endif
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

    void wait(std::size_t milliseconds = 30*1000)
    {
        std::chrono::milliseconds duration(milliseconds);
        if (m_future.wait_for(duration) != std::future_status::ready)
        {
            FAIL() << "WaitForEvent::wait(" << milliseconds << ") timed out";
        }
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
#endif /* SRC_COMMON_INCLUDE_TEST_WAITFOREVENT_H_ */
