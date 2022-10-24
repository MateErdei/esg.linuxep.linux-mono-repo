// Copyright 2022, Sophos Limited.  All rights reserved.

#include "modules/common/NotifyPipeSleeper.h"
#include <tests/common/LogInitializedTests.h>

#include <chrono>

namespace
{
    class TestNotifyPipeSleeper : public LogInitializedTests
    {
    };
}

using namespace common;

TEST_F(TestNotifyPipeSleeper, construction)
{
    Common::Threads::NotifyPipe pipe;
    NotifyPipeSleeper sleeper(pipe);
}

TEST_F(TestNotifyPipeSleeper, sleepWithoutStop)
{
    using namespace std::chrono_literals;
    Common::Threads::NotifyPipe pipe;
    NotifyPipeSleeper sleeper(pipe);
    EXPECT_FALSE(sleeper.stoppableSleep(1ms));
}

TEST_F(TestNotifyPipeSleeper, sleepWithStop)
{
    using namespace std::chrono_literals;
    Common::Threads::NotifyPipe pipe;
    NotifyPipeSleeper sleeper(pipe);
    pipe.notify();
    EXPECT_TRUE(sleeper.stoppableSleep(10s));
}
