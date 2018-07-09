
#include <ManagementAgent/PluginCommunicationImpl/PluginManager.h>
#include "Common/ZeroMQWrapper/IContext.h"
#include "Common/ZeroMQWrapper/ISocketRequester.h"
#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "MockPluginServerCallback.h"
#include "../../Common/PluginApiImpl/MockedApplicationPathManager.h"
#include "Common/PluginApi/ApiException.h"

class TestPluginServerCallbackHandler : public ::testing::Test
{
public:
    TestPluginServerCallbackHandler()
    {
        m_mockServerCallback = std::make_shared<StrictMock<MockPluginServerCallback>>();
        MockedApplicationPathManager *mockApplicationPathManager =  new NiceMock<MockedApplicationPathManager>();
        Common::ApplicationConfiguration::replaceApplicationPathManager(
                std::unique_ptr<Common::ApplicationConfiguration::IApplicationPathManager>(mockApplicationPathManager));
        ON_CALL(*mockApplicationPathManager,  getManagementAgentSocketAddress()).WillByDefault(Return("inproc:///tmp/management.ipc"));
        m_context = Common::ZeroMQWrapper::createContext();
        auto replier = m_context->getReplier();
        m_requester = m_context->getRequester();
        replier->listen("inproc:///tmp/management.ipc");
        m_requester->connect("inproc:///tmp/management.ipc");
        m_PluginManagerPtr = std::unique_ptr<ManagementAgent::PluginCommunicationImpl::PluginManager>(new ManagementAgent::PluginCommunicationImpl::PluginManager());
        m_PluginManagerPtr->setServerCallback(m_mockServerCallback, std::move(replier));
    }

    ~TestPluginServerCallbackHandler()
    {}

    Common::ZeroMQWrapper::IContextPtr m_context;
    std::unique_ptr<ManagementAgent::PluginCommunicationImpl::PluginManager> m_PluginManagerPtr;
    std::shared_ptr<MockPluginServerCallback> m_mockServerCallback;

    Common::ZeroMQWrapper::ISocketRequesterPtr m_requester;
    Common::PluginProtocol::Protocol m_Protocol;

    Common::PluginProtocol::DataMessage createDefaultMessage(Common::PluginProtocol::Commands command, const std::string &firstPayloadItem )
    {
        Common::PluginProtocol::DataMessage dataMessage;
        dataMessage.Command = command;
        dataMessage.ProtocolVersion = Common::PluginProtocol::ProtocolSerializerFactory::ProtocolVersion;
        dataMessage.ApplicationId = "plugin";
        dataMessage.MessageId = "1";
        if ( !firstPayloadItem.empty())
        {
            dataMessage.Payload.push_back(firstPayloadItem);
        }

        return dataMessage;
    }
};

TEST_F(TestPluginServerCallbackHandler, TestServerCallbackReturnsAcknowledgementOnPluginSendEvent)
{
    Common::PluginProtocol::DataMessage eventMessage = createDefaultMessage(Common::PluginProtocol::Commands::PLUGIN_SEND_EVENT, std::string("Event"));
    auto rawMessage = m_Protocol.serialize(eventMessage);
    EXPECT_CALL(*m_mockServerCallback, receivedSendEvent(eventMessage.ApplicationId, eventMessage.Payload[0])).WillOnce(Return());
    m_requester->write(rawMessage);
    Common::PluginProtocol::DataMessage ackMessage = createDefaultMessage(Common::PluginProtocol::Commands::PLUGIN_SEND_EVENT, std::string("ACK"));
    auto expectedRawReply =  m_Protocol.serialize(ackMessage);
    auto rawReply = m_requester->read();
    ASSERT_EQ(expectedRawReply, rawReply);
}

TEST_F(TestPluginServerCallbackHandler, TestServerCallbackReturnsAcknowledgementOnPluginChangeStatus)
{
    Common::PluginApi::StatusInfo statusInfo;
    statusInfo.statusWithoutXml = "withoutXML";
    statusInfo.statusXml = "withXML";
    Common::PluginProtocol::DataMessage statusMessage = createDefaultMessage(Common::PluginProtocol::Commands::PLUGIN_SEND_STATUS, statusInfo.statusWithoutXml);
    statusMessage.Payload.push_back(statusInfo.statusXml);
    auto rawMessage = m_Protocol.serialize(statusMessage);
    EXPECT_CALL(*m_mockServerCallback, receivedChangeStatus(statusMessage.ApplicationId, A<const Common::PluginApi::StatusInfo &>())).WillOnce(Return());
    m_requester->write(rawMessage);
    Common::PluginProtocol::DataMessage ackMessage = createDefaultMessage(Common::PluginProtocol::Commands::PLUGIN_SEND_STATUS, std::string("ACK"));
    auto expectedRawReply =  m_Protocol.serialize(ackMessage);
    auto rawReply = m_requester->read();
    ASSERT_EQ(expectedRawReply, rawReply);
}

