/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "ALCPoliciesExample.h"

#include <Common/FileSystem/IFileSystem.h>
#include <Common/FileSystem/IFileSystemException.h>
#include <Common/TelemetryHelperImpl/TelemetryHelper.h>
#include <Common/Helpers/FileSystemReplaceAndRestore.h>
#include <Common/Helpers/LogInitializedTests.h>
#include <Common/Helpers/MockFileSystem.h>
#include <Common/Helpers/MockFilePermissions.h>
#include <Common/Helpers/TempDir.h>
#include <Common/UtilityImpl/TimeUtils.h>
#include <Common/XmlUtilities/AttributesMap.h>
#include <pluginimpl/PluginUtils.h>
#include <modules/pluginimpl/ApplicationPaths.h>
#include <modules/pluginimpl/PluginAdapter.h>
#include <modules/pluginimpl/LiveQueryPolicyParser.h>
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

    void setLiveQueryRevID(std::string revId)
    {
        m_liveQueryRevId = revId;
    }
    void setLiveQueryStatus(std::string status)
    {
        m_liveQueryStatus = status;
    }
    std::string getLiveQueryStatus()
    {
        return m_liveQueryStatus;
    }
    std::string getLiveQueryRevID()
    {
        return m_liveQueryRevId;
    }
    unsigned int getLiveQueryDataLimit()
    {
        return m_dataLimit;
    }

    void processLiveQueryPolicy(const std::string& policy)
    {
        Plugin::PluginAdapter::processLiveQueryPolicy(policy, false);
    }

    void processFlags(const std::string& policy)
    {
        Plugin::PluginAdapter::processFlags(policy, false);
    }

    void setDataLmit(const int& limit)
    {
        m_loggerExtensionPtr->setDataLimit(limit);
    }

    int getDataLmit()
    {
        return m_loggerExtensionPtr->getDataLimit();
    }

    void setScheduleEpoch(time_t scheduleEpoch)
    {
        m_scheduleEpoch.setValue(scheduleEpoch);
    }
    time_t getScheduleEpochDuration()
    {
        return SCHEDULE_EPOCH_DURATION;
    }
};
class TestPluginAdapterWithLogger : public LogInitializedTests{};
class TestPluginAdapterWithoutLogger : public LogOffInitializedTests{};

TEST_F(TestPluginAdapterWithLogger, processALCPolicyShouldInstructRestartOnChangePolicy)
{ // NOLINT
    testing::internal::CaptureStderr();

    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem> { mockFileSystem });
    EXPECT_CALL(*mockFileSystem, exists(_)).Times(10);
    EXPECT_CALL(*mockFileSystem, isFile(_)).Times(8);
    EXPECT_CALL(*mockFileSystem, writeFile(_, _)).Times(5);

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
    task.m_taskType = Plugin::Task::TaskType::QUERY;
    task.m_correlationId = "id";
    task.m_content = "a";
    return task;
}
const std::chrono::seconds OneSecond { 1 };

TEST_F(TestPluginAdapterWithoutLogger, waitForTheFirstPolicyReturnTheFirstPolicyContent)
{ // NOLINT
    Plugin::QueueTask queueTask;
    std::string policyContent { "test" };
    queueTask.pushPolicy("ALC", policyContent);
    std::string policyValue = Plugin::PluginAdapter::waitForTheFirstPolicy(queueTask, OneSecond, 10,
                                                                           "ALC");
    EXPECT_EQ(policyValue, policyContent);
}
TEST_F(TestPluginAdapterWithoutLogger, waitForTheFirstPolicyReturnTheFirstPolicyContentAndKeepNonPolicyEntriesInTheQueue)
{ // NOLINT
    Plugin::QueueTask queueTask;
    std::string policyContent { "test" };
    Plugin::Task query = defaultQueryTask();
    Plugin::Task query2 { query };
    query2.m_content = "b";
    queueTask.push(query);
    queueTask.push(query2);
    queueTask.pushPolicy("ALC", policyContent);
    std::string policyValue = Plugin::PluginAdapter::waitForTheFirstPolicy(queueTask, OneSecond, 10,
                                                                           "ALC");
    EXPECT_EQ(policyValue, policyContent);
    Plugin::Task extractTask;
    queueTask.pop(extractTask, 1000);
    EXPECT_EQ(extractTask.m_content, query.m_content);
    queueTask.pop(extractTask, 1000);
    EXPECT_EQ(extractTask.m_content, query2.m_content);
}

