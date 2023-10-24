// Copyright 2020-2023 Sophos Limited. All rights reserved.

#include "pluginimpl/QueueTask.h"

#include <gtest/gtest.h>
#include <future>
#include <thread>

TEST(TestQueueTask, popReturnsFalseWhenTImeoutReached)
{
    Plugin::QueueTask queueTask;
    Plugin::Task expectedTask;
    Plugin::Task task;
    bool return_value = queueTask.pop(task, 1);
    ASSERT_FALSE(return_value);

}

TEST(TestQueueTask, popReturnsTrueWhenTaskIsPopped)
{
    Plugin::QueueTask queueTask;
    Plugin::Task task;
    queueTask.pushStop();
    bool return_value = queueTask.pop(task, 1);
    ASSERT_TRUE(return_value);
    ASSERT_EQ(task.m_taskType,Plugin::Task::TaskType::STOP);
}

TEST(TestQueueTask, popReturnsTrueWhenTaskIsPushedDuringWait)
{
    Plugin::QueueTask queueTask;
    Plugin::Task task;
    auto pushThread = std::async(std::launch::async, [&queueTask]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        queueTask.pushStop();
    });
    bool return_value = queueTask.pop(task, 1);


    ASSERT_TRUE(return_value);
    ASSERT_EQ(task.m_taskType,Plugin::Task::TaskType::STOP);
}

TEST(TestQueueTask, pushIfNotAlreadyInQueue)
{
    Plugin::QueueTask queueTask;
    Plugin::Task task1;
    Plugin::Task task2;
    Plugin::Task task3;
    task1.m_taskType = Plugin::Task::TaskType::STOP;
    task1.m_content = "1";
    task2.m_taskType = Plugin::Task::TaskType::QUEUE_OSQUERY_RESTART;
    task2.m_content = "2";
    task3.m_taskType = Plugin::Task::TaskType::STOP;
    task3.m_content = "3";
    queueTask.pushIfNotAlreadyInQueue(task1);
    queueTask.pushIfNotAlreadyInQueue(task2);
    queueTask.pushIfNotAlreadyInQueue(task3);

    Plugin::Task task;
    ASSERT_TRUE(queueTask.pop(task, 1));
    ASSERT_EQ(task.m_content, "1");
    ASSERT_EQ(task.m_taskType, Plugin::Task::TaskType::STOP);

    ASSERT_TRUE(queueTask.pop(task, 1));
    ASSERT_EQ(task.m_content, "2");
    ASSERT_EQ(task.m_taskType, Plugin::Task::TaskType::QUEUE_OSQUERY_RESTART);

    ASSERT_FALSE(queueTask.pop(task, 1));
}

TEST(TestQueueTask, pushOsqueryRestart)
{
    Plugin::QueueTask queueTask;
    queueTask.pushOsqueryRestart("First reason to restart");
    queueTask.pushOsqueryRestart("Second reason to restart");
    Plugin::Task task;
    ASSERT_TRUE(queueTask.pop(task, 1));
    ASSERT_EQ(task.m_content, "First reason to restart");
    ASSERT_EQ(task.m_taskType, Plugin::Task::TaskType::QUEUE_OSQUERY_RESTART);
    ASSERT_FALSE(queueTask.pop(task, 1));
}