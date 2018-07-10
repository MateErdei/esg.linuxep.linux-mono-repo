
#include <ManagementAgent/PluginCommunicationImpl/PluginManager.h>
#include <ManagementAgent/PluginCommunicationImpl/PluginProxy.h>
#include <ManagementAgent/PluginCommunication/IPluginCommunicationException.h>
#include "Common/ZeroMQWrapper/IContext.h"
#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "MockSocketRequester.h"

using ManagementAgent::PluginCommunicationImpl::PluginProxy;

class TestPluginProxy : public ::testing::Test
{
public:
    TestPluginProxy()
    {
        m_mockSocketRequester = new StrictMock<MockSocketRequester>();

        m_pluginProxy = std::unique_ptr<PluginProxy>(
                new PluginProxy(std::unique_ptr<Common::ZeroMQWrapper::ISocketRequester>(m_mockSocketRequester),
                                "plugin_one"
                ));
    }

    ~TestPluginProxy()
    {}

    std::unique_ptr<ManagementAgent::PluginCommunicationImpl::PluginProxy> m_pluginProxy;
    MockSocketRequester *m_mockSocketRequester;
    Common::PluginProtocol::Protocol m_Protocol;

    Common::PluginProtocol::DataMessage
    createDefaultMessage(Common::PluginProtocol::Commands command, const std::string &firstPayloadItem, const std::string & appId = "plugin_one")
    {
        Common::PluginProtocol::DataMessage dataMessage;
        dataMessage.Command = command;
        dataMessage.ProtocolVersion = Common::PluginProtocol::ProtocolSerializerFactory::ProtocolVersion;
        dataMessage.ApplicationId = appId;
        dataMessage.MessageId = "";
        if (!firstPayloadItem.empty())
        {
            dataMessage.Payload.push_back(firstPayloadItem);
        }

        return dataMessage;
    }
};

// Reply error cases

TEST_F(TestPluginProxy, TestPluginProxyReplyBadCommand)
{
    auto applyPolicyMsg = createDefaultMessage(Common::PluginProtocol::Commands::REQUEST_PLUGIN_APPLY_POLICY,
                                               "thisisapolicy"
    );
    auto ackMsg = createDefaultMessage(Common::PluginProtocol::Commands::REQUEST_PLUGIN_DO_ACTION, "ACK");
    auto serialisedMsg = m_Protocol.serialize(applyPolicyMsg);
    EXPECT_CALL(*m_mockSocketRequester, write(serialisedMsg)).WillOnce(
            Return());
    EXPECT_CALL(*m_mockSocketRequester, read()).WillOnce(Return(m_Protocol.serialize(ackMsg)));
    EXPECT_THROW(m_pluginProxy->applyNewPolicy("plugin_one", "thisisapolicy"), ManagementAgent::PluginCommunication::IPluginCommunicationException);
}

TEST_F(TestPluginProxy, TestPluginProxyReplyErrorMessage)
{
    auto getStatusMsg = createDefaultMessage(Common::PluginProtocol::Commands::REQUEST_PLUGIN_STATUS, std::string());
    auto serialisedMsg = m_Protocol.serialize(getStatusMsg);
    EXPECT_CALL(*m_mockSocketRequester, write(serialisedMsg)).WillOnce(
            Return());
    auto ackMsg = createDefaultMessage(Common::PluginProtocol::Commands::REQUEST_PLUGIN_STATUS, "ACK");
    ackMsg.Error = "RandomError";
    EXPECT_CALL(*m_mockSocketRequester, read()).WillOnce(Return(m_Protocol.serialize(ackMsg)));
    EXPECT_THROW(m_pluginProxy->getStatus(), ManagementAgent::PluginCommunication::IPluginCommunicationException);
}

TEST_F(TestPluginProxy, TestPluginProxyReplyWithStdException)
{
    EXPECT_CALL(*m_mockSocketRequester, write(_)).WillOnce(
            Throw(std::exception()));
    EXPECT_THROW(m_pluginProxy->applyNewPolicy("plugin_one", "thisisapolicy"),
                 ManagementAgent::PluginCommunication::IPluginCommunicationException);
}

// Apply Policy