TEST_F(
    TestPluginAdapterWithoutLogger,
    waitForTheFirstPolicyReturnTheFirstPolicyContentAndKeepNonPolicyEntriesInTheQueueButMayChangeOrderOfTasksAfter)
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
    std::string policyValue = Plugin::PluginAdapter::waitForTheFirstPolicy(queueTask, OneSecond, 10,
                                                                           "ALC");
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

TEST_F(TestPluginAdapterWithoutLogger, waitForTheFirstPolicyWillTimeoutIfNoALCPolicyIsGivenButKeepQueueEntries)
{ // NOLINT
    Plugin::QueueTask queueTask;
    Plugin::Task query = defaultQueryTask();
    Plugin::Task query2 { query };
    query2.m_content = "b";
    queueTask.push(query);
    queueTask.push(query2);
    std::string policyValue = Plugin::PluginAdapter::waitForTheFirstPolicy(queueTask, OneSecond, 10,
                                                                           "ALC");
    // no policy received will be returned as empty string
    EXPECT_EQ(policyValue, "");
    // the queue still has all the entries
    Plugin::Task extractTask;
    queueTask.pop(extractTask, 1000);
    EXPECT_EQ(extractTask.m_content, query.m_content);
    queueTask.pop(extractTask, 1000);
    EXPECT_EQ(extractTask.m_content, query2.m_content);
}

