/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <ManagementAgent/ManagementAgentImpl/ManagementAgentMain.h>
#include <ManagementAgent/McsRouterPluginCommunicationImpl/PolicyTask.h>
#include <ManagementAgent/PolicyReceiverImpl/PolicyReceiverImpl.h>

#include <Common/FileSystemImpl/FileSystemImpl.h>
#include <Common/TaskQueueImpl/TaskQueueImpl.h>

#include <tests/ManagementAgent/McsRouterPluginCommunicationImpl/MockPluginManager.h>
#include <tests/Common/TaskQueueImpl/FakeQueue.h>
#include <tests/Common/FileSystemImpl/MockFileSystem.h>
#include <tests/Common/ApplicationConfiguration/MockedApplicationPathManager.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <gmock/gmock-matchers.h>
#include <ManagementAgent/StatusReceiverImpl/StatusTask.h>

namespace
{
    class TestManagementAgent
            : public ManagementAgent::ManagementAgentImpl::ManagementAgentMain
    {
    public:
        void initialiseWrapper(ManagementAgent::PluginCommunication::IPluginManager& pluginManager)
        {
            initialise(pluginManager);
        }

        void runWrapper()
        {
            run();
        }

        Common::TaskQueue::ITaskQueueSharedPtr getTaskQueue()
        {
            return m_taskQueue;
        }
    };

    class ManagementAgentImplTests : public ::testing::Test
    {
    public:
        ManagementAgentImplTests()
            : m_mockApplicationManager(nullptr)
        {
        }

        void SetUp() override
        {
            const std::string frontend="inproc://frontend";
            const std::string backend="inproc://backend";

            m_mockApplicationManager = new NiceMock<MockedApplicationPathManager>();
            MockedApplicationPathManager &mockPathManager(*m_mockApplicationManager);
            ON_CALL(mockPathManager, getManagementAgentSocketAddress()).WillByDefault(Return(frontend));
            Common::ApplicationConfiguration::replaceApplicationPathManager(
                    std::unique_ptr<Common::ApplicationConfiguration::IApplicationPathManager>(m_mockApplicationManager)); // DONATED

        }

        void TearDown() override
        {

        }

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
    };

    TEST_F(ManagementAgentImplTests, ManagementAgentMainConstructorWithValidDataDoesNotThrow) // NOLINT
    {
        EXPECT_NO_THROW(ManagementAgent::ManagementAgentImpl::ManagementAgentMain managementAgent());
    }

    TEST_F(ManagementAgentImplTests, initialiseWillSetupManagementAgentAndNotThrow) // NOLINT
    {
        auto filesystemMock = new NiceMock<MockFileSystem>();
        Common::FileSystem::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));

        std::vector<std::string> pluginFiles = {"PluginName.json"};

        std::string jsonContent = createJsonString();

        std::vector<std::string> registeredPlugins = {"PluginName"};
        std::vector<std::string> policyIds = {"app1"};
        std::vector<std::string> statusIds = {"app2"};

        EXPECT_CALL(*filesystemMock, listFiles(_)).WillOnce(Return(pluginFiles));
        EXPECT_CALL(*filesystemMock, readFile(_)).WillOnce(Return(jsonContent));
        EXPECT_CALL(*m_mockApplicationManager, getMcsPolicyFilePath()).WillRepeatedly(Return("/tmp"));
        EXPECT_CALL(*m_mockApplicationManager, getMcsActionFilePath()).WillRepeatedly(Return("/tmp"));

        EXPECT_CALL(m_mockPluginManager, registerPlugin(registeredPlugins[0])).Times(1);
        EXPECT_CALL(m_mockPluginManager, setAppIds("PluginName", policyIds, statusIds)).Times(1);
        EXPECT_CALL(m_mockPluginManager, setPolicyReceiver(_)).Times(1);
        EXPECT_CALL(m_mockPluginManager, setStatusReceiver(_)).Times(1);
        EXPECT_CALL(m_mockPluginManager, setEventReceiver(_)).Times(1);

        std::vector<Common::PluginApi::StatusInfo> pluginStatusInfoList;

        EXPECT_CALL(m_mockPluginManager, getRegisteredPluginNames()).WillOnce(Return(registeredPlugins));
        EXPECT_CALL(m_mockPluginManager, getStatus(_)).WillOnce(Return(pluginStatusInfoList));

        TestManagementAgent managementAgent;

        EXPECT_NO_THROW(managementAgent.initialiseWrapper(m_mockPluginManager));

     }

    TEST_F(ManagementAgentImplTests, initialiseManagementAgentWillAddStatusTaskToQueue) // NOLINT
    {
        auto filesystemMock = new NiceMock<MockFileSystem>();
        Common::FileSystem::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));

        std::vector<std::string> pluginFiles = {"PluginName.json"};

        std::string jsonContent = createJsonString();

        std::vector<std::string> registeredPlugins = {"PluginName"};
        std::vector<std::string> policyIds = {"app1"};
        std::vector<std::string> statusIds = {"app2"};

        EXPECT_CALL(*filesystemMock, listFiles(_)).WillOnce(Return(pluginFiles));
        EXPECT_CALL(*filesystemMock, readFile(_)).WillOnce(Return(jsonContent));
        EXPECT_CALL(*m_mockApplicationManager, getMcsPolicyFilePath()).WillRepeatedly(Return("/tmp"));
        EXPECT_CALL(*m_mockApplicationManager, getMcsActionFilePath()).WillRepeatedly(Return("/tmp"));

        EXPECT_CALL(m_mockPluginManager, registerPlugin(registeredPlugins[0])).Times(1);
        EXPECT_CALL(m_mockPluginManager, setAppIds("PluginName", policyIds, statusIds)).Times(1);
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

        EXPECT_NO_THROW(managementAgent.initialiseWrapper(m_mockPluginManager));

        Common::TaskQueue::ITaskQueueSharedPtr actualTaskQueue = managementAgent.getTaskQueue();
        Common::TaskQueue::ITaskPtr task = actualTaskQueue->popTask();

        EXPECT_NE(task, nullptr);
    }

}


