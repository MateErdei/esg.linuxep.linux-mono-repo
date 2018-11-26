
#include "MockPluginServerCallback.h"

#include <ManagementAgent/PluginCommunicationImpl/PluginManager.h>

#include <Common/PluginApi/ApiException.h>
#include <Common/ZeroMQWrapper/IContext.h>
#include <Common/ZeroMQWrapper/ISocketRequester.h>
#include <Common/ZeroMQWrapper/IContextSharedPtr.h>

#include <tests/Common/ApplicationConfiguration/MockedApplicationPathManager.h>
#include <tests/Common/PluginApiImpl/TestCompare.h>
#include <tests/Common/Logging/TestConsoleLoggingSetup.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

class TestPluginServerCallbackHandler : public TestCompare
{
public:
    TestPluginServerCallbackHandler()
    {
        m_mockServerCallback = std::make_shared<StrictMock<MockPluginServerCallback>>();
        MockedApplicationPathManager *mockApplicationPathManager = new NiceMock<MockedApplicationPathManager>();
        Common::ApplicationConfiguration::replaceApplicationPathManager(
                std::unique_ptr<Common::ApplicationConfiguration::IApplicationPathManager>(mockApplicationPathManager));
        ON_CALL(*mockApplicationPathManager, getManagementAgentSocketAddress()).WillByDefault(
                Return("inproc:///tmp/management.ipc"));
        m_context = Common::ZeroMQWrapper::createContext();
        auto replier = m_context->getReplier();
        m_requester = m_context->getRequester();
        replier->listen("inproc:///tmp/management.ipc");
        m_requester->connect("inproc:///tmp/management.ipc");
        m_PluginManagerPtr = std::unique_ptr<ManagementAgent::PluginCommunicationImpl::PluginManager>(
                new ManagementAgent::PluginCommunicationImpl::PluginManager());
        m_PluginManagerPtr->setServerCallback(m_mockServerCallback, std::move(replier));
    }

    ~TestPluginServerCallbackHandler() override = default;

    Common::ZeroMQWrapper::IContextSharedPtr m_context;
    Common::ZeroMQWrapper::ISocketRequesterPtr m_requester;
    std::shared_ptr<MockPluginServerCallback> m_mockServerCallback;
    std::unique_ptr<ManagementAgent::PluginCommunicationImpl::PluginManager> m_PluginManagerPtr;


    Common::PluginProtocol::Protocol m_Protocol;

    Common::PluginProtocol::DataMessage

    createDefaultMessage(Common::PluginProtocol::Commands command, const std::string &firstPayloadItem)
    {
        Common::PluginProtocol::DataMessage dataMessage;
        dataMessage.Command = command;
        dataMessage.ProtocolVersion = Common::PluginProtocol::ProtocolSerializerFactory::ProtocolVersion;
        dataMessage.ApplicationId = "plugin";
        dataMessage.MessageId = "1";
        if (!firstPayloadItem.empty())
        {
            dataMessage.Payload.push_back(firstPayloadItem);
        }

        return dataMessage;
    }

    Common::PluginProtocol::DataMessage sendReceive(const Common::PluginProtocol::DataMessage &request) const
    {
        auto rawMessage = m_Protocol.serialize(request);
        m_requester->write(rawMessage);
        auto rawReply = m_requester->read();
        return m_Protocol.deserialize(rawReply);

    }

private:
    TestLogging::TestConsoleLoggingSetup m_loggingSetup;
};

TEST_F(TestPluginServerCallbackHandler, TestServerCallbackHandlerReturnsAcknowledgementOnPluginSendEvent) //NOLINT
{
    Common::PluginProtocol::DataMessage eventMessage = createDefaultMessage(
            Common::PluginProtocol::Commands::PLUGIN_SEND_EVENT, std::string("Event"));
    EXPECT_CALL(*m_mockServerCallback, receivedSendEvent(eventMessage.ApplicationId, eventMessage.Payload[0])).WillOnce(
            Return());
    Common::PluginProtocol::DataMessage ackMessage = createDefaultMessage(
            Common::PluginProtocol::Commands::PLUGIN_SEND_EVENT, std::string("ACK"));
    auto replyMessage = sendReceive(eventMessage);
    EXPECT_PRED_FORMAT2(dataMessageSimilar, ackMessage, replyMessage);
}

