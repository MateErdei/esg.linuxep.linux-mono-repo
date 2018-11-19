/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "MockedPluginApiCallback.h"
#include "SingleManagementRequest.h"
#include "TestCompare.h"

#include <tests/Common/ApplicationConfiguration/MockedApplicationPathManager.h>
#include <tests/Common/FileSystemImpl/MockFileSystem.h>
#include <tests/Common/FileSystemImpl/MockFilePermissions.h>

#include <Common/FileSystemImpl/FilePermissionsImpl.h>
#include <Common/FileSystemImpl/FileSystemImpl.h>
#include <Common/Logging/ConsoleLoggingSetup.h>
#include <Common/PluginApi/IBaseServiceApi.h>
#include <Common/PluginApi/ApiException.h>
#include <Common/PluginApiImpl/PluginResourceManagement.h>
#include <Common/PluginProtocol/MessageBuilder.h>
#include <Common/ZeroMQWrapper/ISocketReplier.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <thread>

namespace
{

    void handleRegistration(const Common::ZeroMQWrapper::IContextSharedPtr& context)
    {
        auto replier = context->getReplier();
        std::string address = Common::ApplicationConfiguration::applicationPathManager().getManagementAgentSocketAddress();
        replier->listen(address );

        // handle registration
        Common::PluginProtocol::Protocol protocol;
        auto request = protocol.deserialize(replier->read());
        assert( request.Command == Common::PluginProtocol::Commands::PLUGIN_SEND_REGISTER);
        Common::PluginProtocol::MessageBuilder messageBuilder("v1", "plugin");
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
            defaultPluginName = "plugin";
            defaultAppId = "pluginApp";
            MockedApplicationPathManager *mockAppManager = new NiceMock<MockedApplicationPathManager>();
            MockedApplicationPathManager &mock(*mockAppManager);
            ON_CALL(mock, getManagementAgentSocketAddress()).WillByDefault(Return("inproc://management.ipc"));
            ON_CALL(mock, getPluginSocketAddress(_)).WillByDefault(Return("inproc://plugin.ipc"));
            Common::ApplicationConfiguration::replaceApplicationPathManager(
                    std::unique_ptr<Common::ApplicationConfiguration::IApplicationPathManager>(mockAppManager));
            mockPluginCallback = std::make_shared<NiceMock<MockedPluginApiCallback>>();

            std::thread registration(handleRegistration, pluginResourceManagement.getSocketContext() );

            auto mockFileSystem = new StrictMock<MockFileSystem>();
            std::unique_ptr<MockFileSystem> mockIFileSystemPtr = std::unique_ptr<MockFileSystem>(mockFileSystem);
            Common::FileSystem::replaceFileSystem(std::move(mockIFileSystemPtr));

            auto mockFilePermissions = new StrictMock<MockFilePermissions>();
            std::unique_ptr<MockFilePermissions> mockIFilePermissionsPtr = std::unique_ptr<MockFilePermissions>(mockFilePermissions);
            Common::FileSystem::replaceFilePermissions(std::move(mockIFilePermissionsPtr));

            EXPECT_CALL(*mockFilePermissions, chmod(_,_)).WillRepeatedly(Return());
            EXPECT_CALL(*mockFilePermissions, chown(_,_,_)).WillRepeatedly(Return());

            plugin = pluginResourceManagement.createPluginAPI("plugin", mockPluginCallback );
            registration.join();

        }
        void TearDown() override
        {
            Common::ApplicationConfiguration::restoreApplicationPathManager();
            plugin.reset();

        }

        Common::PluginProtocol::DataMessage createDefaultMessage(Common::PluginProtocol::Commands command, const std::string &firstPayloadItem )
        {
            Common::PluginProtocol::DataMessage dataMessage;
            dataMessage.Command = command;
            dataMessage.ProtocolVersion = Common::PluginProtocol::ProtocolSerializerFactory::ProtocolVersion;
            dataMessage.ApplicationId = defaultAppId;
            dataMessage.PluginName = defaultPluginName;
            dataMessage.MessageId = "1";
            if ( !firstPayloadItem.empty())
            {
                dataMessage.Payload.push_back(firstPayloadItem);
            }

            return dataMessage;
        }

        Common::ZeroMQWrapper::IContextSharedPtr context()
        {
            return pluginResourceManagement.getSocketContext();
        }

        MockedPluginApiCallback & mock()
        {
            MockedPluginApiCallback * mockPtr = mockPluginCallback.get();
            return *mockPtr;
        }
        Common::Logging::ConsoleLoggingSetup m_consoleLogging;
        std::string defaultAppId;
        std::string defaultPluginName;
        PluginResourceManagement pluginResourceManagement;
        SingleManagementRequest managementRequest;

