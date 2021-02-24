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
#include <modules/pluginimpl/ApplicationPaths.h>
#include <modules/pluginimpl/PluginAdapter.h>
#include <modules/pluginimpl/LiveQueryPolicyParser.h>
#include <tests/googletest/googlemock/include/gmock/gmock-matchers.h>

#include <gtest/gtest.h>
#include <Common/UtilityImpl/TimeUtils.h>
#include <Common/XmlUtilities/AttributesMap.h>

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
        Plugin::PluginAdapter::processLiveQueryPolicy(policy);
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
    task.m_taskType = Plugin::Task::TaskType::Query;
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

TEST_F(PluginAdapterWithMockFileSystem, testGetDataLimit)
{ // NOLINT
    auto queueTask = std::make_shared<Plugin::QueueTask>();
    TestablePluginAdapter pluginAdapter(queueTask);

    std::string liveQueryPolicy100000 = "<?xml version=\"1.0\"?>\n"
                                        "<policy type=\"LiveQuery\" RevID=\"revId\" policyType=\"56\">\n"
                                        "    <configuration>\n"
                                        "        <scheduled>\n"
                                        "            <dailyDataLimit>100000</dailyDataLimit>\n"
                                        "            <queryPacks>\n"
                                        "                <queryPack id=\"queryPackId\" />\n"
                                        "            </queryPacks>\n"
                                        "        </scheduled>\n"
                                        "    </configuration>\n"
                                        "</policy>";
    EXPECT_EQ(pluginAdapter.getDataLimit(liveQueryPolicy100000), 100000);

    std::string liveQueryPolicy234567 = "<?xml version=\"1.0\"?>\n"
                                        "<policy type=\"LiveQuery\" RevID=\"revId\" policyType=\"56\">\n"
                                        "    <configuration>\n"
                                        "        <scheduled>\n"
                                        "            <dailyDataLimit>234567</dailyDataLimit>\n"
                                        "            <queryPacks>\n"
                                        "                <queryPack id=\"queryPackId\" />\n"
                                        "            </queryPacks>\n"
                                        "        </scheduled>\n"
                                        "    </configuration>\n"
                                        "</policy>";
    EXPECT_EQ(pluginAdapter.getDataLimit(liveQueryPolicy234567), 234567);

    std::string nonsense = "asdfbhasdlfhasdflasdhfasd";
    EXPECT_THROW(pluginAdapter.getDataLimit(nonsense), std::exception);

    std::string validXmlWithMissingField = "<?xml version=\"1.0\"?>\n"
                                           "<policy type=\"LiveQuery\" RevID=\"revId\" policyType=\"56\">\n"
                                           "    <configuration>\n"
                                           "        <scheduled>\n"
                                           "            <notDailyDataLimit>234567</notDailyDataLimit>\n"
                                           "            <queryPacks>\n"
                                           "                <queryPack id=\"queryPackId\" />\n"
                                           "            </queryPacks>\n"
                                           "        </scheduled>\n"
                                           "    </configuration>\n"
                                           "</policy>";
    EXPECT_THROW(pluginAdapter.getDataLimit(validXmlWithMissingField), std::exception);

    std::string validXmlWithInvalidFieldData = "<?xml version=\"1.0\"?>\n"
                                               "<policy type=\"LiveQuery\" RevID=\"revId\" policyType=\"56\">\n"
                                               "    <configuration>\n"
                                               "        <scheduled>\n"
                                               "            <notDailyDataLimit>notAnInteger</notDailyDataLimit>\n"
                                               "            <queryPacks>\n"
                                               "                <queryPack id=\"queryPackId\" />\n"
                                               "            </queryPacks>\n"
                                               "        </scheduled>\n"
                                               "    </configuration>\n"
                                               "</policy>";
    EXPECT_THROW(pluginAdapter.getDataLimit(validXmlWithInvalidFieldData), std::exception);
}