TEST_F(TestPluginServerCallbackHandler, TestServerCallbackHandlerPluginSendEventReturnsErrorOnApiException) //NOLINT
{
    Common::PluginProtocol::DataMessage eventMessage = createDefaultMessage(
            Common::PluginProtocol::Commands::PLUGIN_SEND_EVENT, std::string("Event"));
    EXPECT_CALL(*m_mockServerCallback, receivedSendEvent(eventMessage.ApplicationId, eventMessage.Payload[0])).WillOnce(
            Throw(Common::PluginApi::ApiException("Dummy Exception")));
    Common::PluginProtocol::DataMessage errorMessage = createDefaultMessage(
            Common::PluginProtocol::Commands::PLUGIN_SEND_EVENT, ""
    );
    errorMessage.Error = "Dummy Exception";

    auto replyMessage = sendReceive(eventMessage);
    EXPECT_PRED_FORMAT2(dataMessageSimilar, errorMessage, replyMessage);

}

TEST_F(TestPluginServerCallbackHandler, TestServerCallbackHandlerPluginSendEventReturnsErrorOnStdException) //NOLINT
{
    Common::PluginProtocol::DataMessage eventMessage = createDefaultMessage(
            Common::PluginProtocol::Commands::PLUGIN_SEND_EVENT, std::string("Event"));
    auto except = std::exception();
    EXPECT_CALL(*m_mockServerCallback, receivedSendEvent(eventMessage.ApplicationId, eventMessage.Payload[0])).WillOnce(
            Throw(except));
    Common::PluginProtocol::DataMessage errorMessage = createDefaultMessage(
            Common::PluginProtocol::Commands::PLUGIN_SEND_EVENT, ""
    );
    errorMessage.Error = except.what();
    auto replyMessage = sendReceive(eventMessage);
    EXPECT_PRED_FORMAT2(dataMessageSimilar, errorMessage, replyMessage);

}

TEST_F(TestPluginServerCallbackHandler, TestServerCallbackHandlerReturnsAcknowledgementOnPluginSendStatus) //NOLINT
{
    Common::PluginApi::StatusInfo statusInfo;
    statusInfo.statusWithoutTimestampsXml = "withoutXML";
    statusInfo.statusXml = "withXML";
    Common::PluginProtocol::DataMessage statusMessage = createDefaultMessage(
            Common::PluginProtocol::Commands::PLUGIN_SEND_STATUS, statusInfo.statusWithoutTimestampsXml
    );
    statusMessage.Payload.push_back(statusInfo.statusXml);

    EXPECT_CALL(*m_mockServerCallback,
                receivedChangeStatus(statusMessage.ApplicationId, A<const Common::PluginApi::StatusInfo &>())).WillOnce(
            Return());

    Common::PluginProtocol::DataMessage ackMessage = createDefaultMessage(
            Common::PluginProtocol::Commands::PLUGIN_SEND_STATUS, std::string("ACK"));
    auto replyMessage = sendReceive(statusMessage);
    EXPECT_PRED_FORMAT2(dataMessageSimilar, ackMessage, replyMessage);
}

TEST_F(TestPluginServerCallbackHandler, TestServerCallbackHandlerPluginSendStatusReturnsErrorOnApiException) //NOLINT
{
    Common::PluginApi::StatusInfo statusInfo;
    statusInfo.statusWithoutTimestampsXml = "withoutXML";
    statusInfo.statusXml = "withXML";
    Common::PluginProtocol::DataMessage statusMessage = createDefaultMessage(
            Common::PluginProtocol::Commands::PLUGIN_SEND_STATUS, statusInfo.statusWithoutTimestampsXml
    );
    statusMessage.Payload.push_back(statusInfo.statusXml);

    EXPECT_CALL(*m_mockServerCallback,
                receivedChangeStatus(statusMessage.ApplicationId, A<const Common::PluginApi::StatusInfo &>())).WillOnce(
            Throw(Common::PluginApi::ApiException("Dummy Exception")));
    Common::PluginProtocol::DataMessage errorMessage = createDefaultMessage(
            Common::PluginProtocol::Commands::PLUGIN_SEND_STATUS, ""
    );
    errorMessage.Error = "Dummy Exception";

    auto replyMessage = sendReceive(statusMessage);
    EXPECT_PRED_FORMAT2(dataMessageSimilar, errorMessage, replyMessage);
}