        std::shared_ptr<MockedPluginApiCallback> mockPluginCallback;
        std::unique_ptr<Common::PluginApi::IBaseServiceApi> plugin;

    };


    TEST_F(PluginApiCallbackTests, pluginAPICallbackcanRespondToStatus) // NOLINT
    {
        Common::PluginProtocol::DataMessage dataMessage = createDefaultMessage(Common::PluginProtocol::Commands::REQUEST_PLUGIN_STATUS, "");
        Common::PluginProtocol::DataMessage expectedAnswer(dataMessage);
        Common::PluginApi::StatusInfo statusInfo{"statusContent","statusNoTimestamp"};
        expectedAnswer.Payload.clear();
        expectedAnswer.Payload.push_back(statusInfo.statusXml);
        expectedAnswer.Payload.push_back(statusInfo.statusWithoutTimestampsXml);

        EXPECT_CALL(mock(), getStatus(defaultAppId)).WillOnce(Return(statusInfo));

        auto reply = managementRequest.triggerRequest(context(), dataMessage);

        EXPECT_PRED_FORMAT2( dataMessageSimilar, expectedAnswer, reply );
    }


    TEST_F(PluginApiCallbackTests, pluginAPICallbackcanRespondToStatusFail) // NOLINT
    {
        Common::PluginProtocol::DataMessage dataMessage = createDefaultMessage(Common::PluginProtocol::Commands::REQUEST_PLUGIN_STATUS, "");
        Common::PluginProtocol::DataMessage expectedAnswer(dataMessage);
        Common::PluginApi::StatusInfo statusInfo{"statusContent","statusNoTimestamp"};
        expectedAnswer.Payload.clear();
        expectedAnswer.Payload.push_back(statusInfo.statusXml);
        expectedAnswer.Payload.push_back(statusInfo.statusWithoutTimestampsXml);

        dataMessage.Payload.clear();
        dataMessage.ProtocolVersion = "invalid";
        auto reply = managementRequest.triggerRequest(context(), dataMessage);
        EXPECT_EQ(reply.Error, "Protocol not supported");
    }

    TEST_F(PluginApiCallbackTests, pluginAPICallbackcanRespondToTelemetry) // NOLINT
    {
        Common::PluginProtocol::DataMessage dataMessage = createDefaultMessage(Common::PluginProtocol::Commands::REQUEST_PLUGIN_TELEMETRY, "");
        Common::PluginProtocol::DataMessage expectedAnswer(dataMessage);

        std::string telemetryData = "TelemetryData";
        expectedAnswer.Payload.clear();
        expectedAnswer.Payload.push_back(telemetryData);

        EXPECT_CALL(mock(), getTelemetry()).WillOnce(Return(telemetryData));

        auto reply = managementRequest.triggerRequest(context(), dataMessage);

        EXPECT_PRED_FORMAT2( dataMessageSimilar, expectedAnswer, reply );
    }


    TEST_F(PluginApiCallbackTests, pluginAPICallbackcanRespondToTelemetryFail) // NOLINT
    {
        Common::PluginProtocol::DataMessage dataMessage = createDefaultMessage(Common::PluginProtocol::Commands::REQUEST_PLUGIN_TELEMETRY, "");
        Common::PluginProtocol::DataMessage expectedAnswer(dataMessage);

        std::string telemetryData = "TelemetryData";
        expectedAnswer.Payload.clear();
        expectedAnswer.Payload.push_back(telemetryData);

        dataMessage.Payload.clear();
        dataMessage.ProtocolVersion = "invalid";
        auto reply = managementRequest.triggerRequest(context(), dataMessage);
        EXPECT_EQ(reply.Error, "Protocol not supported");
    }

    TEST_F(PluginApiCallbackTests, pluginAPICallbackcanRespondToDoAction) // NOLINT
    {
        Common::PluginProtocol::DataMessage dataMessage = createDefaultMessage(Common::PluginProtocol::Commands::REQUEST_PLUGIN_DO_ACTION, "contentOfAction");
        Common::PluginProtocol::DataMessage expectedAnswer(dataMessage);

        expectedAnswer.Payload.clear();
        expectedAnswer.Payload.emplace_back("ACK");

        EXPECT_CALL(mock(), queueAction(_));

        auto reply = managementRequest.triggerRequest(context(), dataMessage);

        EXPECT_PRED_FORMAT2( dataMessageSimilar, expectedAnswer, reply );
    }


    TEST_F(PluginApiCallbackTests, pluginAPICallbackcanRespondToDoActionFail) // NOLINT
    {
        Common::PluginProtocol::DataMessage dataMessage = createDefaultMessage(Common::PluginProtocol::Commands::REQUEST_PLUGIN_DO_ACTION, "");
        Common::PluginProtocol::DataMessage expectedAnswer(dataMessage);

        std::string actionData = "ActionData";
        expectedAnswer.Payload.clear();
        expectedAnswer.Payload.emplace_back(actionData);

        dataMessage.Payload.clear();
        dataMessage.ProtocolVersion = "invalid";
        auto reply = managementRequest.triggerRequest(context(), dataMessage);
        EXPECT_EQ(reply.Error, "Protocol not supported");
    }

    TEST_F(PluginApiCallbackTests, pluginAPICallbackcanRespondToApplyNewPolicy) // NOLINT
    {
        Common::PluginProtocol::DataMessage dataMessage = createDefaultMessage(Common::PluginProtocol::Commands::REQUEST_PLUGIN_APPLY_POLICY, "contentOfPolicy");
        Common::PluginProtocol::DataMessage expectedAnswer(dataMessage);

        expectedAnswer.Payload.clear();
        expectedAnswer.Payload.emplace_back("ACK");

        EXPECT_CALL(mock(), applyNewPolicy(_));

        auto reply = managementRequest.triggerRequest(context(), dataMessage);

        EXPECT_PRED_FORMAT2( dataMessageSimilar, expectedAnswer, reply );
    }

    TEST_F(PluginApiCallbackTests, pluginAPICallbackcanRespondToApplyNewPolicyFail) // NOLINT
    {
        Common::PluginProtocol::DataMessage dataMessage = createDefaultMessage(Common::PluginProtocol::Commands::REQUEST_PLUGIN_APPLY_POLICY, "");
        Common::PluginProtocol::DataMessage expectedAnswer(dataMessage);

        std::string policyData = "PolicyData";
        expectedAnswer.Payload.clear();
        expectedAnswer.Payload.emplace_back(policyData);

        dataMessage.Payload.clear();
        dataMessage.ProtocolVersion = "invalid";
        auto reply = managementRequest.triggerRequest(context(), dataMessage);
        EXPECT_EQ(reply.Error, "Protocol not supported");
    }

}
