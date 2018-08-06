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

        EXPECT_CALL(*filesystemMock, listFiles(_)).WillOnce(Return(pluginFiles));
        EXPECT_CALL(*filesystemMock, readFile(_)).WillOnce(Return(jsonContent));
        EXPECT_CALL(*m_mockApplicationManager, getMcsPolicyFilePath()).WillRepeatedly(Return("/tmp"));
        EXPECT_CALL(*m_mockApplicationManager, getMcsActionFilePath()).WillRepeatedly(Return("/tmp"));

        EXPECT_CALL(m_mockPluginManager, registerPlugin("PluginName")).Times(1);
        EXPECT_CALL(m_mockPluginManager, setPolicyReceiver(_)).Times(1);
        EXPECT_CALL(m_mockPluginManager, setStatusReceiver(_)).Times(1);
        EXPECT_CALL(m_mockPluginManager, setEventReceiver(_)).Times(1);

        TestManagementAgent managementAgent;

        EXPECT_NO_THROW(managementAgent.initialiseWrapper(m_mockPluginManager));

    }
}


