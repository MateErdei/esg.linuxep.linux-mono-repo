/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "ALCPoliciesExample.h"

#include <Common/FileSystem/IFileSystem.h>
#include <Common/FileSystem/IFileSystemException.h>
#include <Common/Helpers/FileSystemReplaceAndRestore.h>
#include <Common/Helpers/LogInitializedTests.h>
#include <Common/Helpers/MockFileSystem.h>
#include <Common/Helpers/MockFilePermissions.h>
#include <Common/Helpers/TempDir.h>
#include <modules/pluginimpl/PluginAdapter.h>
#include <tests/googletest/googlemock/include/gmock/gmock-matchers.h>

#include <gtest/gtest.h>

class DummyServiceApli : public Common::PluginApi::IBaseServiceApi
{
public:
    void sendEvent(const std::string&, const std::string&) const override {};

    void sendStatus(const std::string&, const std::string&, const std::string&) const override {};

    void requestPolicies(const std::string&) const override {};
};

class TestablePluginAdapter : public Plugin::PluginAdapter
{
public:
    TestablePluginAdapter(std::shared_ptr<Plugin::QueueTask> queueTask) :
        Plugin::PluginAdapter(
            queueTask,
            std::unique_ptr<Common::PluginApi::IBaseServiceApi>(new DummyServiceApli()),
            std::make_shared<Plugin::PluginCallback>(queueTask))
    {

    }
    void processALCPolicy(const std::string& policy, bool firstTime)
    {
        Plugin::PluginAdapter::processALCPolicy(policy, firstTime);
    }

    Plugin::OsqueryConfigurator& osqueryConfigurator()
    {
        return Plugin::PluginAdapter::osqueryConfigurator();
    }

    void ensureMCSCanReadOldResponses()
    {
        Plugin::PluginAdapter::ensureMCSCanReadOldResponses();
    }
};
class TestPluginAdapterWithLogger : public LogInitializedTests{};
class TestPluginAdapterWithoutLogger : public LogOffInitializedTests{};

TEST_F(TestPluginAdapterWithLogger, processALCPolicyShouldInstructRestartOnChangePolicy)
{ // NOLINT
    testing::internal::CaptureStderr();

    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem> { mockFileSystem });
    EXPECT_CALL(*mockFileSystem, exists(_)).Times(7);
    EXPECT_CALL(*mockFileSystem, isFile(_)).Times(8);
    EXPECT_CALL(*mockFileSystem, writeFile(_, _)).Times(3);

    Tests::TempDir tempDir("/tmp");
    // set the config file to enable audit by default. Hence, the rest is configured by policy.
    tempDir.createFile("plugins/edr/etc/plugin.conf", "disable_auditd=1\n");
    Common::ApplicationConfiguration::applicationConfiguration().setData(
        Common::ApplicationConfiguration::SOPHOS_INSTALL, tempDir.dirPath());

    auto queueTask = std::make_shared<Plugin::QueueTask>();
    TestablePluginAdapter pluginAdapter(queueTask);

    pluginAdapter.processALCPolicy(PolicyWithoutMTRFeatureButWithSubscription(), true);
    EXPECT_TRUE(pluginAdapter.osqueryConfigurator().enableAuditDataCollection());

    // processing new policy that now has MTR feature, does not schedule a restart task
    pluginAdapter.processALCPolicy(PolicyWithMTRFeature(), false);
    EXPECT_FALSE(pluginAdapter.osqueryConfigurator().enableAuditDataCollection());
    Plugin::Task task;
    EXPECT_FALSE(queueTask->pop(task, 2));
    std::string detectFirstChange{"INFO Option to enable audit collection changed to false"};

    // new policy without MTR feature, toggle the enableAuditDataCollection and instruct restart query
    pluginAdapter.processALCPolicy(PolicyWithoutMTRFeatureOrSubscription(), false);
    EXPECT_TRUE(pluginAdapter.osqueryConfigurator().enableAuditDataCollection());
    EXPECT_FALSE(queueTask->pop(task, 2));
    std::string detectSecondChange{"INFO Option to enable audit collection changed to true"};

    // another policy without MTR feature does not toggle the enableAuditDataCollection and hence, should not restart
    // query
    pluginAdapter.processALCPolicy(PolicyWithoutMTRFeatureButWithSubscription(), false);
    EXPECT_TRUE(pluginAdapter.osqueryConfigurator().enableAuditDataCollection());
    // timeout as there is no entry in the queue
    EXPECT_FALSE(queueTask->pop(task, 1));
    std::string detectNoChange{"DEBUG Option to enable audit collection remains true"};

    std::string logMessage = testing::internal::GetCapturedStderr();
    EXPECT_THAT(logMessage, ::testing::HasSubstr(detectFirstChange));
    EXPECT_THAT(logMessage, ::testing::HasSubstr(detectSecondChange));
    EXPECT_THAT(logMessage, ::testing::HasSubstr(detectNoChange));
    EXPECT_LT( logMessage.find(detectFirstChange), logMessage.find(detectSecondChange) );
    EXPECT_LT( logMessage.find(detectSecondChange), logMessage.find(detectNoChange) );
}

