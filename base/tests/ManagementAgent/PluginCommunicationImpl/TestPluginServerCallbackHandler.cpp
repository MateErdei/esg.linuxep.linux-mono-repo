/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "MockPluginServerCallback.h"

#include <Common/Logging/ConsoleLoggingSetup.h>
#include <Common/PluginApi/ApiException.h>
#include <Common/ZMQWrapperApi/IContext.h>
#include <Common/ZMQWrapperApi/IContextSharedPtr.h>
#include <Common/ZeroMQWrapper/ISocketRequester.h>
#include <ManagementAgent/PluginCommunicationImpl/PluginManager.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <tests/Common/ApplicationConfiguration/MockedApplicationPathManager.h>
#include <tests/Common/PluginApiImpl/TestCompare.h>

namespace {
    constexpr int defaultTimeoutMs = 2000;
}

class TestPluginServerCallbackHandler : public TestCompare
{
public:
    TestPluginServerCallbackHandler()
    {
        m_mockServerCallback = std::make_shared<StrictMock<MockPluginServerCallback>>();
        MockedApplicationPathManager* mockApplicationPathManager = new NiceMock<MockedApplicationPathManager>();
        Common::ApplicationConfiguration::replaceApplicationPathManager(
            std::unique_ptr<Common::ApplicationConfiguration::IApplicationPathManager>(mockApplicationPathManager));
        ON_CALL(*mockApplicationPathManager, getManagementAgentSocketAddress())
            .WillByDefault(Return("inproc:///tmp/management.ipc"));
        ON_CALL(*mockApplicationPathManager, getPublisherDataChannelAddress())
            .WillByDefault(Return("inproc:///tmp/pubchannel.ipc"));
        ON_CALL(*mockApplicationPathManager, getSubscriberDataChannelAddress())
            .WillByDefault(Return("inproc:///tmp/subchannel.ipc"));
        m_context = Common::ZMQWrapperApi::createContext();
        auto replier = m_context->getReplier();
        m_requester = m_context->getRequester();
        m_requester->setTimeout(defaultTimeoutMs);
        m_requester->setConnectionTimeout(defaultTimeoutMs);
        replier->setTimeout(defaultTimeoutMs);
        replier->setConnectionTimeout(defaultTimeoutMs);
        replier->listen("inproc:///tmp/management.ipc");
        m_requester->connect("inproc:///tmp/management.ipc");
        m_PluginManagerPtr = std::unique_ptr<ManagementAgent::PluginCommunicationImpl::PluginManager>(
            new ManagementAgent::PluginCommunicationImpl::PluginManager(
                Common::ApplicationConfiguration::applicationPathManager().getManagementAgentSocketAddress()));
        m_PluginManagerPtr->setServerCallback(m_mockServerCallback, std::move(replier));
    }

    ~TestPluginServerCallbackHandler() override = default;

    Common::ZMQWrapperApi::IContextSharedPtr m_context;
    Common::ZeroMQWrapper::ISocketRequesterPtr m_requester;
    std::shared_ptr<MockPluginServerCallback> m_mockServerCallback;
    std::unique_ptr<ManagementAgent::PluginCommunicationImpl::PluginManager> m_PluginManagerPtr;

    Common::PluginProtocol::Protocol m_Protocol;

    Common::PluginProtocol::DataMessage createDefaultMessage(
        Common::PluginProtocol::Commands command,
        const std::string& firstPayloadItem)
    {
        Common::PluginProtocol::DataMessage dataMessage;
        dataMessage.m_pluginName = "plugin";
        dataMessage.m_command = command;
        dataMessage.m_applicationId = "plugin";
        if (!firstPayloadItem.empty())
        {
            dataMessage.m_payload.push_back(firstPayloadItem);
        }

        return dataMessage;
    }

    Common::PluginProtocol::DataMessage createAcknowledgementMessage(Common::PluginProtocol::Commands command)
    {
        Common::PluginProtocol::DataMessage dataMessage;
        dataMessage.m_pluginName = "plugin";
        dataMessage.m_command = command;
        dataMessage.m_applicationId = "plugin";
        dataMessage.m_acknowledge = true;
        return dataMessage;
    }

    Common::PluginProtocol::DataMessage sendReceive(const Common::PluginProtocol::DataMessage& request) const
    {
        auto rawMessage = m_Protocol.serialize(request);
        m_requester->write(rawMessage);
        auto rawReply = m_requester->read();
        return m_Protocol.deserialize(rawReply);
    }

private:
    Common::Logging::ConsoleLoggingSetup m_loggingSetup;
};