TEST_F(TestPluginServerCallbackHandler, TestServerCallbackHandlerPluginSendStatusReturnsErrorOnStdException) //NOLINT
{
    Common::PluginApi::StatusInfo statusInfo;
    statusInfo.statusWithoutTimestampsXml = "withoutXML";
    statusInfo.statusXml = "withXML";
    Common::PluginProtocol::DataMessage statusMessage = createDefaultMessage(
            Common::PluginProtocol::Commands::PLUGIN_SEND_STATUS, statusInfo.statusWithoutTimestampsXml
    );
    statusMessage.Payload.push_back(statusInfo.statusXml);

    auto except = std::exception();
    EXPECT_CALL(*m_mockServerCallback,
                receivedChangeStatus(statusMessage.ApplicationId, A<const Common::PluginApi::StatusInfo &>())).WillOnce(
            Throw(except));

    Common::PluginProtocol::DataMessage errorMessage = createDefaultMessage(
            Common::PluginProtocol::Commands::PLUGIN_SEND_STATUS, ""
    );

    errorMessage.Error = except.what();

    auto replyMessage = sendReceive(statusMessage);
    EXPECT_PRED_FORMAT2(dataMessageSimilar, errorMessage, replyMessage);

}

TEST_F(TestPluginServerCallbackHandler, TestServerCallbackHandlerReturnsPolicyOnPluginQueryPolicy) //NOLINT
{
    Common::PluginProtocol::DataMessage queryPolicyMessage = createDefaultMessage(
            Common::PluginProtocol::Commands::PLUGIN_QUERY_CURRENT_POLICY, ""
    );
    auto rawMessage = m_Protocol.serialize(queryPolicyMessage);
    EXPECT_CALL(*m_mockServerCallback, receivedGetPolicyRequest(queryPolicyMessage.ApplicationId)).WillOnce(
            Return(true));

    auto replyMessage = sendReceive(queryPolicyMessage);

    std::vector<std::string> expectedPayload = {"ACK"};

    EXPECT_EQ(expectedPayload, replyMessage.Payload);
}

TEST_F(TestPluginServerCallbackHandler, TestServerCallbackHandlerPluginQueryPolicyReturnsErrorOnApiException) //NOLINT
{
    Common::PluginProtocol::DataMessage queryPolicyMessage = createDefaultMessage(
            Common::PluginProtocol::Commands::PLUGIN_QUERY_CURRENT_POLICY, ""
    );

    EXPECT_CALL(*m_mockServerCallback, receivedGetPolicyRequest(queryPolicyMessage.ApplicationId)).WillOnce(
            Throw(Common::PluginApi::ApiException("Dummy Exception")));

    Common::PluginProtocol::DataMessage errorMessage = createDefaultMessage(
            Common::PluginProtocol::Commands::PLUGIN_QUERY_CURRENT_POLICY, ""
    );
    errorMessage.Error = "Dummy Exception";

    auto replyMessage = sendReceive(queryPolicyMessage);
    EXPECT_PRED_FORMAT2(dataMessageSimilar, errorMessage, replyMessage);
}

