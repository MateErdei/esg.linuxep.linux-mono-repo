// Copyright 2021-2023 Sophos Limited. All rights reserved.

#include "sdu/taskQueue/ITaskQueue.h"
#include "sdu/taskQueue/TaskQueue.h"

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <thread>

TEST(TaskQueueTests, pushedTaskCanBePopped)
{
    RemoteDiagnoseImpl::TaskQueue queue;
    const RemoteDiagnoseImpl::Task taskIn{ RemoteDiagnoseImpl::Task::TaskType::ACTION, "Action_1" };
    queue.push(taskIn);
    RemoteDiagnoseImpl::Task taskOut = queue.pop();
    EXPECT_EQ(taskOut.taskType, taskIn.taskType);
    EXPECT_EQ(taskOut.Content, taskIn.Content);
}

TEST(TaskQueueTests, multiplePushedTasksCanBePopped)
{
    RemoteDiagnoseImpl::TaskQueue queue;
    const std::vector<RemoteDiagnoseImpl::Task> tasksIn = { { RemoteDiagnoseImpl::Task::TaskType::ACTION, "" },
                                                            { RemoteDiagnoseImpl::Task::TaskType::STOP, "" },
                                                            { RemoteDiagnoseImpl::Task::TaskType::DIAGNOSE_TIMED_OUT,
                                                              "" } };

    for (auto task : tasksIn)
    {
        queue.push(task);
    }

    std::vector<RemoteDiagnoseImpl::Task> tasksOut;

    for (size_t i = 0; i < tasksIn.size(); ++i)
    {
        tasksOut.emplace_back(queue.pop());
    }

    EXPECT_EQ(tasksOut.size(), tasksIn.size());
    EXPECT_EQ(tasksOut[0].taskType, tasksIn[0].taskType);
    EXPECT_EQ(tasksOut[0].Content, tasksIn[0].Content);
    EXPECT_EQ(tasksOut[1].taskType, tasksIn[1].taskType);
    EXPECT_EQ(tasksOut[1].Content, tasksIn[1].Content);
}

TEST(TaskQueueTests, ActionTaskIsNotPoppedOffWhenDiagnoseIsProcessing)
{
    RemoteDiagnoseImpl::TaskQueue queue;
    const std::vector<RemoteDiagnoseImpl::Task> tasksIn = { { RemoteDiagnoseImpl::Task::TaskType::ACTION, "" },
                                                            { RemoteDiagnoseImpl::Task::TaskType::STOP, "" } };

    for (auto task : tasksIn)
    {
        queue.push(task);
    }

    std::vector<RemoteDiagnoseImpl::Task> tasksOut;

    for (size_t i = 0; i < tasksIn.size(); ++i)
    {
        tasksOut.emplace_back(queue.pop(true));
    }

    EXPECT_EQ(tasksOut.size(), tasksIn.size());
    EXPECT_NE(tasksOut[0].taskType, RemoteDiagnoseImpl::Task::TaskType::ACTION);
    EXPECT_NE(tasksOut[1].taskType, RemoteDiagnoseImpl::Task::TaskType::ACTION);
}

TEST(TaskQueueTests, popWaitsForPush)
{
    using namespace std::chrono;
    const milliseconds delay(1);

    RemoteDiagnoseImpl::TaskQueue queue;
    RemoteDiagnoseImpl::Task taskOut;
    std::atomic<bool> done(false);

    std::thread popThread(
        [&]
        {
            taskOut = queue.pop();
            done = true;
        });

    EXPECT_FALSE(done);

    const RemoteDiagnoseImpl::Task taskIn{ RemoteDiagnoseImpl::Task::TaskType::ACTION, "" };
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

    EXPECT_EQ(taskOut.taskType, taskIn.taskType);
    EXPECT_EQ(taskOut.Content, taskIn.Content);
}

TEST(TaskQueueTests, pushedPriorityTaskIsPoppedFirst)
{
    RemoteDiagnoseImpl::TaskQueue queue;
    const RemoteDiagnoseImpl::Task priorityTask{ RemoteDiagnoseImpl::Task::TaskType::STOP, "" };
    const RemoteDiagnoseImpl::Task normalTask{ RemoteDiagnoseImpl::Task::TaskType::ACTION, "" };

    queue.push(normalTask);
    queue.pushPriority(priorityTask);

    RemoteDiagnoseImpl::Task taskOut1 = queue.pop();
    EXPECT_EQ(taskOut1.taskType, priorityTask.taskType);
    EXPECT_EQ(taskOut1.Content, priorityTask.Content);

    RemoteDiagnoseImpl::Task taskOut2 = queue.pop();
    EXPECT_EQ(taskOut2.taskType, normalTask.taskType);
    EXPECT_EQ(taskOut2.Content, normalTask.Content);
}
