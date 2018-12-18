/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#pragma once

#include <future>
#include <condition_variable>
#include <mutex>
#include <atomic>
namespace Tests
{
    class TestExecutionSynchronizer
    {
        std::promise<void> m_promise;
        int m_calledNTimes;
        int m_expectedCall;
    public:
        explicit TestExecutionSynchronizer(int toBeNotifiedNTimes = 1);
        // we must not allow copy of the synchronizer.
        TestExecutionSynchronizer(const TestExecutionSynchronizer&) = delete;
        TestExecutionSynchronizer& operator=(const TestExecutionSynchronizer&) = delete;
        void notify();
        bool waitfor(int ms = 500);
        bool waitfor( std::chrono::milliseconds ms);
    };

    class ReentrantExecutionSynchronizer
    {
        std::atomic<int> m_counter;
        std::mutex m_FirstMutex;
        std::condition_variable m_waitsync;
    public:
        explicit ReentrantExecutionSynchronizer();
        void notify();
        void waitfor(int expectedCounter, int ms = 500);
        void waitfor( int expectedCounter, std::chrono::milliseconds ms);
    };


}