TEST_F(PluginAdapterWithMockFileSystem, testGetRevID)
{
    auto queueTask = std::make_shared<Plugin::QueueTask>();
    TestablePluginAdapter pluginAdapter(queueTask);

    std::string liveQueryPolicy100 = "<?xml version=\"1.0\"?>\n"
                                     "<policy type=\"LiveQuery\" RevID=\"100\" policyType=\"56\">\n"
                                     "    <configuration>\n"
                                     "        <scheduled>\n"
                                     "            <dailyDataLimit>250000000</dailyDataLimit>\n"
                                     "            <queryPacks>\n"
                                     "                <queryPack id=\"queryPackId\" />\n"
                                     "            </queryPacks>\n"
                                     "        </scheduled>\n"
                                     "    </configuration>\n"
                                     "</policy>";

    EXPECT_EQ(pluginAdapter.getRevId(liveQueryPolicy100), "100");

    std::string liveQueryPolicy999999999 = "<?xml version=\"1.0\"?>\n"
                                     "<policy type=\"LiveQuery\" RevID=\"999999999\" policyType=\"56\">\n"
                                     "    <configuration>\n"
                                     "        <scheduled>\n"
                                     "            <dailyDataLimit>250000000</dailyDataLimit>\n"
                                     "            <queryPacks>\n"
                                     "                <queryPack id=\"queryPackId\" />\n"
                                     "            </queryPacks>\n"
                                     "        </scheduled>\n"
                                     "    </configuration>\n"
                                     "</policy>";

    EXPECT_EQ(pluginAdapter.getRevId(liveQueryPolicy999999999), "999999999");

    std::string noRevIdPolicy = "<?xml version=\"1.0\"?>\n"
                                     "<policy type=\"LiveQuery\" NotRevID=\"999999999\" policyType=\"56\">\n"
                                     "    <configuration>\n"
                                     "        <scheduled>\n"
                                     "            <dailyDataLimit>250000000</dailyDataLimit>\n"
                                     "            <queryPacks>\n"
                                     "                <queryPack id=\"queryPackId\" />\n"
                                     "            </queryPacks>\n"
                                     "        </scheduled>\n"
                                     "    </configuration>\n"
                                     "</policy>";

    EXPECT_THROW(pluginAdapter.getRevId(noRevIdPolicy), std::exception);

    std::string garbage = "garbage";

    EXPECT_THROW(pluginAdapter.getRevId(garbage), std::exception);
}

TEST_F(PluginAdapterWithMockFileSystem, testUpdateCustomQueries)
{
    auto queueTask = std::make_shared<Plugin::QueueTask>();
    TestablePluginAdapter pluginAdapter(queueTask);

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

    std::string expected = "{"
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
    EXPECT_EQ(Plugin::getCustomQueries(liveQueryPolicy100).value(), expected);
}

TEST_F(PluginAdapterWithMockFileSystem, testUpdateCustomQueriesdoesNotIncludeMalformedQueriesInJson)
{
    auto queueTask = std::make_shared<Plugin::QueueTask>();
    TestablePluginAdapter pluginAdapter(queueTask);

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
                                     "                      <interval></interval>\n"
                                     "                      <tag>stream</tag>\n"
                                     "                      <removed>true</removed>\n"
                                     "                      <denylist>true</denylist>\n"
                                     "                  </customQuery>\n"
                                     "                  <customQuery>\n"
                                     "                      <description>a different basic query</description>\n"
                                     "                      <query>SELECT * FROM otherstuff</query>\n"
                                     "                      <interval></interval>\n"
                                     "                      <tag>stream</tag>\n"
                                     "                      <removed>true</removed>\n"
                                     "                      <denylist>true</denylist>\n"
                                     "                  </customQuery>\n"
                                     "                  <customQuery queryName=\"blah2\">\n"
                                     "                      <description>a different basic query</description>\n"
                                     "                      <query></query>\n"
                                     "                      <interval></interval>\n"
                                     "                      <tag>stream</tag>\n"
                                     "                      <removed>true</removed>\n"
                                     "                      <denylist>true</denylist>\n"
                                     "                  </customQuery>\n"
                                     "                  <customQuery queryName=\"blah2\">\n"
                                     "                      <description>a different basic query</description>\n"
                                     "                      <query>  </query>\n"
                                     "                      <interval></interval>\n"
                                     "                      <tag>stream</tag>\n"
                                     "                      <removed>true</removed>\n"
                                     "                      <denylist>true</denylist>\n"
                                     "                  </customQuery>\n"
                                     "            </customQueries>\n"
                                     "        </scheduled>\n"
                                     "    </configuration>\n"
                                     "</policy>";

    std::string expected = "{"
                           "\"schedule\":{"
                           "\"blah\":{"
                           "\"denylist\":\"false\","
                           "\"description\":\"basic query\","
                           "\"interval\":\"10\","
                           "\"query\":\"SELECT * FROM stuff\","
                           "\"removed\":\"false\","
                           "\"tag\":\"DataLake\""
                           "}"
                           "}"
                           "}";
    EXPECT_EQ(Plugin::getCustomQueries(liveQueryPolicy100).value(), expected);
}

