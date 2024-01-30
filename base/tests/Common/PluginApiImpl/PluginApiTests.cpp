// Copyright 2018-2023 Sophos Limited. All rights reserved.
#include "MockedPluginApiCallback.h"
#include "SingleResponseServer.h"

#include "Common/Logging/ConsoleLoggingSetup.h"
#include "Common/PluginApi/ApiException.h"
#include "Common/PluginApi/IBaseServiceApi.h"
#include "Common/PluginApiImpl/PluginResourceManagement.h"
#include "Common/ZeroMQWrapper/ISocketRequester.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "tests/Common/ApplicationConfiguration/MockedApplicationPathManager.h"
#include "tests/Common/Helpers/FilePermissionsReplaceAndRestore.h"
#include "tests/Common/Helpers/FileSystemReplaceAndRestore.h"
#include "tests/Common/Helpers/MockFilePermissions.h"
#include "tests/Common/Helpers/MockFileSystem.h"

#include <thread>

namespace
{
    using data_t = Common::ZeroMQWrapper::IReadable::data_t;
    using namespace Common::PluginApiImpl;
    using namespace Common::PluginApi;

    using ::testing::NiceMock;
    using ::testing::StrictMock;

    class PluginApiTests : public ::testing::Test
    {
    public:
        PluginApiTests()
        {  
        }
        void SetUp() override
        {
            MockedApplicationPathManager* mockAppManager = new NiceMock<MockedApplicationPathManager>();
            MockedApplicationPathManager& mock(*mockAppManager);
            ON_CALL(mock, getManagementAgentSocketAddress()).WillByDefault(Return("inproc://management.ipc"));
            ON_CALL(mock, getPluginSocketAddress(_)).WillByDefault(Return("inproc://plugin.ipc"));
            Common::ApplicationConfiguration::replaceApplicationPathManager(
                std::unique_ptr<Common::ApplicationConfiguration::IApplicationPathManager>(mockAppManager));

            auto mockFileSystem = new StrictMock<MockFileSystem>();
            m_replacer.replace(std::unique_ptr<Common::FileSystem::IFileSystem>(mockFileSystem));

            auto mockFilePermissions = new StrictMock<MockFilePermissions>();
            std::unique_ptr<MockFilePermissions> mockIFilePermissionsPtr =
                std::unique_ptr<MockFilePermissions>(mockFilePermissions);
            Tests::replaceFilePermissions(std::move(mockIFilePermissionsPtr));

            EXPECT_CALL(*mockFilePermissions, getUserId(_)).WillRepeatedly(Return(1));
            EXPECT_CALL(*mockFilePermissions, chmod(_, _)).WillRepeatedly(Return());
            EXPECT_CALL(*mockFilePermissions, chown(A<const Path&>(), A<const std::string&>(), A<const std::string&>())).WillRepeatedly(Return());

            mockPluginCallback = std::make_shared<NiceMock<MockedPluginApiCallback>>();

            auto context = pluginResourceManagement.getSocketContext();
            server = std::thread(std::ref(responseServer), context);
            // Set default timeouts to be short for timeout test below
            pluginResourceManagement.setDefaultConnectTimeout(500);
            pluginResourceManagement.setDefaultTimeout(500);
            plugin = pluginResourceManagement.createPluginAPI("plugin", mockPluginCallback);
        }

        void TearDown() override
        {
            Common::ApplicationConfiguration::restoreApplicationPathManager();
            plugin.reset();
            
            if (server.joinable())
            {
                server.join();
            }
        }

        Common::PluginProtocol::DataMessage createDefaultMessage()
        {
            Common::PluginProtocol::DataMessage dataMessage;
            dataMessage.m_command = Common::PluginProtocol::Commands::PLUGIN_SEND_EVENT;
            dataMessage.m_pluginName = "plugin";
            dataMessage.m_applicationId = "plugin";
            dataMessage.m_acknowledge = true;

            return dataMessage;
        }
        Tests::ScopedReplaceFileSystem m_replacer;
        Common::Logging::ConsoleLoggingSetup m_consoleLogging;
        PluginResourceManagement pluginResourceManagement;
        SingleResponseServer responseServer;
        std::thread server;
        std::shared_ptr<MockedPluginApiCallback> mockPluginCallback;        
        std::unique_ptr<Common::PluginApi::IBaseServiceApi> plugin;

    };

    // cppcheck-suppress syntaxError
    TEST_F(PluginApiTests, pluginAPIcanSendEvent)
    {
        Common::PluginProtocol::DataMessage dataMessage = createDefaultMessage();

        responseServer.setReply(dataMessage);

        EXPECT_NO_THROW(plugin->sendEvent("plugin", "eventContent"));
    }

    TEST_F(PluginApiTests, pluginAPIcanSendEventWithoutAcknowledgmentFails)
    {
        Common::PluginProtocol::DataMessage dataMessage = createDefaultMessage();

        dataMessage.m_acknowledge = false;

        responseServer.setReply(dataMessage);

        EXPECT_THROW(plugin->sendEvent("plugin", "eventContent"), Common::PluginApi::ApiException);
    }