TEST_F(TestPluginAdapterWithoutLogger, waitForTheFirstPolicyWillDetectPolicyProvidedAfterTheCallToWait)
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

    std::string policyValue = Plugin::PluginAdapter::waitForTheFirstPolicy(queueTask, OneSecond, 10,
                                                                           "ALC");

    // the policy is received and the value is the expected one.
    EXPECT_EQ(policyValue, "policyIntroducedAfterSomeTime");
    // the queue still has all the entries
    Plugin::Task extractTask;
    queueTask.pop(extractTask, 1000);
    EXPECT_EQ(extractTask.m_content, query.m_content);
    queueTask.pop(extractTask, 1000);
    EXPECT_EQ(extractTask.m_content, query2.m_content);
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
            Plugin::Task query = defaultQueryTask();
            query.m_content = std::to_string(count);
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            queueTask.push(query);
        }
    });

    std::string policyValue = Plugin::PluginAdapter::waitForTheFirstPolicy(queueTask, OneSecond, 10,
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

class PluginAdapterWithStrictMockFileSystem: public LogOffInitializedTests
{
public:
    PluginAdapterWithStrictMockFileSystem()
    {
        mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
        mockFilePermissions = new ::testing::StrictMock<MockFilePermissions>();
        Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>{mockFileSystem});
        Tests::replaceFilePermissions(std::unique_ptr<Common::FileSystem::IFilePermissions>{mockFilePermissions});
    }
    ~PluginAdapterWithStrictMockFileSystem()
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

TEST_F(PluginAdapterWithMockFileSystem, testProcessLiveQueryCustomQueries)
{
    std::string liveQueryPolicy100 = "<?xml version=\"1.0\"?>\n"
                                     "<policy type=\"LiveQuery\" RevID=\"100\" policyType=\"56\">\n"
                                     "    <configuration>\n"
                                     "        <scheduled>\n"
                                     "            <dailyDataLimit>250000000</dailyDataLimit>\n"
                                     "            <queryPacks>\n"
                                     "                <queryPack id=\"queryPackId\" />\n"
                                     "            </queryPacks>\n"
                                     "            <customQueries>\n"
                                     "                  <customQuery queryName=\"blah\">\n"
                                     "                      <description>basic query</description>\n"
                                     "                      <query>SELECT * FROM stuff</query>\n"
                                     "                      <interval>10</interval>\n"
                                     "                      <tag>DataLake</tag>\n"
                                     "                      <removed>false</removed>\n"
                                     "                      <denylist>false</denylist>\n"
                                     "                  </customQuery>\n"
                                     "                  <customQuery queryName=\"blah2\">\n"
                                     "                      <description>a different basic query</description>\n"
                                     "                      <query>SELECT * FROM otherstuff</query>\n"
                                     "                      <interval>5</interval>\n"
                                     "                      <tag>stream</tag>\n"
                                     "                      <removed>true</removed>\n"
                                     "                      <denylist>true</denylist>\n"
                                     "                  </customQuery>\n"
                                     "            </customQueries>\n"
                                     "        </scheduled>\n"
                                     "    </configuration>\n"
                                     "</policy>";

    std::string expected =  "{"
                                "\"schedule\":{"
                                    "\"blah\":{"
                                        "\"denylist\":\"false\","
                                        "\"description\":\"basic query\","
                                        "\"interval\":\"10\","
                                        "\"query\":\"SELECT * FROM stuff\","
                                        "\"removed\":\"false\","
                                        "\"tag\":\"DataLake\""
                                    "},"
                                    "\"blah2\":{"
                                       "\"denylist\":\"true\","
                                       "\"description\":\"a different basic query\","
                                       "\"interval\":\"5\","
                                       "\"query\":\"SELECT * FROM otherstuff\","
                                       "\"removed\":\"true\","
                                       "\"tag\":\"stream\""
                                    "}"
                                "}"
                            "}";

    const std::string PLUGIN_VAR_DIR = Plugin::varDir();
    EXPECT_CALL(*mockFileSystem, writeFile(PLUGIN_VAR_DIR + "/persist-xdrDataUsage", _));
    EXPECT_CALL(*mockFileSystem, writeFile(PLUGIN_VAR_DIR + "/persist-xdrScheduleEpoch", _));
    EXPECT_CALL(*mockFileSystem, writeFile(PLUGIN_VAR_DIR + "/persist-xdrPeriodTimestamp", _));
    EXPECT_CALL(*mockFileSystem, writeFile(PLUGIN_VAR_DIR + "/persist-xdrLimitHit", _));
    EXPECT_CALL(*mockFileSystem, writeFile(PLUGIN_VAR_DIR + "/persist-xdrPeriodInSeconds", _));
    EXPECT_CALL(*mockFileSystem, writeFile(Plugin::edrConfigFilePath(),"running_mode=0\n"));
    EXPECT_CALL(*mockFileSystem, writeFile(Plugin::osqueryCustomConfigFilePath(),expected));

    auto queueTask = std::make_shared<Plugin::QueueTask>();
    TestablePluginAdapter pluginAdapter(queueTask);

    pluginAdapter.processLiveQueryPolicy(liveQueryPolicy100);
}

TEST_F(PluginAdapterWithMockFileSystem, testCustomQueryConfigIsNotRemovedWhenWeFailToReadLiveQueryPolicy)
{
    auto queueTask = std::make_shared<Plugin::QueueTask>();
    TestablePluginAdapter pluginAdapter(queueTask);

    std::string liveQueryPolicy100 = "garbage";
    const std::string PLUGIN_VAR_DIR = Plugin::varDir();
    EXPECT_CALL(*mockFileSystem, writeFile(PLUGIN_VAR_DIR + "/persist-xdrDataUsage", _));
    EXPECT_CALL(*mockFileSystem, writeFile(PLUGIN_VAR_DIR + "/persist-xdrScheduleEpoch", _));
    EXPECT_CALL(*mockFileSystem, writeFile(PLUGIN_VAR_DIR + "/persist-xdrPeriodTimestamp", _));
    EXPECT_CALL(*mockFileSystem, writeFile(PLUGIN_VAR_DIR + "/persist-xdrLimitHit", _));
    EXPECT_CALL(*mockFileSystem, writeFile(PLUGIN_VAR_DIR + "/persist-xdrPeriodInSeconds", _));
    EXPECT_CALL(*mockFileSystem, removeFileOrDirectory(Plugin::osqueryCustomConfigFilePath())).Times(0);
    pluginAdapter.processLiveQueryPolicy(liveQueryPolicy100);
}

TEST_F(PluginAdapterWithMockFileSystem, testSerializeLiveQueryStatusGeneratesValidStatusWithNoPolicy)
{
    auto queueTask = std::make_shared<Plugin::QueueTask>();
    TestablePluginAdapter pluginAdapter(queueTask);

    static const std::string expectedStatus { R"sophos(<?xml version="1.0" encoding="utf-8" standalone="yes"?>
<status type='LiveQuery'>
    <CompRes policyType='56' Res='NoRef' RevID=''/>
    <scheduled>
        <dailyDataLimitExceeded>false</dailyDataLimitExceeded>
    </scheduled>
</status>)sophos" };

    std::string actualStatus = pluginAdapter.serializeLiveQueryStatus(false);
    EXPECT_EQ(expectedStatus, actualStatus);
}

TEST_F(PluginAdapterWithMockFileSystem, testSerializeLiveQueryStatusGeneratesValidStatusWithPolicy)
{
    auto queueTask = std::make_shared<Plugin::QueueTask>();
    TestablePluginAdapter pluginAdapter(queueTask);
    pluginAdapter.setLiveQueryRevID("testID");
    pluginAdapter.setLiveQueryStatus("Same");

    static const std::string expectedStatus { R"sophos(<?xml version="1.0" encoding="utf-8" standalone="yes"?>
<status type='LiveQuery'>
    <CompRes policyType='56' Res='Same' RevID='testID'/>
    <scheduled>
        <dailyDataLimitExceeded>false</dailyDataLimitExceeded>
    </scheduled>
</status>)sophos" };

    std::string actualStatus = pluginAdapter.serializeLiveQueryStatus(false);
    EXPECT_EQ(expectedStatus, actualStatus);
}


TEST_F(PluginAdapterWithMockFileSystem, testSerializeLiveQueryStatusGeneratesValidStatusWhenDataLimitHit)
{
    auto queueTask = std::make_shared<Plugin::QueueTask>();
    TestablePluginAdapter pluginAdapter(queueTask);
    pluginAdapter.setLiveQueryRevID("testID");
    pluginAdapter.setLiveQueryStatus("Same");

    static const std::string expectedStatus { R"sophos(<?xml version="1.0" encoding="utf-8" standalone="yes"?>
<status type='LiveQuery'>
    <CompRes policyType='56' Res='Same' RevID='testID'/>
    <scheduled>
        <dailyDataLimitExceeded>true</dailyDataLimitExceeded>
    </scheduled>
</status>)sophos" };

    std::string actualStatus = pluginAdapter.serializeLiveQueryStatus(true);
    EXPECT_EQ(expectedStatus, actualStatus);
}

TEST_F(PluginAdapterWithMockFileSystem, testProcessLiveQueryPolicyWithValidPolicy)
{
    auto queueTask = std::make_shared<Plugin::QueueTask>();
    TestablePluginAdapter pluginAdapter(queueTask);
    const std::string PLUGIN_VAR_DIR = Plugin::varDir();
    std::string liveQueryPolicy = "<?xml version=\"1.0\"?>\n"
                                     "<policy type=\"LiveQuery\" RevID=\"987654321\" policyType=\"56\">\n"
                                     "    <configuration>\n"
                                     "        <scheduled>\n"
                                     "            <dailyDataLimit>123456</dailyDataLimit>\n"
                                     "            <queryPacks>\n"
                                     "                <queryPack id=\"queryPackId\" />\n"
                                     "            </queryPacks>\n"
                                     "        </scheduled>\n"
                                     "    </configuration>\n"
                                     "</policy>";

    pluginAdapter.processLiveQueryPolicy(liveQueryPolicy);
    EXPECT_EQ(pluginAdapter.getLiveQueryStatus(), "Same");
    EXPECT_EQ(pluginAdapter.getLiveQueryRevID(), "987654321");
    EXPECT_EQ(pluginAdapter.getLiveQueryDataLimit(), 123456);
}

TEST_F(PluginAdapterWithMockFileSystem, testProcessLiveQueryPolicyWithInvalidPolicy)
{
    auto queueTask = std::make_shared<Plugin::QueueTask>();
    TestablePluginAdapter pluginAdapter(queueTask);

    std::string garbage = "garbage";

    pluginAdapter.processLiveQueryPolicy(garbage);
    EXPECT_EQ(pluginAdapter.getLiveQueryStatus(), "Failure");
    EXPECT_EQ(pluginAdapter.getLiveQueryRevID(), "");
    EXPECT_EQ(pluginAdapter.getLiveQueryDataLimit(), 250000000);
}

TEST_F(PluginAdapterWithMockFileSystem, testHasScheduleEpochEnded)
{
    auto queueTask = std::make_shared<Plugin::QueueTask>();
    TestablePluginAdapter pluginAdapter(queueTask);
    time_t scheduleEpochTimestamp = 1500000000;
    pluginAdapter.setScheduleEpoch(scheduleEpochTimestamp);
    time_t scheduleEpochDuration = pluginAdapter.getScheduleEpochDuration();

    EXPECT_TRUE(pluginAdapter.hasScheduleEpochEnded(scheduleEpochTimestamp+500000000));
    EXPECT_FALSE(pluginAdapter.hasScheduleEpochEnded(scheduleEpochTimestamp));
    EXPECT_FALSE(pluginAdapter.hasScheduleEpochEnded(scheduleEpochTimestamp+1));
    EXPECT_TRUE(pluginAdapter.hasScheduleEpochEnded(scheduleEpochTimestamp-86401));
    EXPECT_FALSE(pluginAdapter.hasScheduleEpochEnded(scheduleEpochTimestamp-86400));
    EXPECT_FALSE(pluginAdapter.hasScheduleEpochEnded(scheduleEpochTimestamp+scheduleEpochDuration));
    EXPECT_TRUE(pluginAdapter.hasScheduleEpochEnded(scheduleEpochTimestamp+scheduleEpochDuration+1));
    EXPECT_FALSE(pluginAdapter.hasScheduleEpochEnded(scheduleEpochTimestamp+scheduleEpochDuration-1));
}


TEST_F(PluginAdapterWithMockFileSystem, testProcessLiveQueryPolicyKeepsOldCustomQueriesOnBadField)
{
    std::string liveQueryPolicy100 = "<?xml version=\"1.0\"?>\n"
                                     "<policy type=\"LiveQuery\" RevID=\"100\" policyType=\"56\">\n"
                                     "    <configuration>\n"
                                     "        <scheduled>\n"
                                     "            <dailyDataLimit>250000000</dailyDataLimit>\n"
                                     "            <queryPacks>\n"
                                     "                <queryPack id=\"queryPackId\" />\n"
                                     "            </queryPacks>\n"
                                     "            <customQueries>\n"
                                     "                  blah\n"
                                     "            </customQueries>\n"
                                     "        </scheduled>\n"
                                     "    </configuration>\n"
                                     "</policy>";

    const std::string PLUGIN_VAR_DIR = Plugin::varDir();
    EXPECT_CALL(*mockFileSystem, writeFile(PLUGIN_VAR_DIR + "/persist-xdrDataUsage", _));
    EXPECT_CALL(*mockFileSystem, writeFile(PLUGIN_VAR_DIR + "/persist-xdrScheduleEpoch", _));
    EXPECT_CALL(*mockFileSystem, writeFile(PLUGIN_VAR_DIR + "/persist-xdrPeriodTimestamp", _));
    EXPECT_CALL(*mockFileSystem, writeFile(PLUGIN_VAR_DIR + "/persist-xdrLimitHit", _));
    EXPECT_CALL(*mockFileSystem, writeFile(PLUGIN_VAR_DIR + "/persist-xdrPeriodInSeconds", _));
    EXPECT_CALL(*mockFileSystem, writeFile(Plugin::edrConfigFilePath(),"running_mode=0\n"));
    // we expect to not remove and write the custom query pack
    EXPECT_CALL(*mockFileSystem, writeFile(Plugin::osqueryCustomConfigFilePath(),_)).Times(0);
    EXPECT_CALL(*mockFileSystem, removeFileOrDirectory(Plugin::osqueryCustomConfigFilePath())).Times(0);
    EXPECT_CALL(*mockFileSystem, removeFileOrDirectory(Plugin::osqueryCustomConfigFilePath()+".DISABLED")).Times(0);
    auto queueTask = std::make_shared<Plugin::QueueTask>();
    TestablePluginAdapter pluginAdapter(queueTask);

    pluginAdapter.processLiveQueryPolicy(liveQueryPolicy100);
}

TEST_F(PluginAdapterWithMockFileSystem, testCustomQueryPackIsRemovedWhenNoQueriesInPolicy)
{
    auto queueTask = std::make_shared<Plugin::QueueTask>();
    TestablePluginAdapter pluginAdapter(queueTask);

    const std::string PLUGIN_VAR_DIR = Plugin::varDir();
    std::string liveQueryPolicy100 = "<?xml version=\"1.0\"?>\n"
                                     "<policy type=\"LiveQuery\" RevID=\"100\" policyType=\"56\">\n"
                                     "    <configuration>\n"
                                     "        <scheduled>\n"
                                     "            <enabled>true</enabled>\n"
                                     "            <dailyDataLimit>250000000</dailyDataLimit>\n"
                                     "        </scheduled>\n"
                                     "    </configuration>\n"
                                     "</policy>";

    EXPECT_CALL(*mockFileSystem, writeFile(PLUGIN_VAR_DIR + "/persist-xdrDataUsage", _));
    EXPECT_CALL(*mockFileSystem, writeFile(PLUGIN_VAR_DIR + "/persist-xdrScheduleEpoch", _));
    EXPECT_CALL(*mockFileSystem, writeFile(PLUGIN_VAR_DIR + "/persist-xdrPeriodTimestamp", _));
    EXPECT_CALL(*mockFileSystem, writeFile(PLUGIN_VAR_DIR + "/persist-xdrLimitHit", _));
    EXPECT_CALL(*mockFileSystem, writeFile(PLUGIN_VAR_DIR + "/persist-xdrPeriodInSeconds", _));
    EXPECT_CALL(*mockFileSystem, writeFile(Plugin::edrConfigFilePath(), "running_mode=1\n"));

    EXPECT_CALL(*mockFileSystem, exists(Plugin::osqueryCustomConfigFilePath())).Times(2).WillRepeatedly(Return(true));
    // We check twice because processLiveQueryPolicy tries to enable it
    // We say to return false twice because we want to test the DISABLED pack will be removed if it exists too
    EXPECT_CALL(*mockFileSystem, exists(Plugin::osqueryCustomConfigFilePath()+".DISABLED")).Times(2).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockFileSystem, readFile(Plugin::osqueryCustomConfigFilePath())).WillOnce(Return("a value"));
    EXPECT_CALL(*mockFileSystem, removeFileOrDirectory(Plugin::osqueryCustomConfigFilePath())).Times(1);
    EXPECT_CALL(*mockFileSystem, removeFileOrDirectory(Plugin::osqueryCustomConfigFilePath()+".DISABLED")).Times(1);
    pluginAdapter.processLiveQueryPolicy(liveQueryPolicy100);
}