TEST_F(TestPluginServerCallbackHandler, TestServerCallbackReturnsPolicyOnPluginQueryPolicy)
{
    Common::PluginProtocol::DataMessage queryPolicyMessage = createDefaultMessage(Common::PluginProtocol::Commands::PLUGIN_QUERY_CURRENT_POLICY, "");
    auto rawMessage = m_Protocol.serialize(queryPolicyMessage);
    EXPECT_CALL(*m_mockServerCallback, receivedGetPolicy(queryPolicyMessage.ApplicationId)).WillOnce(Return(std::string("policy")));
    m_requester->write(rawMessage);
    Common::PluginProtocol::DataMessage ackMessage = createDefaultMessage(Common::PluginProtocol::Commands::PLUGIN_QUERY_CURRENT_POLICY, std::string("policy"));
    auto expectedRawReply =  m_Protocol.serialize(ackMessage);
    auto rawReply = m_requester->read();
    ASSERT_EQ(expectedRawReply, rawReply);
}

TEST_F(TestPluginServerCallbackHandler, TestServerCallbackReturnsAcknowledgementOnPluginRegistration)
{
    Common::PluginProtocol::DataMessage registerMessage = createDefaultMessage(Common::PluginProtocol::Commands::PLUGIN_SEND_REGISTER, "");
    auto rawMessage = m_Protocol.serialize(registerMessage);
    EXPECT_CALL(*m_mockServerCallback, receivedRegisterWithManagementAgent(registerMessage.ApplicationId)).WillOnce(Return());
    m_requester->write(rawMessage);
    Common::PluginProtocol::DataMessage ackMessage = createDefaultMessage(Common::PluginProtocol::Commands::PLUGIN_SEND_REGISTER, "ACK");
    auto expectedRawReply =  m_Protocol.serialize(ackMessage);
    auto rawReply = m_requester->read();
    ASSERT_EQ(expectedRawReply, rawReply);
}

TEST_F(TestPluginServerCallbackHandler, TestServerCallbackReturnsErrorOnBadCommand)
{
    // Plugin should not send a 'request plugin status' message
    Common::PluginProtocol::DataMessage badMessage = createDefaultMessage(Common::PluginProtocol::Commands::REQUEST_PLUGIN_STATUS, "");
    auto rawMessage = m_Protocol.serialize(badMessage);
    m_requester->write(rawMessage);
    Common::PluginProtocol::DataMessage errorMessage = createDefaultMessage(Common::PluginProtocol::Commands::REQUEST_PLUGIN_STATUS, "Error");
    errorMessage.Payload.emplace_back("Request not supported");
    auto expectedRawReply =  m_Protocol.serialize(errorMessage);
    auto rawReply = m_requester->read();
    ASSERT_EQ(expectedRawReply, rawReply);
}

TEST_F(TestPluginServerCallbackHandler, TestServerCallbackReturnsErrorDescriptionOnApiException)
{
    Common::PluginProtocol::DataMessage eventMessage = createDefaultMessage(Common::PluginProtocol::Commands::PLUGIN_SEND_EVENT, std::string("Event"));
    auto rawMessage = m_Protocol.serialize(eventMessage);
    EXPECT_CALL(*m_mockServerCallback, receivedSendEvent(eventMessage.ApplicationId, eventMessage.Payload[0])).WillOnce(Throw(Common::PluginApi::ApiException("Dummy Exception")));
    m_requester->write(rawMessage);
    Common::PluginProtocol::DataMessage errorMessage = createDefaultMessage(Common::PluginProtocol::Commands::PLUGIN_SEND_EVENT, "Error");
    errorMessage.Payload.emplace_back("Dummy Exception");
    auto expectedRawReply =  m_Protocol.serialize(errorMessage);
    auto rawReply = m_requester->read();
    ASSERT_EQ(expectedRawReply, rawReply);
}

TEST_F(TestPluginServerCallbackHandler, TestServerCallbackReturnsErrorDescriptionOnStdException)
{
    Common::PluginProtocol::DataMessage eventMessage = createDefaultMessage(Common::PluginProtocol::Commands::PLUGIN_SEND_EVENT, std::string("Event"));
    auto rawMessage = m_Protocol.serialize(eventMessage);
    auto except = std::exception();
    EXPECT_CALL(*m_mockServerCallback, receivedSendEvent(eventMessage.ApplicationId, eventMessage.Payload[0])).WillOnce(Throw(except));
    m_requester->write(rawMessage);
    Common::PluginProtocol::DataMessage errorMessage = createDefaultMessage(Common::PluginProtocol::Commands::PLUGIN_SEND_EVENT, "Error");
    errorMessage.Payload.emplace_back(except.what());
    auto expectedRawReply =  m_Protocol.serialize(errorMessage);
    auto rawReply = m_requester->read();
    ASSERT_EQ(expectedRawReply, rawReply);
}