    TEST_F(PluginApiTests, pluginAPIcanSendEventFailIfDifferentCommand)
    {
        Common::PluginProtocol::DataMessage dataMessage = createDefaultMessage();
        dataMessage.m_command = Common::PluginProtocol::Commands::REQUEST_PLUGIN_STATUS;
        responseServer.setReply(dataMessage);
        EXPECT_THROW(plugin->sendEvent("plugin", "eventContent"), Common::PluginApi::ApiException);
    }

    TEST_F(PluginApiTests, pluginAPIcanSendEventFailIfErrorNotEmpty)
    {
        Common::PluginProtocol::DataMessage dataMessage = createDefaultMessage();
        dataMessage.m_error = "Server rejected call";
        responseServer.setReply(dataMessage);
        EXPECT_THROW(plugin->sendEvent("plugin", "eventContent"), Common::PluginApi::ApiException);
    }

    TEST_F(PluginApiTests, pluginAPIcanSendEventFailIfWrongProtocol)
    {
        data_t invalidprotocol({ "not", "valid", "protocol" });
        responseServer.setReplyRaw(invalidprotocol);
        EXPECT_THROW(plugin->sendEvent("plugin", "eventContent"), Common::PluginApi::ApiException);
    }

    TEST_F(PluginApiTests, pluginAPIcanSendEventFailIfNoAnswer)
    {
        responseServer.doNotReply();
        EXPECT_THROW(plugin->sendEvent("plugin", "eventContent"), Common::PluginApi::ApiException);
    }

    TEST_F(PluginApiTests, pluginAPIcanChangeStatusDoesNotFailWithCorrectCommand)
    {
        Common::PluginProtocol::DataMessage dataMessage = createDefaultMessage();
        dataMessage.m_command = Common::PluginProtocol::Commands::PLUGIN_SEND_STATUS;
        responseServer.setReply(dataMessage);
        EXPECT_NO_THROW(plugin->sendStatus("plugin", "statusContent", "statusContentWithoutTimeout"));
    }

    TEST_F(PluginApiTests, pluginAPIcanGetPolicyFailIfErrorNotEmpty)
    {
        Common::PluginProtocol::DataMessage dataMessage = createDefaultMessage();
        dataMessage.m_command = Common::PluginProtocol::Commands::PLUGIN_QUERY_CURRENT_POLICY;
        dataMessage.m_error = "Server rejected call";
        responseServer.setReply(dataMessage);
        EXPECT_THROW(plugin->requestPolicies("plugin"), Common::PluginApi::ApiException);
    }

    TEST_F(PluginApiTests, pluginAPIcanGetPolicyDoesNotFailWithCorrectCommand)
    {
        Common::PluginProtocol::DataMessage dataMessage = createDefaultMessage();
        dataMessage.m_command = Common::PluginProtocol::Commands::PLUGIN_QUERY_CURRENT_POLICY;
        dataMessage.m_payload.clear();
        dataMessage.m_acknowledge = true;
        responseServer.setReply(dataMessage);
        EXPECT_NO_THROW(plugin->requestPolicies("plugin"));
    }

    TEST(PluginRegistrationTests, pluginWillFailToConstructIfNoManagementIsAvaliable)
    {
        Common::Logging::ConsoleLoggingSetup m_consoleLogging;
        MockedApplicationPathManager* mockAppManager = new NiceMock<MockedApplicationPathManager>();
        MockedApplicationPathManager& mock(*mockAppManager);
        ON_CALL(mock, getManagementAgentSocketAddress()).WillByDefault(Return("inproc://management.ipc"));
        ON_CALL(mock, getPluginSocketAddress(_)).WillByDefault(Return("inproc://plugin.ipc"));
        Common::ApplicationConfiguration::replaceApplicationPathManager(
            std::unique_ptr<Common::ApplicationConfiguration::IApplicationPathManager>(mockAppManager));

        auto mockPluginCallback = std::make_shared<NiceMock<MockedPluginApiCallback>>();

        PluginResourceManagement resourceManagement;
        resourceManagement.setDefaultConnectTimeout(500);
        resourceManagement.setDefaultTimeout(500);
        // no management instantiated
        ASSERT_THROW(
            resourceManagement.createPluginAPI("plugin", mockPluginCallback),
            Common::PluginApi::ApiException);
    }

    TEST_F(PluginApiTests, pluginApiCanSendThreatHealthMessageValid)
    {
        Common::PluginProtocol::DataMessage dataMessage = createDefaultMessage();
        dataMessage.m_command = Common::PluginProtocol::Commands::PLUGIN_SEND_THREAT_HEALTH;
        responseServer.setReply(dataMessage);
        EXPECT_NO_THROW(plugin->sendThreatHealth(R"({"ThreatHealth":2})"));
    }

    TEST_F(PluginApiTests, pluginApiCanSendThreatHealthMessageInvalid)
    {
        Common::PluginProtocol::DataMessage dataMessage = createDefaultMessage();
        dataMessage.m_command = Common::PluginProtocol::Commands::PLUGIN_SEND_THREAT_HEALTH;
        responseServer.setReply(dataMessage);
        EXPECT_NO_THROW(plugin->sendThreatHealth("not json"));
    }
} // namespace
