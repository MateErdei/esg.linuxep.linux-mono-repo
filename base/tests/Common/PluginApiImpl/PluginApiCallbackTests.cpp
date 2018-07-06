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
#include <Common/PluginApiImpl/MessageBuilder.h>

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


    class PluginApiCallbackTests : public ::testing::Test
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
            std::thread registration(handleRegistration, std::ref(pluginResourceManagement.socketContext() ));

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
                dataMessage.payload.push_back(firstPayloadItem);
            }

            return dataMessage;
        }

        Common::ZeroMQWrapper::IContext & context()
        {
            return pluginResourceManagement.socketContext();
        }

        MockedPluginApiCallback & mock()
        {
            MockedPluginApiCallback * mockPtr = mockPluginCallback.get();
            return *mockPtr;
        }

        ::testing::AssertionResult dataMessageSimilar( const char* m_expr,
                                                          const char* n_expr,
                                                          const Common::PluginApi::DataMessage  & expected,
                                                          const Common::PluginApi::DataMessage  & resulted)
        {
            std::stringstream s;
            s<< m_expr << " and " << n_expr << " failed: ";

            if( expected.ProtocolVersion != resulted.ProtocolVersion)
            {
                return ::testing::AssertionFailure() << s.str() << " Protocol differ: \n expected: "
                                                     <<  expected.ProtocolVersion
                                                     << "\n result: " <<  resulted.ProtocolVersion;
            }

            if( expected.ApplicationId != resulted.ApplicationId)
            {
                return ::testing::AssertionFailure() << s.str() << " Application Id differ: \n expected: "
                                                     <<  expected.ApplicationId
                                                     << "\n result: " <<  resulted.ApplicationId;
            }

            if( expected.MessageId != resulted.MessageId)
            {
                return ::testing::AssertionFailure() << s.str() << " Message Id differ: \n expected: "
                                                     <<  expected.MessageId
                                                     << "\n result: " <<  resulted.MessageId;
            }

            if( expected.Command != resulted.Command)
            {
                return ::testing::AssertionFailure() << s.str() << " command differ: \n expected: "
                                                     <<  Common::PluginApi::SerializeCommand(expected.Command)
                                                     << "\n result: " <<  Common::PluginApi::SerializeCommand(resulted.Command);
            }

            if( expected.Error != resulted.Error)
            {
                return ::testing::AssertionFailure() << s.str() << " Error message differ: \n expected: "
                                                     <<  expected.Error
                                                     << "\n result: " <<  resulted.Error;
            }

            if( expected.payload != resulted.payload)
            {
                return ::testing::AssertionFailure() << s.str() << " Payload differ: \n expected: "
                                                     <<  PrintToString(expected.payload)
                                                     << "\n result: " <<  PrintToString(resulted.payload);
            }

            return ::testing::AssertionSuccess();
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
        expectedAnswer.payload.clear();
        expectedAnswer.payload.push_back(statusInfo.statusXml);
        expectedAnswer.payload.push_back(statusInfo.statusWithoutXml);

        EXPECT_CALL(mock(), getStatus()).WillOnce(Return(statusInfo));

        auto reply = managementRequest.triggerRequest(context(), dataMessage);

        EXPECT_PRED_FORMAT2( dataMessageSimilar, expectedAnswer, reply );
    }


    TEST_F(PluginApiCallbackTests, pluginAPICallbackcanRespondToStatusFail)
    {
        Common::PluginApi::DataMessage dataMessage = createDefaultMessage(Common::PluginApi::Commands::REQUEST_PLUGIN_STATUS, "");
        Common::PluginApi::DataMessage expectedAnswer(dataMessage);
        Common::PluginApi::StatusInfo statusInfo{"statusContent","statusNoTimestamp"};
        expectedAnswer.payload.clear();
        expectedAnswer.payload.push_back(statusInfo.statusXml);
        expectedAnswer.payload.push_back(statusInfo.statusWithoutXml);

        dataMessage.payload.clear();
        dataMessage.ProtocolVersion = "invalid";
        auto reply = managementRequest.triggerRequest(context(), dataMessage);
        EXPECT_EQ(reply.Error, "Protocol not supported");
    }

    TEST_F(PluginApiCallbackTests, pluginAPICallbackcanRespondToTelemetry)
    {
        Common::PluginApi::DataMessage dataMessage = createDefaultMessage(Common::PluginApi::Commands::REQUEST_PLUGIN_TELEMETRY, "");
        Common::PluginApi::DataMessage expectedAnswer(dataMessage);

        std::string telemetryData = "TelemetryData";
        expectedAnswer.payload.clear();
        expectedAnswer.payload.push_back(telemetryData);

        EXPECT_CALL(mock(), getTelemetry()).WillOnce(Return(telemetryData));

        auto reply = managementRequest.triggerRequest(context(), dataMessage);

        EXPECT_PRED_FORMAT2( dataMessageSimilar, expectedAnswer, reply );
    }


    TEST_F(PluginApiCallbackTests, pluginAPICallbackcanRespondToTelemetryFail)
    {
        Common::PluginApi::DataMessage dataMessage = createDefaultMessage(Common::PluginApi::Commands::REQUEST_PLUGIN_TELEMETRY, "");
        Common::PluginApi::DataMessage expectedAnswer(dataMessage);

        std::string telemetryData = "TelemetryData";
        expectedAnswer.payload.clear();
        expectedAnswer.payload.push_back(telemetryData);

        dataMessage.payload.clear();
        dataMessage.ProtocolVersion = "invalid";
        auto reply = managementRequest.triggerRequest(context(), dataMessage);
        EXPECT_EQ(reply.Error, "Protocol not supported");
    }

    TEST_F(PluginApiCallbackTests, pluginAPICallbackcanRespondToDoAction)
    {
        Common::PluginApi::DataMessage dataMessage = createDefaultMessage(Common::PluginApi::Commands::REQUEST_PLUGIN_DO_ACTION, "contentOfAction");
        Common::PluginApi::DataMessage expectedAnswer(dataMessage);

        expectedAnswer.payload.clear();
        expectedAnswer.payload.push_back("ACK");

        EXPECT_CALL(mock(), doAction(_));

        auto reply = managementRequest.triggerRequest(context(), dataMessage);

        EXPECT_PRED_FORMAT2( dataMessageSimilar, expectedAnswer, reply );
    }


    TEST_F(PluginApiCallbackTests, pluginAPICallbackcanRespondToDoActionFail)
    {
        Common::PluginApi::DataMessage dataMessage = createDefaultMessage(Common::PluginApi::Commands::REQUEST_PLUGIN_DO_ACTION, "");
        Common::PluginApi::DataMessage expectedAnswer(dataMessage);

        std::string actionData = "ActionData";
        expectedAnswer.payload.clear();
        expectedAnswer.payload.push_back(actionData);

        dataMessage.payload.clear();
        dataMessage.ProtocolVersion = "invalid";
        auto reply = managementRequest.triggerRequest(context(), dataMessage);
        EXPECT_EQ(reply.Error, "Protocol not supported");
    }

    TEST_F(PluginApiCallbackTests, pluginAPICallbackcanRespondToApplyNewPolicy)
    {
        Common::PluginApi::DataMessage dataMessage = createDefaultMessage(Common::PluginApi::Commands::REQUEST_PLUGIN_APPLY_POLICY, "contentOfPolicy");
        Common::PluginApi::DataMessage expectedAnswer(dataMessage);

        expectedAnswer.payload.clear();
        expectedAnswer.payload.push_back("ACK");

        EXPECT_CALL(mock(), applyNewPolicy(_));

        auto reply = managementRequest.triggerRequest(context(), dataMessage);

        EXPECT_PRED_FORMAT2( dataMessageSimilar, expectedAnswer, reply );
    }

    TEST_F(PluginApiCallbackTests, pluginAPICallbackcanRespondToApplyNewPolicyFail)
    {
        Common::PluginApi::DataMessage dataMessage = createDefaultMessage(Common::PluginApi::Commands::REQUEST_PLUGIN_APPLY_POLICY, "");
        Common::PluginApi::DataMessage expectedAnswer(dataMessage);

        std::string policyData = "PolicyData";
        expectedAnswer.payload.clear();
        expectedAnswer.payload.push_back(policyData);

        dataMessage.payload.clear();
        dataMessage.ProtocolVersion = "invalid";
        auto reply = managementRequest.triggerRequest(context(), dataMessage);
        EXPECT_EQ(reply.Error, "Protocol not supported");
    }

}