TEST_F(PluginAdapterWithMockFileSystem, testProcessLiveQueryCustomQueries)
{
    auto queueTask = std::make_shared<Plugin::QueueTask>();
    TestablePluginAdapter pluginAdapter(queueTask);

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
    EXPECT_CALL(*mockFileSystem, writeFile(Plugin::osqueryCustomConfigFilePath(),expected));
    pluginAdapter.processLiveQueryPolicy(liveQueryPolicy100);

}

TEST_F(PluginAdapterWithMockFileSystem, testProcessLiveQueryFoldingRulesHandlesExpectedPolicy)
{
     std::string liveQueryPolicy = "<?xml version=\"1.0\"?>\n"
                                    "<policy type=\"LiveQuery\" RevID=\"abc123\" policyType=\"56\">\n"
                                    "   <configuration>\n"
                                    "       <scheduled>\n"
                                    "           <dailyDataLimit>100</dailyDataLimit>\n"
                                    "           <queryPacks>\n"
                                    "               <queryPack id=\"XDR\"/>\n"
                                    "               <queryPack id=\"MTR\"/>\n"
                                    "           </queryPacks>\n"
                                    "           <foldingRules>\n"
                                    "               [\n"
                                    "                   {\n"
                                    "                       \"query_name\":\"test_folding_query\",\n"
                                    "                       \"values\":{\n"
                                    "                           \"column_name\": \"column_value\",\n"
                                    "                           \"column_name2\": \"column_value2\"\n"
                                    "                       }\n"
                                    "                   },\n"
                                    "                   {\n"
                                    "                       \"query_name\":\"test_folding_query2\",\n"
                                    "                        \"values\":{\n"
                                    "                               \"column_name3\": \"column_value3\"\n"
                                    "                       }\n"
                                    "                   }\n"
                                    "               ]\n"
                                    "           </foldingRules>\n"
                                    "       </scheduled>\n"
                                    "   </configuration>\n"
                                    "</policy>";

    auto rules = Plugin::getFoldingRules(liveQueryPolicy);
    EXPECT_EQ(rules.size(), 2);
    size_t count = 0;
    for (const auto& r : rules)
    {
        SCOPED_TRACE(count);

        ASSERT_TRUE(r.get("query_name", "").isString());
        ASSERT_TRUE(r.get("values", "").isObject());

        const std::string query_name = r.get("query_name", "").asString();
        const Json::Value values = r.get("values", "");

        if (count == 0)
        {
            EXPECT_STREQ(query_name.c_str(), "test_folding_query");
            EXPECT_STREQ(values.get("column_name", "").asString().c_str(), "column_value");
            EXPECT_STREQ(values.get("column_name2", "").asString().c_str(), "column_value2");
        }
        else if (count == 1)
        {
            EXPECT_STREQ(query_name.c_str(), "test_folding_query2");
            EXPECT_STREQ(values.get("column_name3", "").asString().c_str(), "column_value3");
        }

        count++;
    }
    EXPECT_EQ(count, rules.size());

}

TEST_F(PluginAdapterWithMockFileSystem, testProcessLiveQueryFoldingRulesWithNoFoldingRules)
{
    std::string liveQueryPolicyNone = "<?xml version=\"1.0\"?>\n"
                                      "<policy type=\"LiveQuery\" RevID=\"abc123\" policyType=\"56\">\n"
                                      "   <configuration>\n"
                                      "       <scheduled>\n"
                                      "           <dailyDataLimit>100</dailyDataLimit>\n"
                                      "           <queryPacks>\n"
                                      "               <queryPack id=\"XDR\"/>\n"
                                      "               <queryPack id=\"MTR\"/>\n"
                                      "           </queryPacks>\n"
                                      "       </scheduled>\n"
                                      "   </configuration>\n"
                                      "</policy>";

    EXPECT_TRUE(Plugin::getFoldingRules(liveQueryPolicyNone).empty());
}

TEST_F(PluginAdapterWithMockFileSystem, testProcessLiveQueryFoldingRulesInvalidJson)
{
    std::string liveQueryPolicyInvalid = "<?xml version=\"1.0\"?>\n"
                                         "<policy type=\"LiveQuery\" RevID=\"abc123\" policyType=\"56\">\n"
                                         "   <configuration>\n"
                                         "       <scheduled>\n"
                                         "           <dailyDataLimit>100</dailyDataLimit>\n"
                                         "           <queryPacks>\n"
                                         "               <queryPack id=\"XDR\"/>\n"
                                         "               <queryPack id=\"MTR\"/>\n"
                                         "           </queryPacks>\n"
                                         "           <foldingRules>\n"
                                         "               blah\n"
                                         "           </foldingRules>\n"
                                         "       </scheduled>\n"
                                         "   </configuration>\n"
                                         "</policy>";

    EXPECT_TRUE(Plugin::getFoldingRules(liveQueryPolicyInvalid).empty());
}

