/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/FileSystemImpl/FileSystemImpl.h>
#include <Common/TaskQueueImpl/TaskQueueImpl.h>
#include <ManagementAgent/ManagementAgentImpl/ManagementAgentMain.h>
#include <ManagementAgent/McsRouterPluginCommunicationImpl/PolicyTask.h>
#include <ManagementAgent/PolicyReceiverImpl/PolicyReceiverImpl.h>
#include <ManagementAgent/StatusReceiverImpl/StatusTask.h>
#include <gmock/gmock-matchers.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <tests/Common/ApplicationConfiguration/MockedApplicationPathManager.h>
#include <tests/Common/Helpers/FileSystemReplaceAndRestore.h>
#include <tests/Common/Helpers/MockFileSystem.h>
#include <tests/Common/Logging/TestConsoleLoggingSetup.h>
#include <tests/Common/TaskQueueImpl/FakeQueue.h>
#include <tests/ManagementAgent/McsRouterPluginCommunicationImpl/MockPluginManager.h>

namespace
{
    class TestManagementAgent : public ManagementAgent::ManagementAgentImpl::ManagementAgentMain
    {
    public:
        TestManagementAgent() = default;

        void initialiseWrapper(ManagementAgent::PluginCommunication::IPluginManager& pluginManager)
        {
            initialise(pluginManager);
        }

        void runWrapper() { run(); }

        Common::TaskQueue::ITaskQueueSharedPtr getTaskQueue() { return m_taskQueue; }
    };

    class ManagementAgentImplTests : public ::testing::Test
    {
    public:
        ManagementAgentImplTests() : m_mockApplicationManager(nullptr) {}

        void SetUp() override
        {
            const std::string frontend = "inproc://frontend";
            const std::string backend = "inproc://backend";

            m_mockApplicationManager = new NiceMock<MockedApplicationPathManager>();
            MockedApplicationPathManager& mockPathManager(*m_mockApplicationManager);
            ON_CALL(mockPathManager, getManagementAgentSocketAddress()).WillByDefault(Return(frontend));
            Common::ApplicationConfiguration::replaceApplicationPathManager(
                std::unique_ptr<Common::ApplicationConfiguration::IApplicationPathManager>(
                    m_mockApplicationManager)); // DONATED
        }

        void TearDown() override { Common::ApplicationConfiguration::restoreApplicationPathManager(); }

