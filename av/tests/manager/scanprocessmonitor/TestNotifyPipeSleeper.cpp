// Copyright 2022, Sophos Limited.  All rights reserved.

#include "manager/scanprocessmonitor/NotifyPipeSleeper.h"

#include "ScanProcessMonitorMemoryAppenderUsingTests.h"

#include <chrono>

namespace
{
    class TestNotifyPipeSleeper : public ScanProcessMonitorMemoryAppenderUsingTests
    {
    };
}

using namespace plugin::manager::scanprocessmonitor;

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
    EXPECT_TRUE(sleeper.stoppableSleep(1ms));
}
