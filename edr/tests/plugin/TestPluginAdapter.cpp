/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/TelemetryHelperImpl/TelemetryHelper.h>
#include <Common/Helpers/LogInitializedTests.h>
#include <Common/UtilityImpl/TimeUtils.h>
#include <modules/pluginimpl/ApplicationPaths.h>
#include <modules/pluginimpl/PluginAdapter.h>

#include <gtest/gtest.h>
#include <future>

class TestPluginAdapterWithLogger : public LogInitializedTests{};
class TestPluginAdapterWithoutLogger : public LogOffInitializedTests{};

Plugin::Task defaultExampleTask()
{
    Plugin::Task task;
    task.m_taskType = Plugin::Task::TaskType::EXAMPLETASK;
    task.m_correlationId = "id";
    task.m_content = "a";
    return task;
}

TEST_F(TestPluginAdapterWithoutLogger, waitForTheFirstPolicyReturnTheFirstPolicyContent)
{ // NOLINT
    Plugin::QueueTask queueTask;
    std::string policyContent { "test" };
    queueTask.pushPolicy("ALC", policyContent);
    std::string policyValue = Plugin::PluginAdapter::waitForTheFirstPolicy(queueTask, std::chrono::seconds(1), 10,
                                                                           "ALC");
    EXPECT_EQ(policyValue, policyContent);
}

TEST_F(TestPluginAdapterWithoutLogger, waitForTheFirstPolicyReturnTheFirstPolicyContentAndKeepNonPolicyEntriesInTheQueue)
{ // NOLINT
    Plugin::QueueTask queueTask;
    std::string policyContent { "test" };
    Plugin::Task example = defaultExampleTask();
    Plugin::Task example2 { example };
    example2.m_content = "b";
    queueTask.push(example);
    queueTask.push(example2);
    queueTask.pushPolicy("ALC", policyContent);
    std::string policyValue = Plugin::PluginAdapter::waitForTheFirstPolicy(queueTask, std::chrono::seconds(1), 10,
                                                                           "ALC");
    EXPECT_EQ(policyValue, policyContent);
    Plugin::Task extractTask;
    queueTask.pop(extractTask, 1000);
    EXPECT_EQ(extractTask.m_content, example.m_content);
    queueTask.pop(extractTask, 1000);
    EXPECT_EQ(extractTask.m_content, example2.m_content);
}

TEST_F(
        TestPluginAdapterWithoutLogger,
        waitForTheFirstPolicyReturnTheFirstPolicyContentAndKeepNonPolicyEntriesInTheQueueButMayChangeOrderOfTasksAfter)
{ // NOLINT
    Plugin::QueueTask queueTask;
    std::string policyContent { "test" };
    Plugin::Task example = defaultExampleTask();
    Plugin::Task example2 { example };
    example2.m_content = "b";
    Plugin::Task afterPolicyTask { example };
    afterPolicyTask.m_content = "c";
    queueTask.push(example);
    queueTask.push(example2);
    queueTask.pushPolicy("ALC", policyContent);
    queueTask.push(afterPolicyTask);
    std::string policyValue = Plugin::PluginAdapter::waitForTheFirstPolicy(queueTask, std::chrono::seconds(1), 10,
                                                                           "ALC");
    EXPECT_EQ(policyValue, policyContent);
    Plugin::Task extractTask;
    queueTask.pop(extractTask, 1000);
    // first task will be the first after the policy as it was not extracted from the queue
    EXPECT_EQ(extractTask.m_content, afterPolicyTask.m_content);
    // the order before the policy is kept unchanged.
    queueTask.pop(extractTask, 1000);
    EXPECT_EQ(extractTask.m_content, example.m_content);
    queueTask.pop(extractTask, 1000);
    EXPECT_EQ(extractTask.m_content, example2.m_content);
}

TEST_F(TestPluginAdapterWithoutLogger, waitForTheFirstPolicyWillTimeoutIfNoALCPolicyIsGivenButKeepQueueEntries)
{ // NOLINT
    Plugin::QueueTask queueTask;
    Plugin::Task example = defaultExampleTask();
    Plugin::Task example2 { example };
    example2.m_content = "b";
    queueTask.push(example);
    queueTask.push(example2);
    std::string policyValue = Plugin::PluginAdapter::waitForTheFirstPolicy(queueTask, std::chrono::seconds(1), 10,
                                                                           "ALC");
    // no policy received will be returned as empty string
    EXPECT_EQ(policyValue, "");
    // the queue still has all the entries
    Plugin::Task extractTask;
    queueTask.pop(extractTask, 1000);
    EXPECT_EQ(extractTask.m_content, example.m_content);
    queueTask.pop(extractTask, 1000);
    EXPECT_EQ(extractTask.m_content, example2.m_content);
}

TEST_F(TestPluginAdapterWithoutLogger, waitForTheFirstPolicyWillDetectPolicyProvidedAfterTheCallToWait)
{ // NOLINT
    Plugin::QueueTask queueTask;
    Plugin::Task example = defaultExampleTask();
    Plugin::Task example2 { example };
    example2.m_content = "b";
    queueTask.push(example);
    queueTask.push(example2);

    auto f = std::async(std::launch::async, [&queueTask]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        queueTask.pushPolicy("ALC", "policyIntroducedAfterSomeTime");
    });

    std::string policyValue = Plugin::PluginAdapter::waitForTheFirstPolicy(queueTask, std::chrono::seconds(1), 10,
                                                                           "ALC");

    // the policy is received and the value is the expected one.
    EXPECT_EQ(policyValue, "policyIntroducedAfterSomeTime");
    // the queue still has all the entries
    Plugin::Task extractTask;
    queueTask.pop(extractTask, 1000);
    EXPECT_EQ(extractTask.m_content, example.m_content);
    queueTask.pop(extractTask, 1000);
    EXPECT_EQ(extractTask.m_content, example2.m_content);
}

TEST_F(TestPluginAdapterWithoutLogger, waitForTheFirstPolicyWillGiveUpWaitingAfterReceivingMoreThanMaxThresholdTasks)
{ // NOLINT
    Plugin::QueueTask queueTask;
    std::atomic<bool> keepRunning { true };

    auto f = std::async(std::launch::async, [&queueTask, &keepRunning]() {
        int count = 0;
        while (keepRunning)
        {
            count++;
            Plugin::Task example = defaultExampleTask();
            example.m_content = std::to_string(count);
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            queueTask.push(example);
        }
    });

    std::string policyValue = Plugin::PluginAdapter::waitForTheFirstPolicy(queueTask, std::chrono::seconds(1), 10,
                                                                           "ALC");
    keepRunning = false;
    // not policy was given, no policy should be available
    EXPECT_EQ(policyValue, "");
    // there must be at least 10 items in the queue (there could be more as by the time we change the keepRunning one
    // more task could be inserted)
    for (int i = 1; i <= 10; i++)
    {
        Plugin::Task extractTask;
        queueTask.pop(extractTask, 1000);
        EXPECT_EQ(extractTask.m_content, std::to_string(i));
    }
}