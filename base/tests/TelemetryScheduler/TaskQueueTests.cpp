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

class TaskQueueTests : public ::testing::Test
{
public:
    void SetUp() override {}

    void TearDown() override {}
};

TEST_F(TaskQueueTests, pushedTaskCanBePopped) // NOLINT
{
    TaskQueue queue;
    const Task taskIn = Task::WaitToRunTelemetry;
    queue.push(taskIn);
    Task taskOut = queue.pop();
    EXPECT_EQ(taskOut, taskIn);
}

TEST_F(TaskQueueTests, multiplePushedTasksCanBePopped) // NOLINT
{
    TaskQueue queue;
    const std::vector<Task> tasksIn = { Task::ShutdownReceived, Task::WaitToRunTelemetry, Task::RunTelemetry };

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

TEST_F(TaskQueueTests, popWaitsForPush) // NOLINT
{
    using namespace std::chrono;
    const milliseconds delay(100);

    TaskQueue queue;
    Task taskOut;
    steady_clock::time_point start = steady_clock::now();
    steady_clock::time_point end = start;
    milliseconds measuredDelay(0);

    std::thread popThread([&] {
      taskOut = queue.pop();
      end = steady_clock::now();
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(delay));
    const Task taskIn = Task::WaitToRunTelemetry;
    queue.push(taskIn);

    popThread.join();
    measuredDelay = duration_cast<milliseconds>(end - start);

    EXPECT_EQ(taskOut, taskIn);
    EXPECT_GE(measuredDelay.count(), delay.count());
}
