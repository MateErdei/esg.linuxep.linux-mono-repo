
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
    createDefaultMessage(Common::PluginProtocol::Commands command, const std::string &firstPayloadItem)
    {
        Common::PluginProtocol::DataMessage dataMessage;
        dataMessage.Command = command;
        dataMessage.ProtocolVersion = Common::PluginProtocol::ProtocolSerializerFactory::ProtocolVersion;
        dataMessage.ApplicationId = "plugin_one";
        dataMessage.MessageId = "";
        if (!firstPayloadItem.empty())
        {
            dataMessage.Payload.push_back(firstPayloadItem);
        }

        return dataMessage;
    }
};

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

TEST_F(TestPluginProxy, TestPluginProxyApplyNewPolicyWithStdException)
{
    EXPECT_CALL(*m_mockSocketRequester, write(_)).WillOnce(
            Throw(std::exception()));
    EXPECT_THROW(m_pluginProxy->applyNewPolicy("plugin_one", "thisisapolicy"),
                 ManagementAgent::PluginCommunication::IPluginCommunicationException);
}

TEST_F(TestPluginProxy, TestPluginProxyHasAppIds)
{
    std::vector<std::string> appIds;
    ASSERT_FALSE(m_pluginProxy->hasAppId("ALC"));
    appIds.emplace_back("ALC");
    appIds.emplace_back("MCS");
    m_pluginProxy->setAppIds(appIds);
    ASSERT_TRUE(m_pluginProxy->hasAppId("ALC"));
    ASSERT_TRUE(m_pluginProxy->hasAppId("MCS"));
    ASSERT_FALSE(m_pluginProxy->hasAppId("INVALID"));
}