        std::string createJsonString()
        {
            std::string jsonString = R"({
                                    "policyAppIds": [
                                    "app1"
                                     ],
                                     "statusAppIds": [
                                      "app2"
                                     ],
                                     "pluginName": "PluginName",
                                     "xmlTranslatorPath": "path1",
                                     "executableFullPath": "path2",
                                     "executableArguments": [
                                      "arg1"
                                     ],
                                     "environmentVariables": [
                                      {
                                       "name": "hello",
                                       "value": "world"
                                      }
                                     ]
                                    })";
            return jsonString;
        }

        StrictMock<MockPluginManager> m_mockPluginManager;
        NiceMock<MockedApplicationPathManager>* m_mockApplicationManager;

    private:
        TestLogging::TestConsoleLoggingSetup m_loggingSetup;
    };

    TEST_F(ManagementAgentImplTests, ManagementAgentMainConstructorWithValidDataDoesNotThrow) // NOLINT
    {
        EXPECT_NO_THROW(ManagementAgent::ManagementAgentImpl::ManagementAgentMain managementAgent); // NOLINT
    }

    TEST_F(ManagementAgentImplTests, initialiseWillSetupManagementAgentAndNotThrow) // NOLINT
    {
        auto filesystemMock = new NiceMock<MockFileSystem>();
        Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));

        std::vector<std::string> pluginFiles = { "PluginName.json" };

        std::string jsonContent = createJsonString();

        std::vector<std::string> registeredPlugins = { "PluginName" };
        std::vector<std::string> policyIds = { "app1" };
        std::vector<std::string> statusIds = { "app2" };
        std::vector<std::string> statusCacheFiles;
        std::string pluginPath("/tmp/plugin"), statusCachePath("/tmp/status_cache");

        EXPECT_CALL(*m_mockApplicationManager, getPluginRegistryPath()).WillRepeatedly(Return(pluginPath));
        EXPECT_CALL(*m_mockApplicationManager, getManagementAgentStatusCacheFilePath())
            .WillRepeatedly(Return(statusCachePath));
        EXPECT_CALL(*filesystemMock, listFiles(pluginPath)).WillOnce(Return(pluginFiles));
        EXPECT_CALL(*filesystemMock, listFiles(statusCachePath)).WillOnce(Return(statusCacheFiles));
        EXPECT_CALL(*filesystemMock, listFiles("/tmp")).WillRepeatedly(Return(std::vector<std::string>{}));
        EXPECT_CALL(*filesystemMock, readFile(_)).WillOnce(Return(jsonContent));
        EXPECT_CALL(*m_mockApplicationManager, getMcsPolicyFilePath()).WillRepeatedly(Return("/tmp"));
        EXPECT_CALL(*m_mockApplicationManager, getMcsActionFilePath()).WillRepeatedly(Return("/tmp"));

        EXPECT_CALL(m_mockPluginManager, registerAndSetAppIds(registeredPlugins[0], policyIds, statusIds)).Times(1);
        EXPECT_CALL(m_mockPluginManager, setPolicyReceiver(_)).Times(1);
        EXPECT_CALL(m_mockPluginManager, setStatusReceiver(_)).Times(1);
        EXPECT_CALL(m_mockPluginManager, setEventReceiver(_)).Times(1);

        std::vector<Common::PluginApi::StatusInfo> pluginStatusInfoList;

        EXPECT_CALL(m_mockPluginManager, getRegisteredPluginNames()).WillOnce(Return(registeredPlugins));
        EXPECT_CALL(m_mockPluginManager, getStatus(_)).WillOnce(Return(pluginStatusInfoList));

        TestManagementAgent managementAgent;

        EXPECT_NO_THROW(managementAgent.initialiseWrapper(m_mockPluginManager)); // NOLINT
        EXPECT_EQ(1, 1);
    }

    TEST_F(ManagementAgentImplTests, initialiseManagementAgentWillAddStatusTaskToQueue) // NOLINT
    {
        auto filesystemMock = new NiceMock<MockFileSystem>();
        Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));

        std::vector<std::string> pluginFiles = { "PluginName.json" };

        std::string jsonContent = createJsonString();

        std::vector<std::string> registeredPlugins = { "PluginName" };
        std::vector<std::string> policyIds = { "app1" };
        std::vector<std::string> statusIds = { "app2" };
        std::vector<std::string> statusCacheFiles;
        std::string pluginPath("/tmp/plugin"), statusCachePath("/tmp/status_cache");

        EXPECT_CALL(*m_mockApplicationManager, getPluginRegistryPath()).WillRepeatedly(Return(pluginPath));
        EXPECT_CALL(*m_mockApplicationManager, getManagementAgentStatusCacheFilePath())
            .WillRepeatedly(Return(statusCachePath));

        EXPECT_CALL(*filesystemMock, listFiles(pluginPath)).WillOnce(Return(pluginFiles));
        EXPECT_CALL(*filesystemMock, listFiles(statusCachePath)).WillOnce(Return(statusCacheFiles));
        EXPECT_CALL(*filesystemMock, listFiles("/tmp")).WillRepeatedly(Return(std::vector<std::string>{}));

        EXPECT_CALL(*filesystemMock, readFile(_)).WillOnce(Return(jsonContent));
        EXPECT_CALL(*m_mockApplicationManager, getMcsPolicyFilePath()).WillRepeatedly(Return("/tmp"));
        EXPECT_CALL(*m_mockApplicationManager, getMcsActionFilePath()).WillRepeatedly(Return("/tmp"));

        EXPECT_CALL(m_mockPluginManager, registerAndSetAppIds(registeredPlugins[0], policyIds, statusIds)).Times(1);
        EXPECT_CALL(m_mockPluginManager, setPolicyReceiver(_)).Times(1);
        EXPECT_CALL(m_mockPluginManager, setStatusReceiver(_)).Times(1);
        EXPECT_CALL(m_mockPluginManager, setEventReceiver(_)).Times(1);

        std::vector<Common::PluginApi::StatusInfo> pluginStatusInfoList;
        Common::PluginApi::StatusInfo statusInfo;
        statusInfo.appId = "app2";
        statusInfo.statusXml = "StatusXml";
        statusInfo.statusWithoutTimestampsXml = "StatusWithoutTimestampsXml";
        pluginStatusInfoList.push_back(statusInfo);

        EXPECT_CALL(m_mockPluginManager, getRegisteredPluginNames()).WillOnce(Return(registeredPlugins));
        EXPECT_CALL(m_mockPluginManager, getStatus(_)).WillOnce(Return(pluginStatusInfoList));

        TestManagementAgent managementAgent;

        EXPECT_NO_THROW(managementAgent.initialiseWrapper(m_mockPluginManager)); // NOLINT

        Common::TaskQueue::ITaskQueueSharedPtr actualTaskQueue = managementAgent.getTaskQueue();
        Common::TaskQueue::ITaskPtr task = actualTaskQueue->popTask();

        EXPECT_NE(task, nullptr);
    }

} // namespace