TEST_F(TestPluginServerCallbackHandler, TestServerCallbackHandlerReturnsAcknowledgementOnPluginSendEvent) // NOLINT
{
    Common::PluginProtocol::DataMessage eventMessage =
        createDefaultMessage(Common::PluginProtocol::Commands::PLUGIN_SEND_EVENT, std::string("Event"));
    EXPECT_CALL(*m_mockServerCallback, receivedSendEvent(eventMessage.m_applicationId, eventMessage.m_payload[0]))
        .WillOnce(Return());
    Common::PluginProtocol::DataMessage ackMessage =
        createAcknowledgementMessage(Common::PluginProtocol::Commands::PLUGIN_SEND_EVENT);
    auto replyMessage = sendReceive(eventMessage);
    EXPECT_PRED_FORMAT2(dataMessageSimilar, ackMessage, replyMessage);
}

TEST_F(TestPluginServerCallbackHandler, TestServerCallbackHandlerPluginSendEventReturnsErrorOnApiException) // NOLINT
{
    Common::PluginProtocol::DataMessage eventMessage =
        createDefaultMessage(Common::PluginProtocol::Commands::PLUGIN_SEND_EVENT, std::string("Event"));
    EXPECT_CALL(*m_mockServerCallback, receivedSendEvent(eventMessage.m_applicationId, eventMessage.m_payload[0]))
        .WillOnce(Throw(Common::PluginApi::ApiException("Dummy Exception")));
    Common::PluginProtocol::DataMessage errorMessage =
        createDefaultMessage(Common::PluginProtocol::Commands::PLUGIN_SEND_EVENT, "");
    errorMessage.m_error = "Dummy Exception";

    auto replyMessage = sendReceive(eventMessage);
    EXPECT_PRED_FORMAT2(dataMessageSimilar, errorMessage, replyMessage);
}

TEST_F(TestPluginServerCallbackHandler, TestServerCallbackHandlerPluginSendEventReturnsErrorOnStdException) // NOLINT
{
    Common::PluginProtocol::DataMessage eventMessage =
        createDefaultMessage(Common::PluginProtocol::Commands::PLUGIN_SEND_EVENT, std::string("Event"));
    auto except = std::exception();
    EXPECT_CALL(*m_mockServerCallback, receivedSendEvent(eventMessage.m_applicationId, eventMessage.m_payload[0]))
        .WillOnce(Throw(except));
    Common::PluginProtocol::DataMessage errorMessage =
        createDefaultMessage(Common::PluginProtocol::Commands::PLUGIN_SEND_EVENT, "");
    errorMessage.m_error = except.what();
    auto replyMessage = sendReceive(eventMessage);
    EXPECT_PRED_FORMAT2(dataMessageSimilar, errorMessage, replyMessage);
}

TEST_F(TestPluginServerCallbackHandler, TestServerCallbackHandlerReturnsAcknowledgementOnPluginSendStatus) // NOLINT
{
    Common::PluginApi::StatusInfo statusInfo;
    statusInfo.statusWithoutTimestampsXml = "withoutXML";
    statusInfo.statusXml = "withXML";
    Common::PluginProtocol::DataMessage statusMessage = createDefaultMessage(
        Common::PluginProtocol::Commands::PLUGIN_SEND_STATUS, statusInfo.statusWithoutTimestampsXml);
    statusMessage.m_payload.push_back(statusInfo.statusXml);

    EXPECT_CALL(
        *m_mockServerCallback,
        receivedChangeStatus(statusMessage.m_applicationId, A<const Common::PluginApi::StatusInfo&>()))
        .WillOnce(Return());

    Common::PluginProtocol::DataMessage ackMessage =
        createAcknowledgementMessage(Common::PluginProtocol::Commands::PLUGIN_SEND_STATUS);
    auto replyMessage = sendReceive(statusMessage);
    EXPECT_PRED_FORMAT2(dataMessageSimilar, ackMessage, replyMessage);
}

