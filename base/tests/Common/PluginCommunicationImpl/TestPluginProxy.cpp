/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "MockSocketRequester.h"

#include "Common/Logging/ConsoleLoggingSetup.h"
#include "Common/PluginCommunication/IPluginCommunicationException.h"
#include "Common/PluginCommunicationImpl/PluginProxy.h"
#include "Common/PluginProtocol/Protocol.h"
#include "Common/ZMQWrapperApi/IContext.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "tests/Common/Helpers/FileSystemReplaceAndRestore.h"
#include "tests/Common/Helpers/MockFileSystem.h"

using Common::PluginCommunicationImpl::PluginProxy;

class TestPluginProxy : public ::testing::Test
{
public:
    TestPluginProxy() : m_mockSocketRequester(new StrictMock<MockSocketRequester>())
    {
        m_pluginProxy.reset(new PluginProxy(
            std::unique_ptr<Common::ZeroMQWrapper::ISocketRequester>(m_mockSocketRequester), "plugin_one"));
    }

    ~TestPluginProxy() override = default;

    std::unique_ptr<Common::PluginCommunicationImpl::PluginProxy> m_pluginProxy;
    MockSocketRequester* m_mockSocketRequester;
    Common::PluginProtocol::Protocol m_Protocol;

    Common::PluginProtocol::DataMessage createDefaultMessage(
        Common::PluginProtocol::Commands command,
        const std::string& firstPayloadItem,
        const std::string& appId = "plugin_one")
    {
        Common::PluginProtocol::DataMessage dataMessage;
        dataMessage.m_command = command;
        dataMessage.m_applicationId = appId;
        dataMessage.m_pluginName = appId;
        if (!firstPayloadItem.empty())
        {
            dataMessage.m_payload.push_back(firstPayloadItem);
        }

        return dataMessage;
    }

    Common::PluginProtocol::DataMessage createAcknowledgementMessage(
        Common::PluginProtocol::Commands command,
        const std::string& appId = "plugin_one")
    {
        Common::PluginProtocol::DataMessage dataMessage;
        dataMessage.m_command = command;
        dataMessage.m_applicationId = appId;
        dataMessage.m_pluginName = appId;
        dataMessage.m_acknowledge = true;
        return dataMessage;
    }

private:
    Common::Logging::ConsoleLoggingSetup m_loggingSetup;
};

// Reply error cases

TEST_F(TestPluginProxy, TestPluginProxyReplyBadCommand) // NOLINT
{
    auto applyPolicyMsg =
        createDefaultMessage(Common::PluginProtocol::Commands::REQUEST_PLUGIN_APPLY_POLICY, "thisisapolicy.xml");
    // Command REQUEST_PLUGIN_DO_ACTION instead of the expected REQUEST_PLUGIN_APPLY_POLICY
    auto ackMsg = createAcknowledgementMessage(Common::PluginProtocol::Commands::REQUEST_PLUGIN_DO_ACTION);
    auto serialisedMsg = m_Protocol.serialize(applyPolicyMsg);
    EXPECT_CALL(*m_mockSocketRequester, write(serialisedMsg)).WillOnce(Return());
    EXPECT_CALL(*m_mockSocketRequester, read()).WillOnce(Return(m_Protocol.serialize(ackMsg)));
    EXPECT_THROW( // NOLINT
        m_pluginProxy->applyNewPolicy("plugin_one", "thisisapolicy.xml"),
        Common::PluginCommunication::IPluginCommunicationException);
}

TEST_F(TestPluginProxy, TestPluginProxyReplyErrorMessage) // NOLINT
{
    auto getStatusMsg = createDefaultMessage(Common::PluginProtocol::Commands::REQUEST_PLUGIN_STATUS, std::string());
    auto serialisedMsg = m_Protocol.serialize(getStatusMsg);
    EXPECT_CALL(*m_mockSocketRequester, write(serialisedMsg)).WillOnce(Return());
    auto ackMsg = createAcknowledgementMessage(Common::PluginProtocol::Commands::REQUEST_PLUGIN_STATUS);
    ackMsg.m_error = "RandomError";
    EXPECT_CALL(*m_mockSocketRequester, read()).WillOnce(Return(m_Protocol.serialize(ackMsg)));
    EXPECT_THROW( // NOLINT
        m_pluginProxy->getStatus(),
        Common::PluginCommunication::IPluginCommunicationException);
}

