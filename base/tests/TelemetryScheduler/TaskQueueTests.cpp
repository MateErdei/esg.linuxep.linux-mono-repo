/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

/**
 * Component tests for Telemetry Scheduler
 */

#include <TelemetryScheduler/TelemetrySchedulerImpl/TaskQueue.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <chrono>
#include <thread>

using TelemetrySchedulerImpl::Task;
using TelemetrySchedulerImpl::TaskQueue;

TEST(TaskQueueTests, pushedTaskCanBePopped) // NOLINT
{
    TaskQueue queue;
    const Task taskIn = Task::WaitToRunTelemetry;
    queue.push(taskIn);
    Task taskOut = queue.pop();
    EXPECT_EQ(taskOut, taskIn);
}

TEST(TaskQueueTests, multiplePushedTasksCanBePopped) // NOLINT
{
    TaskQueue queue;
    const std::vector<Task> tasksIn = { Task::Shutdown, Task::WaitToRunTelemetry, Task::RunTelemetry };

    for (auto task : tasksIn)
    {
        queue.push(task);
    }

    std::vector<Task> tasksOut;

    for (size_t i = 0; i < tasksIn.size(); ++i)
    {
        tasksOut.emplace_back(queue.pop());
    }

    EXPECT_EQ(tasksOut, tasksIn);
}

TEST(TaskQueueTests, popWaitsForPush) // NOLINT
{
    using namespace std::chrono;
    const milliseconds delay(100);

    TaskQueue queue;
    Task taskOut;
    milliseconds measuredDelay(0);

    std::thread popThread([&] {
      steady_clock::time_point start = steady_clock::now();
      taskOut = queue.pop();
      measuredDelay = duration_cast<milliseconds>(steady_clock::now() - start);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(delay));
    const Task taskIn = Task::WaitToRunTelemetry;
    queue.push(taskIn);

    popThread.join();

    EXPECT_EQ(taskOut, taskIn);
    EXPECT_GE(measuredDelay.count(), delay.count());
}

TEST(TaskQueueTests, pushedPriorityTaskIsPoppedFirst) // NOLINT
{
    TaskQueue queue;
    const Task priorityTask = Task::Shutdown;
    const Task normalTask = Task::WaitToRunTelemetry;

    queue.push(normalTask);
    queue.pushPriority(priorityTask);

    Task taskOut1 = queue.pop();
    EXPECT_EQ(taskOut1, priorityTask);

    Task taskOut2 = queue.pop();
    EXPECT_EQ(taskOut2, normalTask);
}
