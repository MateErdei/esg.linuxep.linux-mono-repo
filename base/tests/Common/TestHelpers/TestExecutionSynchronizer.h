/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#ifndef EVEREST_BASE_TESTEXECUTIONSYNCHRONIZER_H
#define EVEREST_BASE_TESTEXECUTIONSYNCHRONIZER_H


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
        void waitfor();
    };

}
#endif //EVEREST_BASE_TESTEXECUTIONSYNCHRONIZER_H