TEST_F(PluginAdapterWithMockFileSystem, processFlagsIgoresEmptyInput)
{
    auto queueTask = std::make_shared<Plugin::QueueTask>();
    TestablePluginAdapter pluginAdapter(queueTask);
    const std::string PLUGIN_VAR_DIR = Plugin::varDir();
    EXPECT_CALL(*mockFileSystem, writeFile(PLUGIN_VAR_DIR + "/persist-xdrDataUsage", _));
    EXPECT_CALL(*mockFileSystem, writeFile(PLUGIN_VAR_DIR + "/persist-xdrScheduleEpoch", _));
    EXPECT_CALL(*mockFileSystem, writeFile(PLUGIN_VAR_DIR + "/persist-xdrPeriodTimestamp", _));
    EXPECT_CALL(*mockFileSystem, writeFile(PLUGIN_VAR_DIR + "/persist-xdrLimitHit", _));
    EXPECT_CALL(*mockFileSystem, writeFile(PLUGIN_VAR_DIR + "/persist-xdrPeriodInSeconds", _));
    EXPECT_NO_THROW(pluginAdapter.processFlags(""));
}

TEST_F(PluginAdapterWithMockFileSystem, processFlagsProcessesNetoworkTableFlagOn)
{
    auto queueTask = std::make_shared<Plugin::QueueTask>();
    TestablePluginAdapter pluginAdapter(queueTask);
    const std::string PLUGIN_VAR_DIR = Plugin::varDir();
    EXPECT_CALL(*mockFileSystem, writeFile(PLUGIN_VAR_DIR + "/persist-xdrDataUsage", _));
    EXPECT_CALL(*mockFileSystem, writeFile(PLUGIN_VAR_DIR + "/persist-xdrScheduleEpoch", _));
    EXPECT_CALL(*mockFileSystem, writeFile(PLUGIN_VAR_DIR + "/persist-xdrPeriodTimestamp", _));
    EXPECT_CALL(*mockFileSystem, writeFile(PLUGIN_VAR_DIR + "/persist-xdrLimitHit", _));
    EXPECT_CALL(*mockFileSystem, writeFile(PLUGIN_VAR_DIR + "/persist-xdrPeriodInSeconds", _));
    EXPECT_CALL(*mockFileSystem, writeFile(Plugin::edrConfigFilePath(), Plugin::PluginUtils::NETWORK_TABLES_AVAILABLE + "=1\n"));
    EXPECT_CALL(*mockFileSystem, writeFile(Plugin::edrConfigFilePath(), Plugin::PluginUtils::QUERY_PACK_NEXT_SETTING + "=0\n"));
    std::string networkFlag = "{\"" + Plugin::PluginUtils::NETWORK_TABLES_FLAG + "\":true}";
    EXPECT_NO_THROW(pluginAdapter.processFlags(networkFlag));
}

