// Copyright 2022, Sophos Limited.  All rights reserved.

#include "Common/Threads/AbstractThread.h"
#include <gtest/gtest.h>

using namespace Common::Threads;

namespace
{
    class TestThread : public AbstractThread
    {
    public:
        TestThread() = default;

        void run() override
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(110));
            announceThreadStarted();
        }
    };

}

TEST(TestAbstractThread, testAnnounceThreadBlocksASecondTime)
{
    auto testThread = TestThread();
    testThread.start();
    testThread.requestStop();
    testThread.join();

    auto start = std::chrono::system_clock::now();
    testThread.start();
    auto end = std::chrono::system_clock::now();

    auto startMs = std::chrono::duration_cast<std::chrono::milliseconds>(start.time_since_epoch()).count();
    auto endMs = std::chrono::duration_cast<std::chrono::milliseconds>(end.time_since_epoch()).count();

    auto duration = endMs - startMs;

    ASSERT_GT(duration, 100);

    testThread.requestStop();
    testThread.join();
}