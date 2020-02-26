/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "ALCPoliciesExample.h"
#include <modules/pluginimpl/PluginAdapter.h>
#include <Common/Logging/ConsoleLoggingSetup.h>
#include <Common/FileSystem/IFileSystem.h>
#include <Common/Helpers/FileSystemReplaceAndRestore.h>
#include <Common/Helpers/MockFileSystem.h>

#include <gtest/gtest.h>
#include <tests/googletest/googlemock/include/gmock/gmock-matchers.h>
#include <modules/osqueryclient/OsqueryProcessor.h>
#include <Common/Helpers/TempDir.h>

class DummyServiceApli : public Common::PluginApi::IBaseServiceApi
{
public:
    void sendEvent(const std::string&, const std::string&) const override {};

    void sendStatus(
            const std::string& ,
            const std::string& ,
            const std::string& ) const override {};

    void requestPolicies(const std::string& ) const override {};


};
class DummyQuery : public livequery::IQueryProcessor
{
    livequery::QueryResponse query(const std::string&) {
        throw std::runtime_error("not implemented");
    };
};

class TestablePluginAdapter : public Plugin::PluginAdapter{
public:
    TestablePluginAdapter(std::shared_ptr<Plugin::QueueTask> queueTask): Plugin::PluginAdapter(
                    queueTask,
                    std::unique_ptr<Common::PluginApi::IBaseServiceApi>(new DummyServiceApli()),
                    std::make_shared<Plugin::PluginCallback>(queueTask),
                    std::unique_ptr<livequery::IQueryProcessor>(new DummyQuery()),
                    std::unique_ptr<livequery::IResponseDispatcher>(new livequery::ResponseDispatcher{})
            )
    {

    }
    void processALCPolicy(const std::string &policy, bool firstTime)
    {
        Plugin::PluginAdapter::processALCPolicy(policy, firstTime);
    }

    Plugin::OsqueryConfigurator& osqueryConfigurator()
    {
        return Plugin::PluginAdapter::osqueryConfigurator();
    }

};

TEST(TestPluginAdapter,  processALCPolicyShouldInstructRestartOnChangePolicy) { // NOLINT
    Tests::TempDir tempDir("/tmp");
    // set the config file to enable audit by default. Hence, the rest is configured by policy.
    tempDir.createFile("plugins/edr/etc/plugin.conf", "disable_auditd=1\n");
    Common::ApplicationConfiguration::applicationConfiguration().setData(Common::ApplicationConfiguration::SOPHOS_INSTALL, tempDir.dirPath());

    auto queueTask = std::make_shared<Plugin::QueueTask>();
    TestablePluginAdapter pluginAdapter(queueTask);

    pluginAdapter.processALCPolicy(PolicyWithoutMTRFeatureButWithSubscription(), true);
    EXPECT_TRUE( pluginAdapter.osqueryConfigurator().enableAuditDataCollection() );

    // processing new policy that now has MTR feature, update queue to restart osquery
    pluginAdapter.processALCPolicy(PolicyWithMTRFeature(), false);
    EXPECT_FALSE( pluginAdapter.osqueryConfigurator().enableAuditDataCollection() );
    Plugin::Task task;
    EXPECT_TRUE(queueTask->pop(task, 2));
    EXPECT_EQ( task.m_taskType, Plugin::Task::TaskType::RESTARTOSQUERY);

    // new policy without MTR feature, toggle the enableAuditDataCollection and instruct restart query
    pluginAdapter.processALCPolicy(PolicyWithoutMTRFeatureOrSubscription(), false);
    EXPECT_TRUE( pluginAdapter.osqueryConfigurator().enableAuditDataCollection() );
    EXPECT_TRUE(queueTask->pop(task, 2));
    EXPECT_EQ( task.m_taskType, Plugin::Task::TaskType::RESTARTOSQUERY);

    // another policy without MTR feature does not toggle the enableAuditDataCollection and hence, should not restart query
    pluginAdapter.processALCPolicy(PolicyWithoutMTRFeatureButWithSubscription(), false);
    EXPECT_TRUE( pluginAdapter.osqueryConfigurator().enableAuditDataCollection() );
    // timeout as there is no entry in the queue
    EXPECT_FALSE(queueTask->pop(task, 1));
}


Plugin::Task defaultQueryTask()
{
    Plugin::Task task;
    task.m_taskType = Plugin::Task::TaskType::Query;
    task.m_correlationId = "id";
    task.m_content = "a";
    return task;

}
const std::chrono::seconds OneSecond{1};