TEST_F(TestPluginProxy, TestPluginProxyReplyWithStdException) // NOLINT
{
    auto mockFileSystem = new StrictMock<MockFileSystem>();
    std::unique_ptr<MockFileSystem> mockIFileSystemPtr(mockFileSystem);
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockIFileSystemPtr));

    //EXPECT_CALL(*mockFileSystem, readFile(_)).WillOnce(Return("thisisapolicy"));
    EXPECT_CALL(*m_mockSocketRequester, write(_)).WillOnce(Throw(std::exception()));
    EXPECT_THROW( // NOLINT
        m_pluginProxy->applyNewPolicy("plugin_one", "thisisapolicy"),
        Common::PluginCommunication::IPluginCommunicationException);
}

// Apply Policy

TEST_F(TestPluginProxy, TestPluginProxyApplyNewPolicy) // NOLINT
{
    auto applyPolicyMsg =
        createDefaultMessage(Common::PluginProtocol::Commands::REQUEST_PLUGIN_APPLY_POLICY, "thisisapolicy.xml");
    auto ackMsg = createAcknowledgementMessage(Common::PluginProtocol::Commands::REQUEST_PLUGIN_APPLY_POLICY);
    auto serialisedMsg = m_Protocol.serialize(applyPolicyMsg);
    EXPECT_CALL(*m_mockSocketRequester, write(serialisedMsg)).WillOnce(Return());
    EXPECT_CALL(*m_mockSocketRequester, read()).WillOnce(Return(m_Protocol.serialize(ackMsg)));
    ASSERT_NO_THROW(m_pluginProxy->applyNewPolicy("plugin_one", "thisisapolicy.xml")); // NOLINT

//    auto mockFileSystem = new StrictMock<MockFileSystem>();
//    std::unique_ptr<MockFileSystem> mockIFileSystemPtr(mockFileSystem);
//    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockIFileSystemPtr));
//    std::string policyXmlFilePath("/opt/sophos-spl/base/mcs/policy/policy.xml");
//
//    auto applyPolicyMsg =
//        createDefaultMessage(Common::PluginProtocol::Commands::REQUEST_PLUGIN_APPLY_POLICY, "thisisapolicy");
//    auto ackMsg = createAcknowledgementMessage(Common::PluginProtocol::Commands::REQUEST_PLUGIN_APPLY_POLICY);
//    auto serialisedMsg = m_Protocol.serialize(applyPolicyMsg);
//    //EXPECT_CALL(*mockFileSystem, readFile(policyXmlFilePath)).WillOnce(Return("thisisapolicy"));
//    EXPECT_CALL(*m_mockSocketRequester, write(serialisedMsg)).WillOnce(Return());
//    EXPECT_CALL(*m_mockSocketRequester, read()).WillOnce(Return(m_Protocol.serialize(ackMsg)));
//    ASSERT_NO_THROW(m_pluginProxy->applyNewPolicy("plugin_one", policyXmlFilePath)); // NOLINT
}