TEST_F(TestPluginServerCallbackHandler, TestServerCallbackHandlerPluginSendStatusReturnsErrorOnApiException) // NOLINT
{
    Common::PluginApi::StatusInfo statusInfo;
    statusInfo.statusWithoutTimestampsXml = "withoutXML";
    statusInfo.statusXml = "withXML";
    Common::PluginProtocol::DataMessage statusMessage = createDefaultMessage(
        Common::PluginProtocol::Commands::PLUGIN_SEND_STATUS, statusInfo.statusWithoutTimestampsXml);
    statusMessage.m_payload.push_back(statusInfo.statusXml);

    EXPECT_CALL(
        *m_mockServerCallback,
        receivedChangeStatus(statusMessage.m_applicationId, A<const Common::PluginApi::StatusInfo&>()))
        .WillOnce(Throw(Common::PluginApi::ApiException("Dummy Exception")));
    Common::PluginProtocol::DataMessage errorMessage =
        createDefaultMessage(Common::PluginProtocol::Commands::PLUGIN_SEND_STATUS, "");
    errorMessage.m_error = "Dummy Exception";

    auto replyMessage = sendReceive(statusMessage);
    EXPECT_PRED_FORMAT2(dataMessageSimilar, errorMessage, replyMessage);
}

TEST_F(TestPluginServerCallbackHandler, TestServerCallbackHandlerPluginSendStatusReturnsErrorOnStdException) // NOLINT
{
    Common::PluginApi::StatusInfo statusInfo;
    statusInfo.statusWithoutTimestampsXml = "withoutXML";
    statusInfo.statusXml = "withXML";
    Common::PluginProtocol::DataMessage statusMessage = createDefaultMessage(
        Common::PluginProtocol::Commands::PLUGIN_SEND_STATUS, statusInfo.statusWithoutTimestampsXml);
    statusMessage.m_payload.push_back(statusInfo.statusXml);

    auto except = std::exception();
    EXPECT_CALL(
        *m_mockServerCallback,
        receivedChangeStatus(statusMessage.m_applicationId, A<const Common::PluginApi::StatusInfo&>()))
        .WillOnce(Throw(except));

    Common::PluginProtocol::DataMessage errorMessage =
        createDefaultMessage(Common::PluginProtocol::Commands::PLUGIN_SEND_STATUS, "");

    errorMessage.m_error = except.what();

    auto replyMessage = sendReceive(statusMessage);
    EXPECT_PRED_FORMAT2(dataMessageSimilar, errorMessage, replyMessage);
}

TEST_F(TestPluginServerCallbackHandler, TestServerCallbackHandlerReturnsPolicyOnPluginQueryPolicy) // NOLINT
{
    Common::PluginProtocol::DataMessage queryPolicyMessage =
        createDefaultMessage(Common::PluginProtocol::Commands::PLUGIN_QUERY_CURRENT_POLICY, "");
    auto rawMessage = m_Protocol.serialize(queryPolicyMessage);
    EXPECT_CALL(*m_mockServerCallback, receivedGetPolicyRequest(queryPolicyMessage.m_applicationId))
        .WillOnce(Return(true));

    auto replyMessage = sendReceive(queryPolicyMessage);

    EXPECT_EQ(replyMessage.m_acknowledge, true);
}

TEST_F(TestPluginServerCallbackHandler, TestServerCallbackHandlerPluginQueryPolicyReturnsErrorOnApiException) // NOLINT
{
    Common::PluginProtocol::DataMessage queryPolicyMessage =
        createDefaultMessage(Common::PluginProtocol::Commands::PLUGIN_QUERY_CURRENT_POLICY, "");

    EXPECT_CALL(*m_mockServerCallback, receivedGetPolicyRequest(queryPolicyMessage.m_applicationId))
        .WillOnce(Throw(Common::PluginApi::ApiException("Dummy Exception")));

    Common::PluginProtocol::DataMessage errorMessage =
        createDefaultMessage(Common::PluginProtocol::Commands::PLUGIN_QUERY_CURRENT_POLICY, "");
    errorMessage.m_error = "Dummy Exception";

    auto replyMessage = sendReceive(queryPolicyMessage);
    EXPECT_PRED_FORMAT2(dataMessageSimilar, errorMessage, replyMessage);
}

TEST_F(TestPluginServerCallbackHandler, TestServerCallbackHandlerPluginQueryPolicyReturnsErrorOnStdException) // NOLINT
{
    Common::PluginProtocol::DataMessage queryPolicyMessage =
        createDefaultMessage(Common::PluginProtocol::Commands::PLUGIN_QUERY_CURRENT_POLICY, "");

    auto except = std::exception();
    EXPECT_CALL(*m_mockServerCallback, receivedGetPolicyRequest(queryPolicyMessage.m_applicationId))
        .WillOnce(Throw(except));

    Common::PluginProtocol::DataMessage errorMessage =
        createDefaultMessage(Common::PluginProtocol::Commands::PLUGIN_QUERY_CURRENT_POLICY, "");

    errorMessage.m_error = except.what();
    auto replyMessage = sendReceive(queryPolicyMessage);
    EXPECT_PRED_FORMAT2(dataMessageSimilar, errorMessage, replyMessage);
}

