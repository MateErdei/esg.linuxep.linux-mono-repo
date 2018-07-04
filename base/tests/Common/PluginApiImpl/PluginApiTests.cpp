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
            plugin = pluginResourceManagement.createPluginAPI("plugin", mockPluginCallback );
            auto & context = pluginResourceManagement.socketContext();
            server = std::thread(std::ref(responseServer), std::ref(context));
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
            dataMessage.payload.push_back("ACK");

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

        dataMessage.payload.clear();

        responseServer.setReply(dataMessage);

        EXPECT_THROW(plugin->sendEvent("plugin", "eventContent"), Common::PluginApi::ApiException);

    }


}