TEST_F(PluginAdapterWithMockFileSystem, processFlagsProcessesNetoworkTableFlagOff)
{
    auto queueTask = std::make_shared<Plugin::QueueTask>();
    TestablePluginAdapter pluginAdapter(queueTask);
    const std::string PLUGIN_VAR_DIR = Plugin::varDir();
    EXPECT_CALL(*mockFileSystem, writeFile(PLUGIN_VAR_DIR + "/persist-xdrDataUsage", _));
    EXPECT_CALL(*mockFileSystem, writeFile(PLUGIN_VAR_DIR + "/persist-xdrScheduleEpoch", _));
    EXPECT_CALL(*mockFileSystem, writeFile(PLUGIN_VAR_DIR + "/persist-xdrPeriodTimestamp", _));
    EXPECT_CALL(*mockFileSystem, writeFile(PLUGIN_VAR_DIR + "/persist-xdrLimitHit", _));
    EXPECT_CALL(*mockFileSystem, writeFile(PLUGIN_VAR_DIR + "/persist-xdrPeriodInSeconds", _));
    EXPECT_CALL(*mockFileSystem, writeFile(Plugin::edrConfigFilePath(), Plugin::PluginUtils::NETWORK_TABLES_AVAILABLE + "=0\n"));
    EXPECT_CALL(*mockFileSystem, writeFile(Plugin::edrConfigFilePath(), Plugin::PluginUtils::QUERY_PACK_NEXT_SETTING + "=0\n"));
    std::string networkFlag = "{\"" + Plugin::PluginUtils::NETWORK_TABLES_FLAG + "\":false}";
    EXPECT_NO_THROW(pluginAdapter.processFlags(networkFlag));
}

