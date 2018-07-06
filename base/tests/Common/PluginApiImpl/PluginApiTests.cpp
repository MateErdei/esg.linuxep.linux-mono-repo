/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "MockedApplicationPathManager.h"
#include "MockedPluginApiCallback.h"
#include "SingleResponseServer.h"

#include "Common/ZeroMQWrapper/ISocketRequester.h"
#include "Common/PluginApi/IPluginApi.h"
#include "Common/PluginApi/ApiException.h"
#include "Common/PluginApiImpl/PluginResourceManagement.h"
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
            mockPluginCallback = std::make_shared<NiceMock<MockedPluginApiCallback>>();
            pluginResourceManagement.setDefaultConnectTimeout(3000);
            auto & context = pluginResourceManagement.getSocketContext();
            server = std::thread(std::ref(responseServer), std::ref(context));
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

        Common::PluginApi::DataMessage createDefaultMessage()
        {
            Common::PluginApi::DataMessage dataMessage;
            dataMessage.Command = Common::PluginApi::Commands::PLUGIN_SEND_EVENT;
            dataMessage.ProtocolVersion = Common::PluginApiImpl::ProtocolSerializerFactory::ProtocolVersion;
            dataMessage.ApplicationId = "plugin";
            dataMessage.MessageId = "1";
            dataMessage.Payload.push_back("ACK");

            return dataMessage;
        }

        PluginResourceManagement pluginResourceManagement;
        SingleResponseServer responseServer;
        std::thread server;
        std::shared_ptr<MockedPluginApiCallback> mockPluginCallback;
        std::unique_ptr<Common::PluginApi::IPluginApi> plugin;

    };


    TEST_F(PluginApiTests, pluginAPIcanSendEvent)
    {
        Common::PluginApi::DataMessage dataMessage = createDefaultMessage();

        responseServer.setReply(dataMessage);

        EXPECT_NO_THROW(plugin->sendEvent("plugin", "eventContent"));
    }

    TEST_F(PluginApiTests, pluginAPIcanSendEventWithoutPayloadFails)
    {
        Common::PluginApi::DataMessage dataMessage = createDefaultMessage();

        dataMessage.Payload.clear();

        responseServer.setReply(dataMessage);

        EXPECT_THROW(plugin->sendEvent("plugin", "eventContent"), Common::PluginApi::ApiException);

    }

    TEST_F(PluginApiTests, pluginAPIcanSendEventFailIfDifferentCommand)
    {
        Common::PluginApi::DataMessage dataMessage = createDefaultMessage();
        dataMessage.Command = Common::PluginApi::Commands::REQUEST_PLUGIN_STATUS;
        responseServer.setReply(dataMessage);
        EXPECT_THROW(plugin->sendEvent("plugin", "eventContent"), Common::PluginApi::ApiException);
    }

    TEST_F(PluginApiTests, pluginAPIcanSendEventFailIfErrorNotEmpty)
    {
        Common::PluginApi::DataMessage dataMessage = createDefaultMessage();
        dataMessage.Error = "Server rejected call";
        responseServer.setReply(dataMessage);
        EXPECT_THROW(plugin->sendEvent("plugin", "eventContent"), Common::PluginApi::ApiException);
    }


    TEST_F(PluginApiTests, pluginAPIcanSendEventFailIfWrongProtocol)
    {
        data_t invalidprotocol({"not", "valid", "protocol"});
        responseServer.setReplyRaw(invalidprotocol);
        EXPECT_THROW(plugin->sendEvent("plugin", "eventContent"), Common::PluginApi::ApiException);
    }


//    TEST_F(PluginApiTests, pluginAPIcanSendEventFailIfNoAnswer)
//    {
//        responseServer.doNotReply();
//        // attention: this require PluginResrouceManager to configure the timeout of plugin.
//        EXPECT_THROW(plugin->sendEvent("plugin", "eventContent"), Common::PluginApi::ApiException);
//    }

    TEST_F(PluginApiTests, pluginAPIcanChangeStatusDoesNotFailWithCorrectCommand)
    {
        Common::PluginApi::DataMessage dataMessage = createDefaultMessage();
        dataMessage.Command = Common::PluginApi::Commands::PLUGIN_SEND_STATUS;
        responseServer.setReply(dataMessage);
        EXPECT_NO_THROW(plugin->changeStatus("plugin", "statusContent", "statusContentWithoutTimeout"));
    }

    TEST_F(PluginApiTests, pluginAPIcanGetPolicyFailIfErrorNotEmpty)
    {
        Common::PluginApi::DataMessage dataMessage = createDefaultMessage();
        dataMessage.Command = Common::PluginApi::Commands::PLUGIN_QUERY_CURRENT_POLICY;
        dataMessage.Error = "Server rejected call";
        responseServer.setReply(dataMessage);
        EXPECT_THROW(plugin->getPolicy("plugin"), Common::PluginApi::ApiException);
    }

    TEST_F(PluginApiTests, pluginAPIcanGetPolicyDoesNotFailWithCorrectCommand)
    {
        Common::PluginApi::DataMessage dataMessage = createDefaultMessage();
        dataMessage.Command = Common::PluginApi::Commands::PLUGIN_QUERY_CURRENT_POLICY;
        dataMessage.Payload.clear();
        dataMessage.Payload.push_back("testPolicyPayload");
        responseServer.setReply(dataMessage);
        EXPECT_EQ(plugin->getPolicy("plugin"), dataMessage.Payload.at(0));
    }




}