TEST_F(TestPluginProxy, TestPluginProxyApplyPolicyReplyNoAck) // NOLINT
{
    auto applyPolicyMsg =
        createDefaultMessage(Common::PluginProtocol::Commands::REQUEST_PLUGIN_APPLY_POLICY, "thisisapolicy.xml");
    auto ackMsg = createDefaultMessage(Common::PluginProtocol::Commands::REQUEST_PLUGIN_APPLY_POLICY, "NOACK");
    auto serialisedMsg = m_Protocol.serialize(applyPolicyMsg);
    EXPECT_CALL(*m_mockSocketRequester, write(serialisedMsg)).WillOnce(Return());
    EXPECT_CALL(*m_mockSocketRequester, read()).WillOnce(Return(m_Protocol.serialize(ackMsg)));
    EXPECT_THROW( // NOLINT
        m_pluginProxy->applyNewPolicy("plugin_one", "thisisapolicy.xml"),
        Common::PluginCommunication::IPluginCommunicationException);

//    auto mockFileSystem = new StrictMock<MockFileSystem>();
//    std::unique_ptr<MockFileSystem> mockIFileSystemPtr(mockFileSystem);
//    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockIFileSystemPtr));
//    std::string policyXmlFilePath("/opt/sophos-spl/base/mcs/policy/policy.xml");
//
//    auto applyPolicyMsg =
//        createDefaultMessage(Common::PluginProtocol::Commands::REQUEST_PLUGIN_APPLY_POLICY, "thisisapolicy");
//    auto ackMsg = createDefaultMessage(Common::PluginProtocol::Commands::REQUEST_PLUGIN_APPLY_POLICY, "NOACK");
//    auto serialisedMsg = m_Protocol.serialize(applyPolicyMsg);
//    //EXPECT_CALL(*mockFileSystem, readFile(policyXmlFilePath)).WillOnce(Return("thisisapolicy"));
//    EXPECT_CALL(*m_mockSocketRequester, write(serialisedMsg)).WillOnce(Return());
//    EXPECT_CALL(*m_mockSocketRequester, read()).WillOnce(Return(m_Protocol.serialize(ackMsg)));
//    EXPECT_THROW( // NOLINT
//        m_pluginProxy->applyNewPolicy("plugin_one", policyXmlFilePath),
//        Common::PluginCommunication::IPluginCommunicationException);
}

// Do Action

TEST_F(TestPluginProxy, TestPluginProxyDoAction) // NOLINT
{
    auto doActionMsg =
        createDefaultMessage(Common::PluginProtocol::Commands::REQUEST_PLUGIN_DO_ACTION, "thisisanaction.xml");
    doActionMsg.m_correlationId = "correlation-id";
    auto ackMsg = createAcknowledgementMessage(Common::PluginProtocol::Commands::REQUEST_PLUGIN_DO_ACTION);
    auto serialisedMsg = m_Protocol.serialize(doActionMsg);
    EXPECT_CALL(*m_mockSocketRequester, write(serialisedMsg)).WillOnce(Return());
    EXPECT_CALL(*m_mockSocketRequester, read()).WillOnce(Return(m_Protocol.serialize(ackMsg)));
    ASSERT_NO_THROW(m_pluginProxy->queueAction("plugin_one", "thisisanaction.xml", "correlation-id"));

//    auto mockFileSystem = new StrictMock<MockFileSystem>();
//    std::unique_ptr<MockFileSystem> mockIFileSystemPtr(mockFileSystem);
//    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockIFileSystemPtr));
//    std::string actionXmlFilePath("/opt/sophos-spl/base/mcs/action/action.xml");
//
//    auto doActionMsg =
//        createDefaultMessage(Common::PluginProtocol::Commands::REQUEST_PLUGIN_DO_ACTION, "thisisanaction");
//    doActionMsg.m_correlationId = "correlation-id";
//    auto ackMsg = createAcknowledgementMessage(Common::PluginProtocol::Commands::REQUEST_PLUGIN_DO_ACTION);
//    auto serialisedMsg = m_Protocol.serialize(doActionMsg);
//    //EXPECT_CALL(*mockFileSystem, readFile(actionXmlFilePath)).WillOnce(Return("thisisanaction"));
//    EXPECT_CALL(*m_mockSocketRequester, write(serialisedMsg)).WillOnce(Return());
//    EXPECT_CALL(*m_mockSocketRequester, read()).WillOnce(Return(m_Protocol.serialize(ackMsg)));
//    ASSERT_NO_THROW(m_pluginProxy->queueAction("plugin_one", actionXmlFilePath, "correlation-id"));
}