TEST_F(TestPluginServerCallbackHandler, TestServerCallbackHandlerReturnsAcknowledgementOnPluginRegistration) // NOLINT
{
    Common::PluginProtocol::DataMessage registerMessage =
        createDefaultMessage(Common::PluginProtocol::Commands::PLUGIN_SEND_REGISTER, "");

    EXPECT_CALL(*m_mockServerCallback, receivedRegisterWithManagementAgent(registerMessage.m_pluginName))
        .WillOnce(Return());

    Common::PluginProtocol::DataMessage ackMessage =
        createAcknowledgementMessage(Common::PluginProtocol::Commands::PLUGIN_SEND_REGISTER);

    auto replyMessage = sendReceive(registerMessage);
    EXPECT_PRED_FORMAT2(dataMessageSimilar, ackMessage, replyMessage);
}

TEST_F(TestPluginServerCallbackHandler, TestServerCallbackHandlerPluginRegistrationReturnsErrorOnApiException) // NOLINT
{
    Common::PluginProtocol::DataMessage registerMessage =
        createDefaultMessage(Common::PluginProtocol::Commands::PLUGIN_SEND_REGISTER, "");

    EXPECT_CALL(*m_mockServerCallback, receivedRegisterWithManagementAgent(registerMessage.m_pluginName))
        .WillOnce(Throw(Common::PluginApi::ApiException("Dummy Exception")));

    Common::PluginProtocol::DataMessage errorMessage =
        createDefaultMessage(Common::PluginProtocol::Commands::PLUGIN_SEND_REGISTER, "");

    errorMessage.m_error = "Dummy Exception";
    auto replyMessage = sendReceive(registerMessage);
    EXPECT_PRED_FORMAT2(dataMessageSimilar, errorMessage, replyMessage);
}

TEST_F(TestPluginServerCallbackHandler, TestServerCallbackHandlerPluginRegistrationReturnsErrorOnStdException) // NOLINT
{
    Common::PluginProtocol::DataMessage registerMessage =
        createDefaultMessage(Common::PluginProtocol::Commands::PLUGIN_SEND_REGISTER, "");

    auto except = std::exception();
    EXPECT_CALL(*m_mockServerCallback, receivedRegisterWithManagementAgent(registerMessage.m_pluginName))
        .WillOnce(Throw(except));

    Common::PluginProtocol::DataMessage errorMessage =
        createDefaultMessage(Common::PluginProtocol::Commands::PLUGIN_SEND_REGISTER, "");

    errorMessage.m_error = except.what();
    auto replyMessage = sendReceive(registerMessage);
    EXPECT_PRED_FORMAT2(dataMessageSimilar, errorMessage, replyMessage);
}

TEST_F(TestPluginServerCallbackHandler, TestServerCallbackHandlerReturnsErrorOnBadCommand) // NOLINT
{
    // Plugin should not send a 'request plugin status' message
    Common::PluginProtocol::DataMessage badMessage =
        createDefaultMessage(Common::PluginProtocol::Commands::REQUEST_PLUGIN_STATUS, "");

    Common::PluginProtocol::DataMessage errorMessage =
        createDefaultMessage(Common::PluginProtocol::Commands::REQUEST_PLUGIN_STATUS, "");
    errorMessage.m_error = "Request not supported";
    auto replyMessage = sendReceive(badMessage);
    EXPECT_PRED_FORMAT2(dataMessageSimilar, errorMessage, replyMessage);
}

TEST_F(TestPluginServerCallbackHandler, TestThatMessageInWrongSerialisationDoesNotStopHandlersAbilityToRespond)
{
    data_t rawMessage{"NotProtobufMessage"};
    m_requester->write(rawMessage);
    auto rawReply = m_requester->read();
    EXPECT_EQ(rawReply[0], "INVALID");

    Common::PluginProtocol::DataMessage registerMessage =
            createDefaultMessage(Common::PluginProtocol::Commands::PLUGIN_SEND_REGISTER, "");

    EXPECT_CALL(*m_mockServerCallback, receivedRegisterWithManagementAgent(registerMessage.m_pluginName))
            .WillOnce(Return());

    Common::PluginProtocol::DataMessage ackMessage =
            createAcknowledgementMessage(Common::PluginProtocol::Commands::PLUGIN_SEND_REGISTER);

    auto replyMessage = sendReceive(registerMessage);
    EXPECT_PRED_FORMAT2(dataMessageSimilar, ackMessage, replyMessage);
}