TEST_F(TestPluginServerCallbackHandler, TestServerCallbackHandlerPluginQueryPolicyReturnsErrorOnStdException) //NOLINT
{
    Common::PluginProtocol::DataMessage queryPolicyMessage = createDefaultMessage(
            Common::PluginProtocol::Commands::PLUGIN_QUERY_CURRENT_POLICY, ""
    );

    auto except = std::exception();
    EXPECT_CALL(*m_mockServerCallback, receivedGetPolicyRequest(queryPolicyMessage.ApplicationId)).WillOnce(
            Throw(except));

    Common::PluginProtocol::DataMessage errorMessage = createDefaultMessage(
            Common::PluginProtocol::Commands::PLUGIN_QUERY_CURRENT_POLICY, ""
    );

    errorMessage.Error = except.what();
    auto replyMessage = sendReceive(queryPolicyMessage);
    EXPECT_PRED_FORMAT2(dataMessageSimilar, errorMessage, replyMessage);
}

TEST_F(TestPluginServerCallbackHandler, TestServerCallbackHandlerReturnsAcknowledgementOnPluginRegistration) //NOLINT
{
    Common::PluginProtocol::DataMessage registerMessage = createDefaultMessage(
            Common::PluginProtocol::Commands::PLUGIN_SEND_REGISTER, ""
    );

    EXPECT_CALL(*m_mockServerCallback, receivedRegisterWithManagementAgent(registerMessage.PluginName)).WillOnce(
            Return());

    Common::PluginProtocol::DataMessage ackMessage = createDefaultMessage(
            Common::PluginProtocol::Commands::PLUGIN_SEND_REGISTER, "ACK"
    );

    auto replyMessage = sendReceive(registerMessage);
    EXPECT_PRED_FORMAT2(dataMessageSimilar, ackMessage, replyMessage);
}

TEST_F(TestPluginServerCallbackHandler, TestServerCallbackHandlerPluginRegistrationReturnsErrorOnApiException) //NOLINT
{
    Common::PluginProtocol::DataMessage registerMessage = createDefaultMessage(
            Common::PluginProtocol::Commands::PLUGIN_SEND_REGISTER, ""
    );

    EXPECT_CALL(*m_mockServerCallback, receivedRegisterWithManagementAgent(registerMessage.PluginName)).WillOnce(
            Throw(Common::PluginApi::ApiException("Dummy Exception")));

    Common::PluginProtocol::DataMessage errorMessage = createDefaultMessage(
            Common::PluginProtocol::Commands::PLUGIN_SEND_REGISTER, ""
    );

    errorMessage.Error = "Dummy Exception";
    auto replyMessage = sendReceive(registerMessage);
    EXPECT_PRED_FORMAT2(dataMessageSimilar, errorMessage, replyMessage);
}

TEST_F(TestPluginServerCallbackHandler, TestServerCallbackHandlerPluginRegistrationReturnsErrorOnStdException) //NOLINT
{
    Common::PluginProtocol::DataMessage registerMessage = createDefaultMessage(
            Common::PluginProtocol::Commands::PLUGIN_SEND_REGISTER, ""
    );

    auto except = std::exception();
    EXPECT_CALL(*m_mockServerCallback, receivedRegisterWithManagementAgent(registerMessage.PluginName)).WillOnce(
            Throw(except));

    Common::PluginProtocol::DataMessage errorMessage = createDefaultMessage(
            Common::PluginProtocol::Commands::PLUGIN_SEND_REGISTER, ""
    );

    errorMessage.Error = except.what();
    auto replyMessage = sendReceive(registerMessage);
    EXPECT_PRED_FORMAT2(dataMessageSimilar, errorMessage, replyMessage);
}

TEST_F(TestPluginServerCallbackHandler, TestServerCallbackHandlerReturnsErrorOnBadCommand) //NOLINT
{
    // Plugin should not send a 'request plugin status' message
    Common::PluginProtocol::DataMessage badMessage = createDefaultMessage(
            Common::PluginProtocol::Commands::REQUEST_PLUGIN_STATUS, ""
    );

    Common::PluginProtocol::DataMessage errorMessage = createDefaultMessage(
            Common::PluginProtocol::Commands::REQUEST_PLUGIN_STATUS, ""
    );
    errorMessage.Error = "Request not supported";
    auto replyMessage = sendReceive(badMessage);
    EXPECT_PRED_FORMAT2(dataMessageSimilar, errorMessage, replyMessage);
}