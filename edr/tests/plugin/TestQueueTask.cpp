/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <modules/pluginimpl/QueueTask.h>

#include <gtest/gtest.h>
#include <future>

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