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
        std::unique_lock<std::mutex> lock(mutex);
        m_calledNTimes++;
        if (m_calledNTimes >= m_expectedCall)
        {
            m_waitCondition.notify_all();
        }
    }

    void TestExecutionSynchronizer::waitfor()
    {
        std::unique_lock<std::mutex> lock(mutex);
        m_waitCondition.wait_for(lock, std::chrono::milliseconds(500));

    }
}