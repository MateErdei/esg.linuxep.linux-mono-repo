/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "MockedPluginApiCallback.h"
#include "SingleResponseServer.h"

#include <tests/Common/ApplicationConfiguration/MockedApplicationPathManager.h>
#include <tests/Common/FileSystemImpl/MockFileSystem.h>
#include <tests/Common/FileSystemImpl/MockFilePermissions.h>

#include <Common/FileSystemImpl/FileSystemImpl.h>
#include <Common/FileSystemImpl/FilePermissionsImpl.h>
#include <Common/ZeroMQWrapper/ISocketRequester.h>
#include <Common/Logging/ConsoleLoggingSetup.h>
#include <Common/PluginApi/IBaseServiceApi.h>
#include <Common/PluginApi/ApiException.h>
#include <Common/PluginApiImpl/PluginResourceManagement.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <thread>

namespace
{
    using data_t = Common::ZeroMQWrapper::IReadable::data_t ;
    using namespace Common::PluginApiImpl;
    using namespace Common::PluginApi;

    using ::testing::NiceMock;
    using ::testing::StrictMock;

    class PluginApiTests : public ::testing::Test
    {

    public:

        void SetUp() override
        {
            MockedApplicationPathManager *mockAppManager = new NiceMock<MockedApplicationPathManager>();
            MockedApplicationPathManager &mock(*mockAppManager);
            ON_CALL(mock, getManagementAgentSocketAddress()).WillByDefault(Return("inproc://management.ipc"));
            ON_CALL(mock, getPluginSocketAddress(_)).WillByDefault(Return("inproc://plugin.ipc"));
            Common::ApplicationConfiguration::replaceApplicationPathManager(
                    std::unique_ptr<Common::ApplicationConfiguration::IApplicationPathManager>(mockAppManager));

            auto mockFileSystem = new StrictMock<MockFileSystem>();
            std::unique_ptr<MockFileSystem> mockIFileSystemPtr = std::unique_ptr<MockFileSystem>(mockFileSystem);
            Common::FileSystem::replaceFileSystem(std::move(mockIFileSystemPtr));

            auto mockFilePermissions = new StrictMock<MockFilePermissions>();
            std::unique_ptr<MockFilePermissions> mockIFilePermissionsPtr = std::unique_ptr<MockFilePermissions>(mockFilePermissions);
            Common::FileSystem::replaceFilePermissions(std::move(mockIFilePermissionsPtr));

            EXPECT_CALL(*mockFilePermissions, chmod(_,_)).WillRepeatedly(Return());
            EXPECT_CALL(*mockFilePermissions, chown(_,_,_)).WillRepeatedly(Return());

            mockPluginCallback = std::make_shared<NiceMock<MockedPluginApiCallback>>();

            auto context = pluginResourceManagement.getSocketContext();
            server = std::thread(std::ref(responseServer), context);
            plugin = pluginResourceManagement.createPluginAPI("plugin", mockPluginCallback );
        }
        void TearDown() override
        {
            Common::ApplicationConfiguration::restoreApplicationPathManager();
            plugin.reset();
            if ( server.joinable())
            {
                server.join();
            }
        }

        Common::PluginProtocol::DataMessage createDefaultMessage()
        {
            Common::PluginProtocol::DataMessage dataMessage;
            dataMessage.Command = Common::PluginProtocol::Commands::PLUGIN_SEND_EVENT;
            dataMessage.ProtocolVersion = Common::PluginProtocol::ProtocolSerializerFactory::ProtocolVersion;
            dataMessage.ApplicationId = "plugin";
            dataMessage.MessageId = "1";
            dataMessage.Payload.emplace_back("ACK");

            return dataMessage;
        }
        Common::Logging::ConsoleLoggingSetup m_consoleLogging;
        PluginResourceManagement pluginResourceManagement;
        SingleResponseServer responseServer;
        std::thread server;
        std::shared_ptr<MockedPluginApiCallback> mockPluginCallback;
        std::unique_ptr<Common::PluginApi::IBaseServiceApi> plugin;

    };


    TEST_F(PluginApiTests, pluginAPIcanSendEvent) //NOLINT
    {
        Common::PluginProtocol::DataMessage dataMessage = createDefaultMessage();

        responseServer.setReply(dataMessage);

        EXPECT_NO_THROW(plugin->sendEvent("plugin", "eventContent")); //NOLINT
    }

    TEST_F(PluginApiTests, pluginAPIcanSendEventWithoutPayloadFails) //NOLINT
    {
        Common::PluginProtocol::DataMessage dataMessage = createDefaultMessage();

        dataMessage.Payload.clear();

        responseServer.setReply(dataMessage);

        EXPECT_THROW(plugin->sendEvent("plugin", "eventContent"), Common::PluginApi::ApiException); //NOLINT

    }