TEST_F(TestPluginProxy, TestPluginProxyDoActionReplyNoAck) // NOLINT
{
    auto doActionMsg =
        createDefaultMessage(Common::PluginProtocol::Commands::REQUEST_PLUGIN_DO_ACTION, "thisisanaction.xml");
    auto ackMsg = createDefaultMessage(Common::PluginProtocol::Commands::REQUEST_PLUGIN_APPLY_POLICY, "NOACK");
    auto serialisedMsg = m_Protocol.serialize(doActionMsg);
    EXPECT_CALL(*m_mockSocketRequester, write(serialisedMsg)).WillOnce(Return());
    EXPECT_CALL(*m_mockSocketRequester, read()).WillOnce(Return(m_Protocol.serialize(ackMsg)));
    EXPECT_THROW( // NOLINT
        m_pluginProxy->queueAction("plugin_one", "thisisanaction.xml", ""),
        Common::PluginCommunication::IPluginCommunicationException);

//    auto mockFileSystem = new StrictMock<MockFileSystem>();
//    std::unique_ptr<MockFileSystem> mockIFileSystemPtr(mockFileSystem);
//    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockIFileSystemPtr));
//    std::string actionXmlFilePath("/opt/sophos-spl/base/mcs/action/action.xml");
//
//    auto doActionMsg =
//        createDefaultMessage(Common::PluginProtocol::Commands::REQUEST_PLUGIN_DO_ACTION, "thisisanaction");
//    auto ackMsg = createDefaultMessage(Common::PluginProtocol::Commands::REQUEST_PLUGIN_APPLY_POLICY, "NOACK");
//    auto serialisedMsg = m_Protocol.serialize(doActionMsg);
//    //EXPECT_CALL(*mockFileSystem, readFile(actionXmlFilePath)).WillOnce(Return("thisisanaction"));
//    EXPECT_CALL(*m_mockSocketRequester, write(serialisedMsg)).WillOnce(Return());
//    EXPECT_CALL(*m_mockSocketRequester, read()).WillOnce(Return(m_Protocol.serialize(ackMsg)));
//    EXPECT_THROW( // NOLINT
//        m_pluginProxy->queueAction("plugin_one", actionXmlFilePath, ""),
//        Common::PluginCommunication::IPluginCommunicationException);
}

// Get Status

TEST_F(TestPluginProxy, TestPluginProxyGetStatus) // NOLINT
{
    auto getStatus = createDefaultMessage(Common::PluginProtocol::Commands::REQUEST_PLUGIN_STATUS, "");
    auto ackMsg = createDefaultMessage(Common::PluginProtocol::Commands::REQUEST_PLUGIN_STATUS, "statusWithXml");
    ackMsg.m_payload.emplace_back("statusWithoutXml");
    auto serialisedMsg = m_Protocol.serialize(getStatus);
    EXPECT_CALL(*m_mockSocketRequester, write(serialisedMsg)).WillOnce(Return());
    EXPECT_CALL(*m_mockSocketRequester, read()).WillOnce(Return(m_Protocol.serialize(ackMsg)));
    auto reply = m_pluginProxy->getStatus();
    EXPECT_EQ(reply[0].statusXml, ackMsg.m_payload[0]);
    EXPECT_EQ(reply[0].statusWithoutTimestampsXml, ackMsg.m_payload[1]);
}

TEST_F(TestPluginProxy, TestPluginProxyGetMultipleStatuses) // NOLINT
{
    auto getAlcStatus = createDefaultMessage(Common::PluginProtocol::Commands::REQUEST_PLUGIN_STATUS, "", "ALC");
    getAlcStatus.m_pluginName = "plugin_one";
    auto serialisedAlcMsg = m_Protocol.serialize(getAlcStatus);
    auto getMcsStatus = createDefaultMessage(Common::PluginProtocol::Commands::REQUEST_PLUGIN_STATUS, "", "MCS");
    getMcsStatus.m_pluginName = "plugin_one";
    auto serialisedMcsMsg = m_Protocol.serialize(getMcsStatus);

    auto ackAlcMsg =
        createDefaultMessage(Common::PluginProtocol::Commands::REQUEST_PLUGIN_STATUS, "alcStatusWithXml", "ALC");
    ackAlcMsg.m_payload.emplace_back("alcStatusWithoutXml");
    auto ackMcsMsg =
        createDefaultMessage(Common::PluginProtocol::Commands::REQUEST_PLUGIN_STATUS, "mcsStatusWithXml", "ALC");
    ackMcsMsg.m_payload.emplace_back("mcsStatusWithoutXml");

    std::vector<std::string> appIds;
    appIds.emplace_back("ALC");
    appIds.emplace_back("MCS");
    m_pluginProxy->setStatusAppIds(appIds);

    EXPECT_CALL(*m_mockSocketRequester, write(serialisedAlcMsg)).WillOnce(Return());
    EXPECT_CALL(*m_mockSocketRequester, write(serialisedMcsMsg)).WillOnce(Return());

    EXPECT_CALL(*m_mockSocketRequester, read()).WillOnce(Return(m_Protocol.serialize(ackAlcMsg))).RetiresOnSaturation();
    EXPECT_CALL(*m_mockSocketRequester, read()).WillOnce(Return(m_Protocol.serialize(ackMcsMsg))).RetiresOnSaturation();
    auto reply = m_pluginProxy->getStatus();
    EXPECT_EQ(reply[0].statusXml, ackMcsMsg.m_payload[0]);
    EXPECT_EQ(reply[0].statusWithoutTimestampsXml, ackMcsMsg.m_payload[1]);
    EXPECT_EQ(reply[1].statusXml, ackAlcMsg.m_payload[0]);
    EXPECT_EQ(reply[1].statusWithoutTimestampsXml, ackAlcMsg.m_payload[1]);
}

