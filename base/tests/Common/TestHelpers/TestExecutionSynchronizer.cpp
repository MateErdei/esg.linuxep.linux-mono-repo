/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#include "TestExecutionSynchronizer.h"
namespace Tests
{

    TestExecutionSynchronizer::TestExecutionSynchronizer(int toBeNotifiedNTimes)
            : m_calledNTimes(0)
            , m_expectedCall(toBeNotifiedNTimes)
    {

    }

    void TestExecutionSynchronizer::notify()
    {
        m_calledNTimes++;
        if (m_calledNTimes >= m_expectedCall)
        {
            m_promise.set_value();
        }
    }

    bool TestExecutionSynchronizer::waitfor(int ms)
    {
        return waitfor(std::chrono::milliseconds(ms));
    }
    bool TestExecutionSynchronizer::waitfor(std::chrono::milliseconds ms)
    {
        return m_promise.get_future().wait_for(ms) == std::future_status::ready;

    }
}