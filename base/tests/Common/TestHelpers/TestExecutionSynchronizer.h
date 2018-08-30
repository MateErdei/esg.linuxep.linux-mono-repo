/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#pragma once

#include <future>

namespace Tests
{
    class TestExecutionSynchronizer
    {
        std::promise<void> m_promise;
        int m_calledNTimes;
        int m_expectedCall;
    public:
        explicit TestExecutionSynchronizer(int toBeNotifiedNTimes = 1);
        void notify();
        bool waitfor(int ms = 500);
        bool waitfor( std::chrono::milliseconds ms);
    };

}

