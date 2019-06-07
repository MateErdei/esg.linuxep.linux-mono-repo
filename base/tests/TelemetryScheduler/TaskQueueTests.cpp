/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <TelemetryScheduler/TelemetrySchedulerImpl/TaskQueue.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <thread>

using TelemetrySchedulerImpl::SchedulerTask;
using TelemetrySchedulerImpl::TaskQueue;

TEST(TaskQueueTests, pushedTaskCanBePopped) // NOLINT
{
    TaskQueue queue;
    const SchedulerTask taskIn = SchedulerTask::InitialWaitToRunTelemetry;
    queue.push(taskIn);
    SchedulerTask taskOut = queue.pop();
    EXPECT_EQ(taskOut, taskIn);
}

TEST(TaskQueueTests, multiplePushedTasksCanBePopped) // NOLINT
{
    TaskQueue queue;
    const std::vector<SchedulerTask> tasksIn = { SchedulerTask::Shutdown,
                                                 SchedulerTask::InitialWaitToRunTelemetry,
                                                 SchedulerTask::RunTelemetry };

    for (auto task : tasksIn)
    {
        queue.push(task);
    }

    std::vector<SchedulerTask> tasksOut;

    for (size_t i = 0; i < tasksIn.size(); ++i)
    {
        tasksOut.emplace_back(queue.pop());
    }

    EXPECT_EQ(tasksOut, tasksIn);
}

TEST(TaskQueueTests, popWaitsForPush) // NOLINT
{
    using namespace std::chrono;
    const milliseconds delay(10);

    TaskQueue queue;
    SchedulerTask taskOut;
    std::atomic<bool> done(false);

    std::thread popThread([&] {
        taskOut = queue.pop();
        done = true;
    });

    EXPECT_FALSE(done);

    const SchedulerTask taskIn = SchedulerTask::InitialWaitToRunTelemetry;
    queue.push(taskIn);
    std::this_thread::sleep_for(milliseconds(delay));

    EXPECT_TRUE(done);

    popThread.join();

    EXPECT_EQ(taskOut, taskIn);
}

TEST(TaskQueueTests, pushedPriorityTaskIsPoppedFirst) // NOLINT
{
    TaskQueue queue;
    const SchedulerTask priorityTask = SchedulerTask::Shutdown;
    const SchedulerTask normalTask = SchedulerTask::InitialWaitToRunTelemetry;

    queue.push(normalTask);
    queue.pushPriority(priorityTask);

    SchedulerTask taskOut1 = queue.pop();
    EXPECT_EQ(taskOut1, priorityTask);

    SchedulerTask taskOut2 = queue.pop();
    EXPECT_EQ(taskOut2, normalTask);
}
