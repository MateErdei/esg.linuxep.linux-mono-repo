// Copyright 2022, Sophos Limited.  All rights reserved.

#include "pluginimpl/TaskQueue.h"

#include "tests/common/LogInitializedTests.h"

#include <chrono>

namespace
{
    class TestTaskQueue : public LogInitializedTests
    {
    };
}

TEST_F(TestTaskQueue, create)
{
    Plugin::TaskQueue queue;
    EXPECT_TRUE(queue.empty());
}

TEST_F(TestTaskQueue, sleepReturnsFalseIfNotStopped)
{
    using namespace std::chrono_literals;
    Plugin::TaskQueue queue;
    EXPECT_FALSE(queue.stoppableSleep(1ms));
}

TEST_F(TestTaskQueue, sleepReturnsTrueIfStopped)
{
    using namespace std::chrono_literals;
    Plugin::TaskQueue queue;
    queue.pushStop();
    EXPECT_TRUE(queue.stoppableSleep(1ms));
}