class MockablePluginAdapter : public TestablePluginAdapter
{
public:
    MockablePluginAdapter(std::shared_ptr<Plugin::QueueTask> queueTask) :
        TestablePluginAdapter(queueTask)
    {}
    MOCK_METHOD2(applyLiveQueryPolicy, void(std::optional<Common::XmlUtilities::AttributesMap>, bool));
};

TEST_F(PluginAdapterWithMockFileSystem, testProcessLiveQueryPolicyDoesNotApplyPolicyWhenPolicyIsBroken)
{
    auto queueTask = std::make_shared<Plugin::QueueTask>();
    auto mockPluginAdapter = ::testing::StrictMock<MockablePluginAdapter>(queueTask);

    const std::string PLUGIN_VAR_DIR = Plugin::varDir();
    std::string brokenPolicy = "garbage";

    EXPECT_CALL(mockPluginAdapter, applyLiveQueryPolicy(_, false)).Times(0);
    mockPluginAdapter.processLiveQueryPolicy(brokenPolicy);
    EXPECT_EQ(mockPluginAdapter.getLiveQueryStatus(), "Failure");
}

TEST_F(PluginAdapterWithMockFileSystem, testProcessLiveQueryPolicyAppliesPolicyWhenPolicyIsBroken)
{
    auto queueTask = std::make_shared<Plugin::QueueTask>();
    auto mockPluginAdapter = ::testing::StrictMock<MockablePluginAdapter>(queueTask);

    const std::string PLUGIN_VAR_DIR = Plugin::varDir();
    std::string liveQueryPolicy = "<?xml version=\"1.0\"?>\n"
                                     "<policy type=\"LiveQuery\" RevID=\"100\" policyType=\"56\">\n"
                                     "    <configuration>\n"
                                     "        <scheduled>\n"
                                     "        </scheduled>\n"
                                     "    </configuration>\n"
                                     "</policy>";

    EXPECT_CALL(mockPluginAdapter, applyLiveQueryPolicy(_, false)).Times(1);
    mockPluginAdapter.processLiveQueryPolicy(liveQueryPolicy);
    EXPECT_EQ(mockPluginAdapter.getLiveQueryStatus(), "Same");
}

