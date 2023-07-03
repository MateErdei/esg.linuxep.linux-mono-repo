// Copyright 2022, Sophos Limited.  All rights reserved.

#include "modules/Common/Threads/AbstractThread.h"
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

TEST(TestAbstractThread, testAnnounceThreadBlocks)
{
    auto testThread = TestThread();
    auto start = std::chrono::steady_clock::now();
    testThread.start();
    auto end = std::chrono::steady_clock::now();

    ASSERT_GT(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count(), 100);

    testThread.requestStop();
    testThread.join();
}

TEST(TestAbstractThread, testAnnounceThreadBlocksASecondTime)
{
    auto testThread = TestThread();
    testThread.start();
    testThread.requestStop();
    testThread.join();

    auto start = std::chrono::steady_clock::now();
    testThread.start();
    auto end = std::chrono::steady_clock::now();

    ASSERT_GT(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count(), 100);

    testThread.requestStop();
    testThread.join();
}