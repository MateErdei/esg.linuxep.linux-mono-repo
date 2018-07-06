/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "MockedApplicationPathManager.h"
#include "MockedPluginApiCallback.h"
#include "SingleManagementRequest.h"

#include "Common/ZeroMQWrapper/ISocketRequester.h"
#include "Common/PluginApi/IPluginApi.h"
#include "Common/PluginApi/ApiException.h"
#include "Common/PluginApiImpl/PluginResourceManagement.h"
#include "Common/ZeroMQWrapper/ISocketReplier.h"
#include "Common/PluginApiImpl/MessageBuilder.h"
#include "TestCompare.h"

#include <thread>

namespace
{

    void handleRegistration(Common::ZeroMQWrapper::IContext & context)
    {
        auto replier = context.getReplier();
        std::string address = Common::ApplicationConfiguration::applicationPathManager().getManagementAgentSocketAddress();
        replier->listen(address );

        // handle registration
        Common::PluginApiImpl::Protocol protocol;
        auto request = protocol.deserialize(replier->read());
        assert( request.Command == Common::PluginApi::Commands::PLUGIN_SEND_REGISTER);
        Common::PluginApiImpl::MessageBuilder messageBuilder( "plugin", "v1");
        auto replyMessage = protocol.serialize( messageBuilder.replyAckMessage(request) );
        replier->write(replyMessage);
    }


    using data_t = Common::ZeroMQWrapper::IReadable::data_t ;
    using namespace Common::PluginApiImpl;
    using namespace Common::PluginApi;

    using ::testing::NiceMock;
    using ::testing::StrictMock;


    class PluginApiCallbackTests : public TestCompare
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
            std::thread registration(handleRegistration, std::ref(pluginResourceManagement.getSocketContext() ));