TEST_F(PluginAdapterWithMockFileSystem, testProcessLiveQueryFoldingRulesParsesAllButInvalidRules)
{
    std::string liveQueryPolicyInvalid = "<?xml version=\"1.0\"?>\n"
                                         "<policy type=\"LiveQuery\" RevID=\"abc123\" policyType=\"56\">\n"
                                         "   <configuration>\n"
                                         "       <scheduled>\n"
                                         "           <dailyDataLimit>100</dailyDataLimit>\n"
                                         "           <queryPacks>\n"
                                         "               <queryPack id=\"XDR\"/>\n"
                                         "               <queryPack id=\"MTR\"/>\n"
                                         "           </queryPacks>\n"
                                         "           <foldingRules>\n"
                                         "               [\n"
                                         "                   {\n"
                                         "                       \"query_name\":\"test_folding_query\",\n"
                                         "                       \"not-values\":{\n"
                                         "                           \"column_name\": \"column_value\",\n"
                                         "                           \"column_name2\": \"column_value2\"\n"
                                         "                       }\n"
                                         "                   },\n"
                                         "                   {\n"
                                         "                       \"query_name\":\"test_folding_query2\",\n"
                                         "                        \"values\":{\n"
                                         "                               \"column_name3\": \"column_value3\"\n"
                                         "                       }\n"
                                         "                   }\n"
                                         "               ]\n"
                                         "           </foldingRules>\n"
                                         "       </scheduled>\n"
                                         "   </configuration>\n"
                                         "</policy>";

    std::vector<Json::Value> expected;
    Json::Value root;
    Json::Value values;
    values["column_name3"] = "column_value3";
    root["query_name"] = "test_folding_query2";
    root["values"] = values;
    expected.push_back(root);
    auto actual = Plugin::getFoldingRules(liveQueryPolicyInvalid);

    EXPECT_EQ(actual.size(), 1);
    EXPECT_EQ(actual, expected);
}

TEST_F(PluginAdapterWithMockFileSystem, testFoldingRulesTelemetry)
{
    auto queueTask = std::make_shared<Plugin::QueueTask>();
    TestablePluginAdapter pluginAdapter(queueTask);

    auto& telemetry = Common::Telemetry::TelemetryHelper::getInstance();
    telemetry.serialiseAndReset();

    std::string liveQueryPolicy = "<?xml version=\"1.0\"?>\n"
                                  "<policy type=\"LiveQuery\" RevID=\"abc123\" policyType=\"56\">\n"
                                  "   <configuration>\n"
                                  "       <scheduled>\n"
                                  "           <dailyDataLimit>100</dailyDataLimit>\n"
                                  "           <queryPacks>\n"
                                  "               <queryPack id=\"XDR\"/>\n"
                                  "               <queryPack id=\"MTR\"/>\n"
                                  "           </queryPacks>\n"
                                  "           <foldingRules>\n"
                                  "               [\n"
                                  "                   {\n"
                                  "                       \"query_name\":\"test_folding_query\",\n"
                                  "                       \"values\":{\n"
                                  "                           \"column_name\": \"column_value\",\n"
                                  "                           \"column_name2\": \"column_value2\"\n"
                                  "                       }\n"
                                  "                   },\n"
                                  "                   {\n"
                                  "                       \"query_name\":\"test_folding_query2\",\n"
                                  "                        \"values\":{\n"
                                  "                               \"column_name3\": \"column_value3\"\n"
                                  "                       }\n"
                                  "                   }\n"
                                  "               ]\n"
                                  "           </foldingRules>\n"
                                  "       </scheduled>\n"
                                  "   </configuration>\n"
                                  "</policy>";

    pluginAdapter.processLiveQueryPolicy(liveQueryPolicy);

    EXPECT_EQ(telemetry.serialiseAndReset(), "{\"foldable-queries\":[\"test_folding_query\",\"test_folding_query2\"]}");
}