TEST_F(TestPluginProxy, TestPluginProxyApplyNewPolicy)
{
    auto applyPolicyMsg = createDefaultMessage(Common::PluginProtocol::Commands::REQUEST_PLUGIN_APPLY_POLICY,
                                               "thisisapolicy"
    );
    auto ackMsg = createDefaultMessage(Common::PluginProtocol::Commands::REQUEST_PLUGIN_APPLY_POLICY, "ACK");
    auto serialisedMsg = m_Protocol.serialize(applyPolicyMsg);
    EXPECT_CALL(*m_mockSocketRequester, write(serialisedMsg)).WillOnce(
            Return());
    EXPECT_CALL(*m_mockSocketRequester, read()).WillOnce(Return(m_Protocol.serialize(ackMsg)));
    ASSERT_NO_THROW(m_pluginProxy->applyNewPolicy("plugin_one", "thisisapolicy"));
}


TEST_F(TestPluginProxy, TestPluginProxyApplyPolicyReplyNoAck)
{
    auto applyPolicyMsg = createDefaultMessage(Common::PluginProtocol::Commands::REQUEST_PLUGIN_APPLY_POLICY,
                                            "thisisapolicy"
    );
    auto ackMsg = createDefaultMessage(Common::PluginProtocol::Commands::REQUEST_PLUGIN_APPLY_POLICY, "NOACK");
    auto serialisedMsg = m_Protocol.serialize(applyPolicyMsg);
    EXPECT_CALL(*m_mockSocketRequester, write(serialisedMsg)).WillOnce(
            Return());
    EXPECT_CALL(*m_mockSocketRequester, read()).WillOnce(Return(m_Protocol.serialize(ackMsg)));
    EXPECT_THROW(m_pluginProxy->applyNewPolicy("plugin_one", "thisisapolicy"), ManagementAgent::PluginCommunication::IPluginCommunicationException);
}

// Do Action

TEST_F(TestPluginProxy, TestPluginProxyDoAction)
{
    auto doActionMsg = createDefaultMessage(Common::PluginProtocol::Commands::REQUEST_PLUGIN_DO_ACTION,
                                               "thisisanaction"
    );
    auto ackMsg = createDefaultMessage(Common::PluginProtocol::Commands::REQUEST_PLUGIN_DO_ACTION, "ACK");
    auto serialisedMsg = m_Protocol.serialize(doActionMsg);
    EXPECT_CALL(*m_mockSocketRequester, write(serialisedMsg)).WillOnce(
            Return());
    EXPECT_CALL(*m_mockSocketRequester, read()).WillOnce(Return(m_Protocol.serialize(ackMsg)));
    ASSERT_NO_THROW(m_pluginProxy->doAction("plugin_one", "thisisanaction"));
}


TEST_F(TestPluginProxy, TestPluginProxyDoActionReplyNoAck)
{
    auto doActionMsg = createDefaultMessage(Common::PluginProtocol::Commands::REQUEST_PLUGIN_DO_ACTION,
                                            "thisisanaction"
    );
    auto ackMsg = createDefaultMessage(Common::PluginProtocol::Commands::REQUEST_PLUGIN_APPLY_POLICY, "NOACK");
    auto serialisedMsg = m_Protocol.serialize(doActionMsg);
    EXPECT_CALL(*m_mockSocketRequester, write(serialisedMsg)).WillOnce(
            Return());
    EXPECT_CALL(*m_mockSocketRequester, read()).WillOnce(Return(m_Protocol.serialize(ackMsg)));
    EXPECT_THROW(m_pluginProxy->doAction("plugin_one", "thisisanaction"), ManagementAgent::PluginCommunication::IPluginCommunicationException);
}

// Get Status

TEST_F(TestPluginProxy, TestPluginProxyGetStatus)
{
    auto getStatus = createDefaultMessage(Common::PluginProtocol::Commands::REQUEST_PLUGIN_STATUS,
                                            ""
    );
    auto ackMsg = createDefaultMessage(Common::PluginProtocol::Commands::REQUEST_PLUGIN_STATUS, "statusWithXml");
    ackMsg.Payload.emplace_back("statusWithoutXml");
    auto serialisedMsg = m_Protocol.serialize(getStatus);
    EXPECT_CALL(*m_mockSocketRequester, write(serialisedMsg)).WillOnce(
            Return());
    EXPECT_CALL(*m_mockSocketRequester, read()).WillOnce(Return(m_Protocol.serialize(ackMsg)));
    auto reply = m_pluginProxy->getStatus();
    EXPECT_EQ(reply[0].statusXml, ackMsg.Payload[0]);
    EXPECT_EQ(reply[0].statusWithoutXml, ackMsg.Payload[1]);
}