    TEST_F(PluginApiTests, pluginAPIcanSendEventFailIfDifferentCommand) //NOLINT
    {
        Common::PluginProtocol::DataMessage dataMessage = createDefaultMessage();
        dataMessage.Command = Common::PluginProtocol::Commands::REQUEST_PLUGIN_STATUS;
        responseServer.setReply(dataMessage);
        EXPECT_THROW(plugin->sendEvent("plugin", "eventContent"), Common::PluginApi::ApiException); //NOLINT
    }

    TEST_F(PluginApiTests, pluginAPIcanSendEventFailIfErrorNotEmpty) //NOLINT
    {
        Common::PluginProtocol::DataMessage dataMessage = createDefaultMessage();
        dataMessage.Error = "Server rejected call";
        responseServer.setReply(dataMessage);
        EXPECT_THROW(plugin->sendEvent("plugin", "eventContent"), Common::PluginApi::ApiException); //NOLINT
    }


    TEST_F(PluginApiTests, pluginAPIcanSendEventFailIfWrongProtocol) //NOLINT
    {
        data_t invalidprotocol({"not", "valid", "protocol"});
        responseServer.setReplyRaw(invalidprotocol);
        EXPECT_THROW(plugin->sendEvent("plugin", "eventContent"), Common::PluginApi::ApiException); //NOLINT
    }


//    TEST_F(PluginApiTests, pluginAPIcanSendEventFailIfNoAnswer) //NOLINT
//    {
//        responseServer.doNotReply();
//        // attention: this require PluginResrouceManager to configure the timeout of plugin.
//        EXPECT_THROW(plugin->sendEvent("plugin", "eventContent"), Common::PluginApi::ApiException); //NOLINT
//    }

    TEST_F(PluginApiTests, pluginAPIcanChangeStatusDoesNotFailWithCorrectCommand) //NOLINT
    {
        Common::PluginProtocol::DataMessage dataMessage = createDefaultMessage();
        dataMessage.Command = Common::PluginProtocol::Commands::PLUGIN_SEND_STATUS;
        responseServer.setReply(dataMessage);
        EXPECT_NO_THROW(plugin->sendStatus("plugin", "statusContent", "statusContentWithoutTimeout")); //NOLINT
    }

    TEST_F(PluginApiTests, pluginAPIcanGetPolicyFailIfErrorNotEmpty) //NOLINT
    {
        Common::PluginProtocol::DataMessage dataMessage = createDefaultMessage();
        dataMessage.Command = Common::PluginProtocol::Commands::PLUGIN_QUERY_CURRENT_POLICY;
        dataMessage.Error = "Server rejected call";
        responseServer.setReply(dataMessage);
        EXPECT_THROW(plugin->requestPolicies("plugin"), Common::PluginApi::ApiException); //NOLINT
    }

    TEST_F(PluginApiTests, pluginAPIcanGetPolicyDoesNotFailWithCorrectCommand) //NOLINT
    {
        Common::PluginProtocol::DataMessage dataMessage = createDefaultMessage();
        dataMessage.Command = Common::PluginProtocol::Commands::PLUGIN_QUERY_CURRENT_POLICY;
        dataMessage.Payload.clear();
        dataMessage.Payload.emplace_back("ACK");
        responseServer.setReply(dataMessage);
        EXPECT_NO_THROW(plugin->requestPolicies("plugin")); //NOLINT
    }

    TEST( PluginRegistrationTests, pluginWillFailToConstructIfNoManagementIsAvaliable) //NOLINT
    {
        Common::Logging::ConsoleLoggingSetup m_consoleLogging;
        MockedApplicationPathManager *mockAppManager = new NiceMock<MockedApplicationPathManager>();
        MockedApplicationPathManager &mock(*mockAppManager);
        ON_CALL(mock, getManagementAgentSocketAddress()).WillByDefault(Return("inproc://management.ipc"));
        ON_CALL(mock, getPluginSocketAddress(_)).WillByDefault(Return("inproc://plugin.ipc"));
        Common::ApplicationConfiguration::replaceApplicationPathManager(
                std::unique_ptr<Common::ApplicationConfiguration::IApplicationPathManager>(mockAppManager));

        auto mockPluginCallback = std::make_shared<NiceMock<MockedPluginApiCallback>>();

        PluginResourceManagement resourceManagement;
        resourceManagement.setDefaultConnectTimeout(500);
        resourceManagement.setDefaultTimeout(500); 
        // no management instantiated
        ASSERT_THROW(resourceManagement.createPluginAPI("plugin", mockPluginCallback), Common::PluginApi::ApiException); //NOLINT

    }
}