TEST(TestPluginAdapter,  waitForTheFirstALCPolicyReturnTheFirstPolicyContent) { // NOLINT
    Plugin::QueueTask queueTask;
    std::string policyContent{"test"};
    queueTask.pushPolicy(policyContent);
    std::string policyValue = Plugin::PluginAdapter::waitForTheFirstALCPolicy(queueTask, OneSecond, 10);
    EXPECT_EQ(policyValue, policyContent);
}
TEST(TestPluginAdapter,  waitForTheFirstALCPolicyReturnTheFirstPolicyContentAndKeepNonPolicyEntriesInTheQueue) { // NOLINT
    Plugin::QueueTask queueTask;
    std::string policyContent{"test"};
    Plugin::Task query = defaultQueryTask();
    Plugin::Task query2{query};
    query2.m_content = "b";
    queueTask.push(query);
    queueTask.push(query2);
    queueTask.pushPolicy(policyContent);
    std::string policyValue = Plugin::PluginAdapter::waitForTheFirstALCPolicy(queueTask, OneSecond, 10);
    EXPECT_EQ(policyValue, policyContent);
    Plugin::Task extractTask;
    queueTask.pop(extractTask, 1000);
    EXPECT_EQ(extractTask.m_content, query.m_content);
    queueTask.pop(extractTask, 1000);
    EXPECT_EQ(extractTask.m_content, query2.m_content);
}

TEST(TestPluginAdapter,  waitForTheFirstALCPolicyReturnTheFirstPolicyContentAndKeepNonPolicyEntriesInTheQueueButMayChangeOrderOfTasksAfter) { // NOLINT
    Plugin::QueueTask queueTask;
    std::string policyContent{"test"};
    Plugin::Task query = defaultQueryTask();
    Plugin::Task query2{query};
    query2.m_content = "b";
    Plugin::Task afterPolicyTask{query};
    afterPolicyTask.m_content = "c";
    queueTask.push(query);
    queueTask.push(query2);
    queueTask.pushPolicy(policyContent);
    queueTask.push(afterPolicyTask);
    std::string policyValue = Plugin::PluginAdapter::waitForTheFirstALCPolicy(queueTask, OneSecond, 10);
    EXPECT_EQ(policyValue, policyContent);
    Plugin::Task extractTask;
    queueTask.pop(extractTask, 1000);
    // first task will be the first after the policy as it was not extracted from the queue
    EXPECT_EQ(extractTask.m_content, afterPolicyTask.m_content);
    // the order before the policy is kept unchanged.
    queueTask.pop(extractTask, 1000);
    EXPECT_EQ(extractTask.m_content, query.m_content);
    queueTask.pop(extractTask, 1000);
    EXPECT_EQ(extractTask.m_content, query2.m_content);
}

TEST(TestPluginAdapter,  waitForTheFirstALCPolicyWillTimeoutIfNoALCPolicyIsGivenButKeepQueueEntries) { // NOLINT
    Plugin::QueueTask queueTask;
    Plugin::Task query = defaultQueryTask();
    Plugin::Task query2{query};
    query2.m_content = "b";
    queueTask.push(query);
    queueTask.push(query2);
    std::string policyValue = Plugin::PluginAdapter::waitForTheFirstALCPolicy(queueTask, OneSecond, 10);
    // no policy received will be returned as empty string
    EXPECT_EQ(policyValue, "");
    // the queue still has all the entries
    Plugin::Task extractTask;
    queueTask.pop(extractTask, 1000);
    EXPECT_EQ(extractTask.m_content, query.m_content);
    queueTask.pop(extractTask, 1000);
    EXPECT_EQ(extractTask.m_content, query2.m_content);
}

TEST(TestPluginAdapter,  waitForTheFirstALCPolicyWillDetectPolicyProvidedAfterTheCallToWait) { // NOLINT
    Plugin::QueueTask queueTask;
    Plugin::Task query = defaultQueryTask();
    Plugin::Task query2{query};
    query2.m_content = "b";
    queueTask.push(query);
    queueTask.push(query2);

    auto f = std::async(std::launch::async, [&queueTask](){
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        queueTask.pushPolicy("policyIntroducedAfterSomeTime");
    });

    std::string policyValue = Plugin::PluginAdapter::waitForTheFirstALCPolicy(queueTask, OneSecond, 10);

    // the policy is received and the value is the expected one.
    EXPECT_EQ(policyValue, "policyIntroducedAfterSomeTime");
    // the queue still has all the entries
    Plugin::Task extractTask;
    queueTask.pop(extractTask, 1000);
    EXPECT_EQ(extractTask.m_content, query.m_content);
    queueTask.pop(extractTask, 1000);
    EXPECT_EQ(extractTask.m_content, query2.m_content);
}

TEST(TestPluginAdapter,  waitForTheFirstALCPolicyWillGiveUpWaitingAfterReceivingMoreThanMaxThresholdTasks) { // NOLINT
    Plugin::QueueTask queueTask;
    std::atomic<bool> keepRunning{true};

    auto f = std::async(std::launch::async, [&queueTask, &keepRunning](){
        int count = 0;
        while(keepRunning)
        {
            count++;
            Plugin::Task query = defaultQueryTask();
            query.m_content = std::to_string(count);
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            queueTask.push(query);
        }
    });

    std::string policyValue = Plugin::PluginAdapter::waitForTheFirstALCPolicy(queueTask, OneSecond, 10);
    keepRunning = false;
    // not policy was given, no policy should be available
    EXPECT_EQ(policyValue, "");
    // there must be at least 10 items in the queue (there could be more as by the time we change the keepRunning one more task could be inserted)
    for(int i=1; i<=10; i++)
    {
        Plugin::Task extractTask;
        queueTask.pop(extractTask, 1000);
        EXPECT_EQ(extractTask.m_content, std::to_string(i));
    }
}

