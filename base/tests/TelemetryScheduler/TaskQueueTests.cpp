// Copyright 2019-2023 Sophos Limited. All rights reserved.

#include "TelemetryScheduler/TelemetrySchedulerImpl/TaskQueue.h"
#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <thread>

using TelemetrySchedulerImpl::SchedulerTask;
using TelemetrySchedulerImpl::TaskQueue;

TEST(TaskQueueTests, pushedTaskCanBePopped)
{
    TaskQueue queue;
    const SchedulerTask taskIn = { SchedulerTask::TaskType::InitialWaitToRunTelemetry };
    queue.push(taskIn);
    SchedulerTask taskOut = queue.pop();
    EXPECT_EQ(taskOut, taskIn);
}

TEST(TaskQueueTests, multiplePushedTasksCanBePopped)
{
    TaskQueue queue;
    const std::vector<SchedulerTask> tasksIn = { { SchedulerTask::TaskType::Shutdown },
                                                 { SchedulerTask::TaskType::InitialWaitToRunTelemetry },
                                                 { SchedulerTask::TaskType::RunTelemetry } };

    for (const auto& task : tasksIn)
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

TEST(TaskQueueTests, popWaitsForPush)
{
    using namespace std::chrono;
    const milliseconds delay(1);

    TaskQueue queue;
    SchedulerTask taskOut;
    std::atomic<bool> done(false);

    std::thread popThread(
        [&]
        {
            taskOut = queue.pop();
            done = true;
        });

    EXPECT_FALSE(done);

    const SchedulerTask taskIn = { SchedulerTask::TaskType::InitialWaitToRunTelemetry };
    queue.push(taskIn);

    for (int i = 0; i < 200; i++)
    {
        if (done)
        {
            break;
        }
        std::this_thread::sleep_for(delay);
    }

    EXPECT_TRUE(done);

    popThread.join();

    EXPECT_EQ(taskOut, taskIn);
}

TEST(TaskQueueTests, pushedPriorityTaskIsPoppedFirst)
{
    TaskQueue queue;
    const SchedulerTask priorityTask = { SchedulerTask::TaskType::Shutdown };
    const SchedulerTask normalTask = { SchedulerTask::TaskType::InitialWaitToRunTelemetry };

    queue.push(normalTask);
    queue.pushPriority(priorityTask);

    SchedulerTask taskOut1 = queue.pop();
    EXPECT_EQ(taskOut1, priorityTask);

    SchedulerTask taskOut2 = queue.pop();
    EXPECT_EQ(taskOut2, normalTask);
}