Plugin::Task defaultQueryTask()
{
    Plugin::Task task;
    task.m_taskType = Plugin::Task::TaskType::Query;
    task.m_correlationId = "id";
    task.m_content = "a";
    return task;
}
const std::chrono::seconds OneSecond { 1 };

TEST_F(TestPluginAdapterWithoutLogger, waitForTheFirstALCPolicyReturnTheFirstPolicyContent)
{ // NOLINT
    Plugin::QueueTask queueTask;
    std::string policyContent { "test" };
    queueTask.pushPolicy("ALC", policyContent);
    std::string policyValue = Plugin::PluginAdapter::waitForTheFirstALCPolicy(queueTask, OneSecond, 10);
    EXPECT_EQ(policyValue, policyContent);
}
TEST_F(TestPluginAdapterWithoutLogger, waitForTheFirstALCPolicyReturnTheFirstPolicyContentAndKeepNonPolicyEntriesInTheQueue)
{ // NOLINT
    Plugin::QueueTask queueTask;
    std::string policyContent { "test" };
    Plugin::Task query = defaultQueryTask();
    Plugin::Task query2 { query };
    query2.m_content = "b";
    queueTask.push(query);
    queueTask.push(query2);
    queueTask.pushPolicy("ALC", policyContent);
    std::string policyValue = Plugin::PluginAdapter::waitForTheFirstALCPolicy(queueTask, OneSecond, 10);
    EXPECT_EQ(policyValue, policyContent);
    Plugin::Task extractTask;
    queueTask.pop(extractTask, 1000);
    EXPECT_EQ(extractTask.m_content, query.m_content);
    queueTask.pop(extractTask, 1000);
    EXPECT_EQ(extractTask.m_content, query2.m_content);
}