TEST_F(PluginAdapterWithMockFileSystem, processFlagsProcessesAllFlagsOn)
{
    auto queueTask = std::make_shared<Plugin::QueueTask>();
    TestablePluginAdapter pluginAdapter(queueTask);
    const std::string PLUGIN_VAR_DIR = Plugin::varDir();
    EXPECT_CALL(*mockFileSystem, writeFile(PLUGIN_VAR_DIR + "/persist-xdrDataUsage", _));
    EXPECT_CALL(*mockFileSystem, writeFile(PLUGIN_VAR_DIR + "/persist-xdrScheduleEpoch", _));
    EXPECT_CALL(*mockFileSystem, writeFile(PLUGIN_VAR_DIR + "/persist-xdrPeriodTimestamp", _));
    EXPECT_CALL(*mockFileSystem, writeFile(PLUGIN_VAR_DIR + "/persist-xdrLimitHit", _));
    EXPECT_CALL(*mockFileSystem, writeFile(PLUGIN_VAR_DIR + "/persist-xdrPeriodInSeconds", _));
    EXPECT_CALL(*mockFileSystem, writeFile(Plugin::edrConfigFilePath(), Plugin::PluginUtils::NETWORK_TABLES_AVAILABLE + "=1\n"));
    EXPECT_CALL(*mockFileSystem, writeFile(Plugin::edrConfigFilePath(), Plugin::PluginUtils::QUERY_PACK_NEXT_SETTING + "=0\n"));
    std::string flags = "{\"" +
                              Plugin::PluginUtils::XDR_FLAG + "\":true, \"" +
                              Plugin::PluginUtils::NETWORK_TABLES_FLAG +"\":true}";
    EXPECT_NO_THROW(pluginAdapter.processFlags(flags));
}

