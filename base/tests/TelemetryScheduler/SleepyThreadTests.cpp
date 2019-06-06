/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <TelemetryScheduler/TelemetrySchedulerImpl/SleepyThread.h>
#include <TelemetryScheduler/TelemetrySchedulerImpl/TaskQueue.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::StrictMock;
using namespace TelemetrySchedulerImpl;

class SleepyThreadTests : public ::testing::Test
{
public:
    void SetUp() override {}
};

TEST_F(SleepyThreadTests, threadCompletesWhenTimeIsInTheFuture) // NOLINT
{
    using namespace std::chrono_literals;

    auto queue = std::make_shared<TaskQueue>();
    const SchedulerTask task = SchedulerTask::WaitToRunTelemetry;

    const std::chrono::seconds delay = 2s;
    const auto now = std::chrono::system_clock::now();
    const auto inTheFuture = now + delay;

    SleepyThread thread(inTheFuture, task, queue);
    thread.start();
    std::this_thread::sleep_for(delay + 1s);

    ASSERT_EQ(queue->pop(), task);
    ASSERT_TRUE(thread.finished());
}

TEST_F(SleepyThreadTests, threadCompletesWhenTimeIsInThePast) // NOLINT
{
    using namespace std::chrono_literals;

    auto queue = std::make_shared<TaskQueue>();
    const SchedulerTask task = SchedulerTask::WaitToRunTelemetry;

    const std::chrono::seconds offset = 3600s;
    const auto now = std::chrono::system_clock::now();
    const auto inThePast = now - offset;

    SleepyThread thread(inThePast, task, queue);
    thread.start();
    std::this_thread::sleep_for(1s);

    ASSERT_EQ(queue->pop(), task);
    ASSERT_TRUE(thread.finished());
}

TEST_F(SleepyThreadTests, createAndStopBeforeFinished) // NOLINT
{
    using namespace std::chrono_literals;

    auto queue = std::make_shared<TaskQueue>();
    const SchedulerTask task = SchedulerTask::WaitToRunTelemetry;

    const std::chrono::seconds delay = 3600s;
    const auto now = std::chrono::system_clock::now();
    const auto inTheFuture = now + delay;

    SleepyThread thread(inTheFuture, task, queue);
    thread.start();
    std::this_thread::sleep_for(1s);
    thread.requestStop();
    std::this_thread::sleep_for(1s);

    ASSERT_FALSE(thread.finished());
}