TEST_F(PluginAdapterWithMockFileSystem, testNoFoldingRulesTelemetry)
{
    auto queueTask = std::make_shared<Plugin::QueueTask>();
    TestablePluginAdapter pluginAdapter(queueTask);

    auto& telemetry = Common::Telemetry::TelemetryHelper::getInstance();
    telemetry.serialiseAndReset();

    std::string liveQueryPolicy = "<?xml version=\"1.0\"?>\n"
                                  "<policy type=\"LiveQuery\" RevID=\"abc123\" policyType=\"56\">\n"
                                  "   <configuration>\n"
                                  "       <scheduled>\n"
                                  "           <dailyDataLimit>100</dailyDataLimit>\n"
                                  "           <queryPacks>\n"
                                  "               <queryPack id=\"XDR\"/>\n"
                                  "               <queryPack id=\"MTR\"/>\n"
                                  "           </queryPacks>\n"
                                  "       </scheduled>\n"
                                  "   </configuration>\n"
                                  "</policy>";

    pluginAdapter.processLiveQueryPolicy(liveQueryPolicy);

    EXPECT_EQ(telemetry.serialiseAndReset(), "{}");
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
    EXPECT_CALL(*mockFileSystem, exists(Plugin::osqueryCustomConfigFilePath())).WillOnce(Return(false));
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
    EXPECT_FALSE(pluginAdapter.hasScheduleEpochEnded(scheduleEpochTimestamp-500000000));
    EXPECT_FALSE(pluginAdapter.hasScheduleEpochEnded(scheduleEpochTimestamp+scheduleEpochDuration));
    EXPECT_TRUE(pluginAdapter.hasScheduleEpochEnded(scheduleEpochTimestamp+scheduleEpochDuration+1));
    EXPECT_FALSE(pluginAdapter.hasScheduleEpochEnded(scheduleEpochTimestamp+scheduleEpochDuration-1));
}

TEST_F(PluginAdapterWithMockFileSystem, testHaveCustomQueriesChanged)
{
    std::optional<std::string> value1 = "1";
    std::optional<std::string> value2 = "2";
    std::optional<std::string> emptyOptionalString;

    EXPECT_CALL(*mockFileSystem, exists(Plugin::osqueryCustomConfigFilePath())).Times(2).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockFileSystem, readFile(Plugin::osqueryCustomConfigFilePath())).Times(2).WillRepeatedly(Return(value2.value()));
    // file exists, different value
    EXPECT_TRUE(Plugin::PluginAdapter::haveCustomQueriesChanged(value1));
    // file exists, same value
    EXPECT_FALSE(Plugin::PluginAdapter::haveCustomQueriesChanged(value2));

    EXPECT_CALL(*mockFileSystem, exists(Plugin::osqueryCustomConfigFilePath())).Times(2).WillRepeatedly(Return(false));
    // file doesn't exist, new custom pack has value
    EXPECT_TRUE(Plugin::PluginAdapter::haveCustomQueriesChanged(value1));
    // file doesn't exist, new custom pack has no value
    EXPECT_FALSE(Plugin::PluginAdapter::haveCustomQueriesChanged(emptyOptionalString));
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
                                     "            <dailyDataLimit>250000000</dailyDataLimit>\n"
                                     "            <queryPacks>\n"
                                     "                <queryPack id=\"queryPackId\" />\n"
                                     "            </queryPacks>\n"
                                     "        </scheduled>\n"
                                     "    </configuration>\n"
                                     "</policy>";

    EXPECT_CALL(*mockFileSystem, writeFile(PLUGIN_VAR_DIR + "/persist-xdrDataUsage", _));
    EXPECT_CALL(*mockFileSystem, writeFile(PLUGIN_VAR_DIR + "/persist-xdrScheduleEpoch", _));
    EXPECT_CALL(*mockFileSystem, writeFile(PLUGIN_VAR_DIR + "/persist-xdrPeriodTimestamp", _));
    EXPECT_CALL(*mockFileSystem, writeFile(PLUGIN_VAR_DIR + "/persist-xdrLimitHit", _));
    EXPECT_CALL(*mockFileSystem, writeFile(PLUGIN_VAR_DIR + "/persist-xdrPeriodInSeconds", _));

    EXPECT_CALL(*mockFileSystem, exists(Plugin::osqueryCustomConfigFilePath())).Times(2).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockFileSystem, readFile(Plugin::osqueryCustomConfigFilePath())).WillOnce(Return("a value"));
    EXPECT_CALL(*mockFileSystem, removeFileOrDirectory(Plugin::osqueryCustomConfigFilePath())).Times(1);
    pluginAdapter.processLiveQueryPolicy(liveQueryPolicy100);

}