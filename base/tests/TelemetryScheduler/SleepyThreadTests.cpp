/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <TelemetryScheduler/TelemetrySchedulerImpl/SleepyThread.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::StrictMock;
using namespace TelemetrySchedulerImpl;

class SleepyThreadTests : public ::testing::Test
{
public:
    void SetUp() override
    {
    }
};

TEST_F(SleepyThreadTests, threadCompletesWhenTimeIsInTheFuture) // NOLINT
{
    using namespace std::chrono_literals;

    auto queue = std::make_shared<TaskQueue>();
    const Task task = Task::WaitToRunTelemetry;

    const std::chrono::seconds delay = 2s;
    const auto now = std::chrono::system_clock::now();
    const auto inTheFuture = now + delay;
    const size_t inTheFutureSecondsSinceEpoch = std::chrono::duration_cast<std::chrono::seconds>(inTheFuture.time_since_epoch()).count();

    SleepyThread thread(inTheFutureSecondsSinceEpoch, task, queue);
    thread.start();
    std::this_thread::sleep_for(delay + 1s);

    ASSERT_EQ(queue->pop(), task);
    ASSERT_TRUE(thread.finished());
}

TEST_F(SleepyThreadTests, threadCompletesWhenTimeIsInThePast) // NOLINT
{
    using namespace std::chrono_literals;

    auto queue = std::make_shared<TaskQueue>();
    const Task task = Task::WaitToRunTelemetry;

    const std::chrono::seconds offset = 3600s;
    const auto now = std::chrono::system_clock::now();
    const auto inThePast = now - offset;
    const size_t inThePastSecondsSinceEpoch = std::chrono::duration_cast<std::chrono::seconds>(inThePast.time_since_epoch()).count();

    SleepyThread thread(inThePastSecondsSinceEpoch, task, queue);
    thread.start();
    std::this_thread::sleep_for(1s);

    ASSERT_EQ(queue->pop(), task);
    ASSERT_TRUE(thread.finished());
}

TEST_F(SleepyThreadTests, createAndStopBeforeFinished) // NOLINT
{
    using namespace std::chrono_literals;

    auto queue = std::make_shared<TaskQueue>();
    const Task task = Task::WaitToRunTelemetry;

    const std::chrono::seconds delay = 3600s;
    const auto now = std::chrono::system_clock::now();
    const auto inTheFuture = now + delay;
    const size_t inTheFutureSecondsSinceEpoch = std::chrono::duration_cast<std::chrono::seconds>(inTheFuture.time_since_epoch()).count();

    SleepyThread thread(inTheFutureSecondsSinceEpoch, task, queue);
    thread.start();
    std::this_thread::sleep_for(1s);
    thread.requestStop();
    std::this_thread::sleep_for(1s);

    ASSERT_FALSE(thread.finished());
}