TEST_F(
    TestPluginAdapterWithoutLogger,
    waitForTheFirstALCPolicyReturnTheFirstPolicyContentAndKeepNonPolicyEntriesInTheQueueButMayChangeOrderOfTasksAfter)
{ // NOLINT
    Plugin::QueueTask queueTask;
    std::string policyContent { "test" };
    Plugin::Task query = defaultQueryTask();
    Plugin::Task query2 { query };
    query2.m_content = "b";
    Plugin::Task afterPolicyTask { query };
    afterPolicyTask.m_content = "c";
    queueTask.push(query);
    queueTask.push(query2);
    queueTask.pushPolicy("ALC", policyContent);
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

TEST_F(TestPluginAdapterWithoutLogger, waitForTheFirstALCPolicyWillTimeoutIfNoALCPolicyIsGivenButKeepQueueEntries)
{ // NOLINT
    Plugin::QueueTask queueTask;
    Plugin::Task query = defaultQueryTask();
    Plugin::Task query2 { query };
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

TEST_F(TestPluginAdapterWithoutLogger, waitForTheFirstALCPolicyWillDetectPolicyProvidedAfterTheCallToWait)
{ // NOLINT
    Plugin::QueueTask queueTask;
    Plugin::Task query = defaultQueryTask();
    Plugin::Task query2 { query };
    query2.m_content = "b";
    queueTask.push(query);
    queueTask.push(query2);

    auto f = std::async(std::launch::async, [&queueTask]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        queueTask.pushPolicy("ALC", "policyIntroducedAfterSomeTime");
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

TEST_F(TestPluginAdapterWithoutLogger, waitForTheFirstALCPolicyWillGiveUpWaitingAfterReceivingMoreThanMaxThresholdTasks)
{ // NOLINT
    Plugin::QueueTask queueTask;
    std::atomic<bool> keepRunning { true };

    auto f = std::async(std::launch::async, [&queueTask, &keepRunning]() {
        int count = 0;
        while (keepRunning)
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
    // there must be at least 10 items in the queue (there could be more as by the time we change the keepRunning one
    // more task could be inserted)
    for (int i = 1; i <= 10; i++)
    {
        Plugin::Task extractTask;
        queueTask.pop(extractTask, 1000);
        EXPECT_EQ(extractTask.m_content, std::to_string(i));
    }
}

class PluginAdapterWithMockFileSystem: public LogOffInitializedTests
{
public:
    PluginAdapterWithMockFileSystem()
    {
        mockFileSystem = new ::testing::NiceMock<MockFileSystem>();
        mockFilePermissions = new ::testing::NiceMock<MockFilePermissions>();
        Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>{mockFileSystem});
        Tests::replaceFilePermissions(std::unique_ptr<Common::FileSystem::IFilePermissions>{mockFilePermissions});
    }
    ~PluginAdapterWithMockFileSystem()
    {
        Tests::restoreFileSystem();
    }
    MockFileSystem * mockFileSystem;
    MockFilePermissions * mockFilePermissions;
};

TEST_F(PluginAdapterWithMockFileSystem, ensureMCSCanReadOldResponsesChownsSingleFile)
{ // NOLINT
    auto queueTask = std::make_shared<Plugin::QueueTask>();
    TestablePluginAdapter pluginAdapter(queueTask);

    std::vector<std::string> files;
    std::string filename("/opt/sophos-spl/base/mcs/response/LiveQuery_correlation_response.json");
    files.push_back(filename);

    EXPECT_CALL(*mockFileSystem, isDirectory(HasSubstr("base/mcs/response"))).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, listFiles(_)).WillOnce(Return(files));
    EXPECT_CALL(*mockFilePermissions, chown(filename, "sophos-spl-local", "sophos-spl-group"));

    pluginAdapter.ensureMCSCanReadOldResponses();
}

TEST_F(PluginAdapterWithMockFileSystem, ensureMCSCanReadOldResponsesDoesNotChownIfExceptionThrown)
{ // NOLINT
    auto queueTask = std::make_shared<Plugin::QueueTask>();
    TestablePluginAdapter pluginAdapter(queueTask);

    std::vector<std::string> files;
    std::string filename("/opt/sophos-spl/base/mcs/response/LiveQuery_correlation_response.json");
    files.push_back(filename);
    Common::FileSystem::IFileSystemException e("test exception");

    EXPECT_CALL(*mockFileSystem, isDirectory(HasSubstr("base/mcs/response"))).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, listFiles(_)).WillOnce(Throw(e));
    EXPECT_CALL(*mockFilePermissions, chown(filename, "sophos-spl-local", "sophos-spl-group")).Times(0);

    pluginAdapter.ensureMCSCanReadOldResponses();
}

TEST_F(PluginAdapterWithMockFileSystem, ensureMCSCanReadOldResponsesChownsMultipleFiles)
{ // NOLINT
    auto queueTask = std::make_shared<Plugin::QueueTask>();
    TestablePluginAdapter pluginAdapter(queueTask);

    std::vector<std::string> files;
    std::string filename1("/opt/sophos-spl/base/mcs/response/LiveQuery_correlation1_response.json");
    std::string filename2("/opt/sophos-spl/base/mcs/response/LiveQuery_correlation2_response.json");
    std::string filename3("/opt/sophos-spl/base/mcs/response/LiveQuery_correlation3_response.json");
    files.push_back(filename1);
    files.push_back(filename2);
    files.push_back(filename3);

    EXPECT_CALL(*mockFileSystem, isDirectory(HasSubstr("base/mcs/response"))).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, listFiles(_)).WillOnce(Return(files));
    EXPECT_CALL(*mockFilePermissions, chown(filename1, "sophos-spl-local", "sophos-spl-group"));
    EXPECT_CALL(*mockFilePermissions, chown(filename2, "sophos-spl-local", "sophos-spl-group"));
    EXPECT_CALL(*mockFilePermissions, chown(filename3, "sophos-spl-local", "sophos-spl-group"));

    pluginAdapter.ensureMCSCanReadOldResponses();
}

TEST_F(PluginAdapterWithMockFileSystem, ensureMCSCanReadOldResponsesDoesNotChownIfNoDirectory)
{ // NOLINT
    auto queueTask = std::make_shared<Plugin::QueueTask>();
    TestablePluginAdapter pluginAdapter(queueTask);

    EXPECT_CALL(*mockFileSystem, isDirectory(HasSubstr("base/mcs/response"))).WillOnce(Return(false));
    EXPECT_CALL(*mockFileSystem, listFiles(_)).Times(0);
    EXPECT_CALL(*mockFilePermissions, chown(_, "sophos-spl-local", "sophos-spl-group")).Times(0);

    pluginAdapter.ensureMCSCanReadOldResponses();
}

TEST_F(PluginAdapterWithMockFileSystem, ensureMCSCanReadOldResponsesDoesNothingIfDirectoryEmpty)
{ // NOLINT
    auto queueTask = std::make_shared<Plugin::QueueTask>();
    TestablePluginAdapter pluginAdapter(queueTask);
    std::vector<std::string> files;

    EXPECT_CALL(*mockFileSystem, isDirectory(HasSubstr("base/mcs/response"))).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, listFiles(_)).WillOnce(Return(files));
    EXPECT_CALL(*mockFilePermissions, chown(_, "sophos-spl-local", "sophos-spl-group")).Times(0);

    pluginAdapter.ensureMCSCanReadOldResponses();
}