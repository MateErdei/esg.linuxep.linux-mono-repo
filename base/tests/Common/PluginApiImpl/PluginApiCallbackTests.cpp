/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "MockedPluginApiCallback.h"
#include "SingleManagementRequest.h"
#include "TestCompare.h"

#include <Common/FileSystemImpl/FilePermissionsImpl.h>
#include <Common/FileSystemImpl/FileSystemImpl.h>
#include <Common/Logging/ConsoleLoggingSetup.h>
#include <Common/PluginApi/ApiException.h>
#include <Common/PluginApi/IBaseServiceApi.h>
#include <Common/PluginApiImpl/PluginResourceManagement.h>
#include <Common/PluginProtocol/MessageBuilder.h>
#include <Common/ZeroMQWrapper/ISocketReplier.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <tests/Common/ApplicationConfiguration/MockedApplicationPathManager.h>
#include <tests/Common/Helpers/FilePermissionsReplaceAndRestore.h>
#include <tests/Common/Helpers/FileSystemReplaceAndRestore.h>
#include <tests/Common/Helpers/MockFilePermissions.h>
#include <tests/Common/Helpers/MockFileSystem.h>

#include <thread>

namespace
{
    void handleRegistration(const Common::ZMQWrapperApi::IContextSharedPtr& context)
    {
        auto replier = context->getReplier();
        std::string address =
            Common::ApplicationConfiguration::applicationPathManager().getManagementAgentSocketAddress();
        replier->listen(address);

        // handle registration
        Common::PluginProtocol::Protocol protocol;
        auto request = protocol.deserialize(replier->read());
        assert(request.m_command == Common::PluginProtocol::Commands::PLUGIN_SEND_REGISTER);
        Common::PluginProtocol::MessageBuilder messageBuilder("plugin");
        auto replyMessage = protocol.serialize(messageBuilder.replyAckMessage(request));
        replier->write(replyMessage);
    }

    using data_t = Common::ZeroMQWrapper::IReadable::data_t;
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
            MockedApplicationPathManager* mockAppManager = new NiceMock<MockedApplicationPathManager>();
            MockedApplicationPathManager& mock(*mockAppManager);
            ON_CALL(mock, getManagementAgentSocketAddress()).WillByDefault(Return("inproc://management.ipc"));
            ON_CALL(mock, getPluginSocketAddress(_)).WillByDefault(Return("inproc://plugin.ipc"));
            Common::ApplicationConfiguration::replaceApplicationPathManager(
                std::unique_ptr<Common::ApplicationConfiguration::IApplicationPathManager>(mockAppManager));
            mockPluginCallback = std::make_shared<NiceMock<MockedPluginApiCallback>>();

            std::thread registration(handleRegistration, pluginResourceManagement.getSocketContext());

            //auto mockFileSystem = new StrictMock<MockFileSystem>();
            //m_replacer.replace(std::unique_ptr<Common::FileSystem::IFileSystem>(mockFileSystem));

            auto mockFilePermissions = new StrictMock<MockFilePermissions>();
            std::unique_ptr<MockFilePermissions> mockIFilePermissionsPtr =
                std::unique_ptr<MockFilePermissions>(mockFilePermissions);
            Tests::replaceFilePermissions(std::move(mockIFilePermissionsPtr));

            EXPECT_CALL(*mockFilePermissions, getUserId(_)).WillRepeatedly(Return(1));
            EXPECT_CALL(*mockFilePermissions, chmod(_, _)).WillRepeatedly(Return());
            EXPECT_CALL(*mockFilePermissions, chown(_, _, _)).WillRepeatedly(Return());