            plugin = pluginResourceManagement.createPluginAPI("plugin", mockPluginCallback );
            registration.join();

        }
        void TearDown() override
        {
            Common::ApplicationConfiguration::restoreApplicationPathManager();
            plugin.reset();

        }

        Common::PluginApi::DataMessage createDefaultMessage(Common::PluginApi::Commands command, const std::string &firstPayloadItem )
        {
            Common::PluginApi::DataMessage dataMessage;
            dataMessage.Command = command;
            dataMessage.ProtocolVersion = Common::PluginApiImpl::ProtocolSerializerFactory::ProtocolVersion;
            dataMessage.ApplicationId = "plugin";
            dataMessage.MessageId = "1";
            if ( !firstPayloadItem.empty())
            {
                dataMessage.Payload.push_back(firstPayloadItem);
            }

            return dataMessage;
        }

        Common::ZeroMQWrapper::IContext & context()
        {
            return pluginResourceManagement.getSocketContext();
        }

        MockedPluginApiCallback & mock()
        {
            MockedPluginApiCallback * mockPtr = mockPluginCallback.get();
            return *mockPtr;
        }

        PluginResourceManagement pluginResourceManagement;
        SingleManagementRequest managementRequest;

        std::shared_ptr<MockedPluginApiCallback> mockPluginCallback;
        std::unique_ptr<Common::PluginApi::IPluginApi> plugin;

    };


    TEST_F(PluginApiCallbackTests, pluginAPICallbackcanRespondToStatus)
    {
        Common::PluginApi::DataMessage dataMessage = createDefaultMessage(Common::PluginApi::Commands::REQUEST_PLUGIN_STATUS, "");
        Common::PluginApi::DataMessage expectedAnswer(dataMessage);
        Common::PluginApi::StatusInfo statusInfo{"statusContent","statusNoTimestamp"};
        expectedAnswer.Payload.clear();
        expectedAnswer.Payload.push_back(statusInfo.statuxXml);
        expectedAnswer.Payload.push_back(statusInfo.statusWithoutXml);

        EXPECT_CALL(mock(), getStatus()).WillOnce(Return(statusInfo));

        auto reply = managementRequest.triggerRequest(context(), dataMessage);

        EXPECT_PRED_FORMAT2( dataMessageSimilar, expectedAnswer, reply );
    }


    TEST_F(PluginApiCallbackTests, pluginAPICallbackcanRespondToStatusFail)
    {
        Common::PluginApi::DataMessage dataMessage = createDefaultMessage(Common::PluginApi::Commands::REQUEST_PLUGIN_STATUS, "");
        Common::PluginApi::DataMessage expectedAnswer(dataMessage);
        Common::PluginApi::StatusInfo statusInfo{"statusContent","statusNoTimestamp"};
        expectedAnswer.Payload.clear();
        expectedAnswer.Payload.push_back(statusInfo.statuxXml);
        expectedAnswer.Payload.push_back(statusInfo.statusWithoutXml);

        dataMessage.Payload.clear();
        dataMessage.ProtocolVersion = "invalid";
        auto reply = managementRequest.triggerRequest(context(), dataMessage);
        EXPECT_EQ(reply.Error, "Protocol not supported");
    }

    TEST_F(PluginApiCallbackTests, pluginAPICallbackcanRespondToTelemetry)
    {
        Common::PluginApi::DataMessage dataMessage = createDefaultMessage(Common::PluginApi::Commands::REQUEST_PLUGIN_TELEMETRY, "");
        Common::PluginApi::DataMessage expectedAnswer(dataMessage);

        std::string telemetryData = "TelemetryData";
        expectedAnswer.Payload.clear();
        expectedAnswer.Payload.push_back(telemetryData);

        EXPECT_CALL(mock(), getTelemetry()).WillOnce(Return(telemetryData));

        auto reply = managementRequest.triggerRequest(context(), dataMessage);

        EXPECT_PRED_FORMAT2( dataMessageSimilar, expectedAnswer, reply );
    }


    TEST_F(PluginApiCallbackTests, pluginAPICallbackcanRespondToTelemetryFail)
    {
        Common::PluginApi::DataMessage dataMessage = createDefaultMessage(Common::PluginApi::Commands::REQUEST_PLUGIN_TELEMETRY, "");
        Common::PluginApi::DataMessage expectedAnswer(dataMessage);

        std::string telemetryData = "TelemetryData";
        expectedAnswer.Payload.clear();
        expectedAnswer.Payload.push_back(telemetryData);

        dataMessage.Payload.clear();
        dataMessage.ProtocolVersion = "invalid";
        auto reply = managementRequest.triggerRequest(context(), dataMessage);
        EXPECT_EQ(reply.Error, "Protocol not supported");
    }

    TEST_F(PluginApiCallbackTests, pluginAPICallbackcanRespondToDoAction)
    {
        Common::PluginApi::DataMessage dataMessage = createDefaultMessage(Common::PluginApi::Commands::REQUEST_PLUGIN_DO_ACTION, "contentOfAction");
        Common::PluginApi::DataMessage expectedAnswer(dataMessage);

        expectedAnswer.Payload.clear();
        expectedAnswer.Payload.push_back("ACK");

        EXPECT_CALL(mock(), doAction(_));

        auto reply = managementRequest.triggerRequest(context(), dataMessage);

        EXPECT_PRED_FORMAT2( dataMessageSimilar, expectedAnswer, reply );
    }


    TEST_F(PluginApiCallbackTests, pluginAPICallbackcanRespondToDoActionFail)
    {
        Common::PluginApi::DataMessage dataMessage = createDefaultMessage(Common::PluginApi::Commands::REQUEST_PLUGIN_DO_ACTION, "");
        Common::PluginApi::DataMessage expectedAnswer(dataMessage);

        std::string actionData = "ActionData";
        expectedAnswer.Payload.clear();
        expectedAnswer.Payload.push_back(actionData);

        dataMessage.Payload.clear();
        dataMessage.ProtocolVersion = "invalid";
        auto reply = managementRequest.triggerRequest(context(), dataMessage);
        EXPECT_EQ(reply.Error, "Protocol not supported");
    }

    TEST_F(PluginApiCallbackTests, pluginAPICallbackcanRespondToApplyNewPolicy)
    {
        Common::PluginApi::DataMessage dataMessage = createDefaultMessage(Common::PluginApi::Commands::REQUEST_PLUGIN_APPLY_POLICY, "contentOfPolicy");
        Common::PluginApi::DataMessage expectedAnswer(dataMessage);

        expectedAnswer.Payload.clear();
        expectedAnswer.Payload.push_back("ACK");

        EXPECT_CALL(mock(), applyNewPolicy(_));

        auto reply = managementRequest.triggerRequest(context(), dataMessage);

        EXPECT_PRED_FORMAT2( dataMessageSimilar, expectedAnswer, reply );
    }

    TEST_F(PluginApiCallbackTests, pluginAPICallbackcanRespondToApplyNewPolicyFail)
    {
        Common::PluginApi::DataMessage dataMessage = createDefaultMessage(Common::PluginApi::Commands::REQUEST_PLUGIN_APPLY_POLICY, "");
        Common::PluginApi::DataMessage expectedAnswer(dataMessage);

        std::string policyData = "PolicyData";
        expectedAnswer.Payload.clear();
        expectedAnswer.Payload.push_back(policyData);

        dataMessage.Payload.clear();
        dataMessage.ProtocolVersion = "invalid";
        auto reply = managementRequest.triggerRequest(context(), dataMessage);
        EXPECT_EQ(reply.Error, "Protocol not supported");
    }

}