TEST_F(PluginAdapterWithMockFileSystem, processFlagsProcessesAllFlagsOff)
{
    auto queueTask = std::make_shared<Plugin::QueueTask>();
    TestablePluginAdapter pluginAdapter(queueTask);
    const std::string PLUGIN_VAR_DIR = Plugin::varDir();
    EXPECT_CALL(*mockFileSystem, writeFile(PLUGIN_VAR_DIR + "/persist-xdrDataUsage", _));
    EXPECT_CALL(*mockFileSystem, writeFile(PLUGIN_VAR_DIR + "/persist-xdrScheduleEpoch", _));
    EXPECT_CALL(*mockFileSystem, writeFile(PLUGIN_VAR_DIR + "/persist-xdrPeriodTimestamp", _));
    EXPECT_CALL(*mockFileSystem, writeFile(PLUGIN_VAR_DIR + "/persist-xdrLimitHit", _));
    EXPECT_CALL(*mockFileSystem, writeFile(PLUGIN_VAR_DIR + "/persist-xdrPeriodInSeconds", _));
    EXPECT_CALL(*mockFileSystem, writeFile(Plugin::edrConfigFilePath(), Plugin::PluginUtils::NETWORK_TABLES_AVAILABLE + "=0\n"));
    EXPECT_CALL(*mockFileSystem, writeFile(Plugin::edrConfigFilePath(), Plugin::PluginUtils::QUERY_PACK_NEXT_SETTING + "=0\n"));
    std::string flags = "{\"" +
                              Plugin::PluginUtils::XDR_FLAG + "\":false, \"" +
                              Plugin::PluginUtils::NETWORK_TABLES_FLAG +"\":false}";
    EXPECT_NO_THROW(pluginAdapter.processFlags(flags));
}