            plugin = pluginResourceManagement.createPluginAPI("plugin", mockPluginCallback);
            registration.join();
        }

        void TearDown() override
        {
            Common::ApplicationConfiguration::restoreApplicationPathManager();
            plugin.reset();
        }

        Common::PluginProtocol::DataMessage createDefaultMessage(
            Common::PluginProtocol::Commands command,
            const std::string& firstPayloadItem)
        {
            Common::PluginProtocol::DataMessage dataMessage;
            dataMessage.m_command = command;
            dataMessage.m_applicationId = defaultAppId;
            dataMessage.m_pluginName = defaultPluginName;
            if (!firstPayloadItem.empty())
            {
                dataMessage.m_payload.push_back(firstPayloadItem);
            }

            return dataMessage;
        }

        Common::ZMQWrapperApi::IContextSharedPtr context() { return pluginResourceManagement.getSocketContext(); }

        MockedPluginApiCallback& mock()
        {
            MockedPluginApiCallback* mockPtr = mockPluginCallback.get();
            return *mockPtr;
        }

        Common::Logging::ConsoleLoggingSetup m_consoleLogging;
        std::string defaultAppId;
        std::string defaultPluginName;
        PluginResourceManagement pluginResourceManagement;
        SingleManagementRequest managementRequest;

        std::shared_ptr<MockedPluginApiCallback> mockPluginCallback;
        std::unique_ptr<Common::PluginApi::IBaseServiceApi> plugin;
        Tests::ScopedReplaceFileSystem m_replacer; 
    };

    // cppcheck-suppress syntaxError
    TEST_F(PluginApiCallbackTests, pluginAPICallbackcanRespondToStatus) // NOLINT
    {
        Common::PluginProtocol::DataMessage dataMessage =
            createDefaultMessage(Common::PluginProtocol::Commands::REQUEST_PLUGIN_STATUS, "");
        Common::PluginProtocol::DataMessage expectedAnswer(dataMessage);
        Common::PluginApi::StatusInfo statusInfo{ "statusContent", "statusNoTimestamp", "" };
        expectedAnswer.m_payload.clear();
        expectedAnswer.m_payload.push_back(statusInfo.statusXml);
        expectedAnswer.m_payload.push_back(statusInfo.statusWithoutTimestampsXml);

        EXPECT_CALL(mock(), getStatus(defaultAppId)).WillOnce(Return(statusInfo));

        auto reply = managementRequest.triggerRequest(context(), dataMessage);

        EXPECT_PRED_FORMAT2(dataMessageSimilar, expectedAnswer, reply);
    }

    TEST_F(PluginApiCallbackTests, pluginAPICallbackcanRespondToMessageFail) // NOLINT
    {
        Common::PluginProtocol::DataMessage dataMessage =
            createDefaultMessage(static_cast<Common::PluginProtocol::Commands>(20), "");
        dataMessage.m_payload.clear();
        auto reply = managementRequest.triggerRequest(context(), dataMessage);
        EXPECT_EQ(reply.m_error, "Invalid request");
    }

    TEST_F(PluginApiCallbackTests, pluginAPICallbackcanRespondToTelemetry) // NOLINT
    {
        Common::PluginProtocol::DataMessage dataMessage =
            createDefaultMessage(Common::PluginProtocol::Commands::REQUEST_PLUGIN_TELEMETRY, "");
        Common::PluginProtocol::DataMessage expectedAnswer(dataMessage);

        std::string telemetryData = "TelemetryData";
        expectedAnswer.m_payload.clear();
        expectedAnswer.m_payload.push_back(telemetryData);

        EXPECT_CALL(mock(), getTelemetry()).WillOnce(Return(telemetryData));

        auto reply = managementRequest.triggerRequest(context(), dataMessage);

        EXPECT_PRED_FORMAT2(dataMessageSimilar, expectedAnswer, reply);
    }

    TEST_F(PluginApiCallbackTests, pluginAPICallbackcanRespondToDoAction) // NOLINT
    {
        auto mockFileSystem = new StrictMock<MockFileSystem>();
        m_replacer.replace(std::unique_ptr<Common::FileSystem::IFileSystem>(mockFileSystem));
        EXPECT_CALL(*mockFileSystem, readFile("contentOfAction.xml")).WillRepeatedly(Return("contentOfAction"));
        Common::PluginProtocol::DataMessage dataMessage =
            createDefaultMessage(Common::PluginProtocol::Commands::REQUEST_PLUGIN_DO_ACTION, "contentOfAction.xml");
        Common::PluginProtocol::DataMessage expectedAnswer(dataMessage);

        expectedAnswer.m_payload.clear();
        expectedAnswer.m_acknowledge = true;

        EXPECT_CALL(mock(), queueAction(_));

        auto reply = managementRequest.triggerRequest(context(), dataMessage);

        EXPECT_PRED_FORMAT2(dataMessageSimilar, expectedAnswer, reply);

    }

    TEST_F(PluginApiCallbackTests, pluginAPICallbackcanRespondToApplyNewPolicyWithAppId) // NOLINT
    {
        auto mockFileSystem = new StrictMock<MockFileSystem>();
        m_replacer.replace(std::unique_ptr<Common::FileSystem::IFileSystem>(mockFileSystem));
        EXPECT_CALL(*mockFileSystem, readFile("contentOfPolicy.xml")).WillRepeatedly(Return("contentOfPolicy"));
        Common::PluginProtocol::DataMessage dataMessage =
                createDefaultMessage(Common::PluginProtocol::Commands::REQUEST_PLUGIN_APPLY_POLICY, "contentOfPolicy.xml");
        Common::PluginProtocol::DataMessage expectedAnswer(dataMessage);

        expectedAnswer.m_payload.clear();
        expectedAnswer.m_acknowledge = true;

        EXPECT_CALL(mock(), applyNewPolicyWithAppId(_, _));

        auto reply = managementRequest.triggerRequest(context(), dataMessage);

        EXPECT_PRED_FORMAT2(dataMessageSimilar, expectedAnswer, reply);
    }
} // namespace