TEST_F(TestPluginProxy, TestPluginProxyGetMultipleStatuses)
{
    auto getAlcStatus = createDefaultMessage(Common::PluginProtocol::Commands::REQUEST_PLUGIN_STATUS,
                                          "", "ALC"
    );
    auto serialisedAlcMsg = m_Protocol.serialize(getAlcStatus);
    auto getMcsStatus = createDefaultMessage(Common::PluginProtocol::Commands::REQUEST_PLUGIN_STATUS,
                                          "", "MCS"
    );
    auto serialisedMcsMsg = m_Protocol.serialize(getMcsStatus);

    auto ackAlcMsg = createDefaultMessage(Common::PluginProtocol::Commands::REQUEST_PLUGIN_STATUS, "alcStatusWithXml", "ALC");
    ackAlcMsg.Payload.emplace_back("alcStatusWithoutXml");
    auto ackMcsMsg = createDefaultMessage(Common::PluginProtocol::Commands::REQUEST_PLUGIN_STATUS, "mcsStatusWithXml", "ALC");
    ackMcsMsg.Payload.emplace_back("mcsStatusWithoutXml");

    std::vector<std::string> appIds;
    appIds.emplace_back("ALC");
    appIds.emplace_back("MCS");
    m_pluginProxy->setAppIds(appIds);

    EXPECT_CALL(*m_mockSocketRequester, write(serialisedAlcMsg)).WillOnce(
            Return());
    EXPECT_CALL(*m_mockSocketRequester, write(serialisedMcsMsg)).WillOnce(
            Return());

    EXPECT_CALL(*m_mockSocketRequester, read()).WillOnce(Return(m_Protocol.serialize(ackAlcMsg))).RetiresOnSaturation();
    EXPECT_CALL(*m_mockSocketRequester, read()).WillOnce(Return(m_Protocol.serialize(ackMcsMsg))).RetiresOnSaturation();
    auto reply = m_pluginProxy->getStatus();
    EXPECT_EQ(reply[0].statusXml, ackMcsMsg.Payload[0]);
    EXPECT_EQ(reply[0].statusWithoutXml, ackMcsMsg.Payload[1]);
    EXPECT_EQ(reply[1].statusXml, ackAlcMsg.Payload[0]);
    EXPECT_EQ(reply[1].statusWithoutXml, ackAlcMsg.Payload[1]);

}

// Get Telemetry

TEST_F(TestPluginProxy, TestPluginProxyGetTelemetry)
{
    auto getTelemetry = createDefaultMessage(Common::PluginProtocol::Commands::REQUEST_PLUGIN_TELEMETRY,
                                          "", "Telemetry request"
    );
    auto ackMsg = createDefaultMessage(Common::PluginProtocol::Commands::REQUEST_PLUGIN_TELEMETRY, "Telemetry");
    auto serialisedMsg = m_Protocol.serialize(getTelemetry);
    EXPECT_CALL(*m_mockSocketRequester, write(serialisedMsg)).WillOnce(
            Return());
    EXPECT_CALL(*m_mockSocketRequester, read()).WillOnce(Return(m_Protocol.serialize(ackMsg)));
    auto reply = m_pluginProxy->getTelemetry();
    EXPECT_EQ(reply, ackMsg.Payload[0]);
}

// APP IDS

TEST_F(TestPluginProxy, TestPluginProxyHasAppIds)
{
    ASSERT_FALSE(m_pluginProxy->hasAppId("ALC"));
    std::vector<std::string> appIds;
    appIds.emplace_back("ALC");
    appIds.emplace_back("MCS");
    m_pluginProxy->setAppIds(appIds);
    ASSERT_TRUE(m_pluginProxy->hasAppId("ALC"));
    ASSERT_TRUE(m_pluginProxy->hasAppId("MCS"));
    ASSERT_FALSE(m_pluginProxy->hasAppId("INVALID"));
}

TEST_F(TestPluginProxy, TestDefaultPluginProxyAppIdIsPluginName)
{
    ASSERT_TRUE(m_pluginProxy->hasAppId("plugin_one"));
}
