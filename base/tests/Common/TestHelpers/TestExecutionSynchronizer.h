/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#pragma once



#include <mutex>
#include <condition_variable>


namespace Tests
{
    class TestExecutionSynchronizer
    {
        std::mutex mutex;
        std::condition_variable m_waitCondition;
        int m_calledNTimes;
        int m_expectedCall;
    public:
        explicit TestExecutionSynchronizer(int toBeNotifiedNTimes = 1);
        void notify();
        void waitfor(int ms = 500);
    };

}