// Get Telemetry

TEST_F(TestPluginProxy, TestPluginProxyGetTelemetry) // NOLINT
{
    auto getTelemetry = createDefaultMessage(Common::PluginProtocol::Commands::REQUEST_PLUGIN_TELEMETRY, "", "");
    getTelemetry.m_pluginName = "plugin_one";
    auto ackMsg = createDefaultMessage(Common::PluginProtocol::Commands::REQUEST_PLUGIN_TELEMETRY, "Telemetry");
    auto serialisedMsg = m_Protocol.serialize(getTelemetry);
    EXPECT_CALL(*m_mockSocketRequester, write(serialisedMsg)).WillOnce(Return());
    EXPECT_CALL(*m_mockSocketRequester, read()).WillOnce(Return(m_Protocol.serialize(ackMsg)));
    auto reply = m_pluginProxy->getTelemetry();
    EXPECT_EQ(reply, ackMsg.m_payload[0]);
}

// APP IDS

TEST_F(TestPluginProxy, TestPluginProxyHasPolicyAppIds) // NOLINT
{
    ASSERT_FALSE(m_pluginProxy->hasPolicyAppId("ALC"));
    std::vector<std::string> appIds;
    appIds.emplace_back("ALC");
    appIds.emplace_back("MCS");
    appIds.emplace_back("ALC"); // repeat entries has no effect
    m_pluginProxy->setPolicyAppIds(appIds);
    m_pluginProxy->setActionAppIds(appIds);
    ASSERT_TRUE(m_pluginProxy->hasPolicyAppId("ALC"));
    ASSERT_TRUE(m_pluginProxy->hasPolicyAppId("MCS"));
    ASSERT_FALSE(m_pluginProxy->hasPolicyAppId("INVALID"));
    ASSERT_TRUE(m_pluginProxy->hasActionAppId("ALC"));
    ASSERT_TRUE(m_pluginProxy->hasActionAppId("MCS"));
    ASSERT_FALSE(m_pluginProxy->hasActionAppId("INVALID"));
}

TEST_F(TestPluginProxy, TestDefaultPluginProxyAppIdIsPluginName) // NOLINT
{
    ASSERT_TRUE(m_pluginProxy->hasPolicyAppId("plugin_one"));
}

TEST_F(TestPluginProxy, TestPluginProxyHasStatusAppIds) // NOLINT
{
    ASSERT_FALSE(m_pluginProxy->hasStatusAppId("ALC"));
    std::vector<std::string> appIds;
    appIds.emplace_back("ALC");
    appIds.emplace_back("MCS");
    appIds.emplace_back("ALC"); // repeat entries has no effect
    m_pluginProxy->setStatusAppIds(appIds);
    ASSERT_TRUE(m_pluginProxy->hasStatusAppId("ALC"));
    ASSERT_TRUE(m_pluginProxy->hasStatusAppId("MCS"));
    ASSERT_FALSE(m_pluginProxy->hasStatusAppId("INVALID"));
}