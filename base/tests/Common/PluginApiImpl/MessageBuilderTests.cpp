/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "TestCompare.h"

#include <Common/Logging/ConsoleLoggingSetup.h>
#include <Common/PluginProtocol/MessageBuilder.h>
#include <Common/PluginProtocol/ProtocolSerializerFactory.h>
#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>

#include <memory>

using Common::PluginProtocol::DataMessage;
using Common::PluginProtocol::MessageBuilder;

class MessageBuilderTests : public TestCompare
{
public:
    void SetUp() override
    {
        defaultAppId = "testApp";
        defaultPluginName = "PluginApp";
        m_messageBuilder = std::unique_ptr<MessageBuilder>(new MessageBuilder(defaultPluginName));
    }

    void TearDown() override { m_messageBuilder.reset(); }

    DataMessage createDataMessage()
    {
        DataMessage dataMessage;
        dataMessage.m_applicationId = defaultAppId;
        dataMessage.m_pluginName = defaultPluginName;
        dataMessage.m_command = Common::PluginProtocol::Commands::UNKNOWN;
        return dataMessage;
    }
    Common::Logging::ConsoleLoggingSetup m_consoleLogging;
    std::string defaultAppId;
    std::string defaultPluginName;
    std::unique_ptr<MessageBuilder> m_messageBuilder;
};

TEST_F(MessageBuilderTests, requestSendEventMessageReturnsExpectedMessage) // NOLINT
{
    DataMessage expectedMessage = createDataMessage();
    expectedMessage.m_command = Common::PluginProtocol::Commands::PLUGIN_SEND_EVENT;
    std::string payload = "EventXml";
    expectedMessage.m_payload.push_back(payload);

    DataMessage actualMessage = m_messageBuilder->requestSendEventMessage(defaultAppId, payload);

    EXPECT_PRED_FORMAT2(dataMessageSimilar, expectedMessage, actualMessage);
}

TEST_F(MessageBuilderTests, requestSendStatusMessageReturnsExpectedMessage) // NOLINT
{
    DataMessage expectedMessage = createDataMessage();
    expectedMessage.m_command = Common::PluginProtocol::Commands::PLUGIN_SEND_STATUS;
    Common::PluginApi::StatusInfo statusInfo;
    statusInfo.statusXml = "StatusXml";
    statusInfo.statusWithoutTimestampsXml = "StatusWithoutTimeStampXml";

    expectedMessage.m_payload.push_back(statusInfo.statusXml);
    expectedMessage.m_payload.push_back(statusInfo.statusWithoutTimestampsXml);

    DataMessage actualMessage = m_messageBuilder->requestSendStatusMessage(
        defaultAppId, statusInfo.statusXml, statusInfo.statusWithoutTimestampsXml);

    EXPECT_PRED_FORMAT2(dataMessageSimilar, expectedMessage, actualMessage);
}

TEST_F(MessageBuilderTests, requestRegisterMessageReturnsExpectedMessage) // NOLINT
{
    DataMessage expectedMessage = createDataMessage();
    expectedMessage.m_command = Common::PluginProtocol::Commands::PLUGIN_SEND_REGISTER;
    expectedMessage.m_applicationId = "";

    DataMessage actualMessage = m_messageBuilder->requestRegisterMessage();

    EXPECT_PRED_FORMAT2(dataMessageSimilar, expectedMessage, actualMessage);
}

TEST_F(MessageBuilderTests, requestCurrentPolicyMessageReturnsExpectedMessage) // NOLINT
{
    DataMessage expectedMessage = createDataMessage();
    expectedMessage.m_command = Common::PluginProtocol::Commands::PLUGIN_QUERY_CURRENT_POLICY;

    DataMessage actualMessage = m_messageBuilder->requestCurrentPolicyMessage(defaultAppId);

    EXPECT_PRED_FORMAT2(dataMessageSimilar, expectedMessage, actualMessage);
}

TEST_F(MessageBuilderTests, requestApplyPolicyMessageReturnsExpectedMessage) // NOLINT
{
    DataMessage expectedMessage = createDataMessage();
    expectedMessage.m_command = Common::PluginProtocol::Commands::REQUEST_PLUGIN_APPLY_POLICY;
    std::string payload = "PolicyXml";
    expectedMessage.m_payload.push_back(payload);

    DataMessage actualMessage = m_messageBuilder->requestApplyPolicyMessage(defaultAppId, payload);

    EXPECT_PRED_FORMAT2(dataMessageSimilar, expectedMessage, actualMessage);
}

TEST_F(MessageBuilderTests, requestDoActionMessageReturnsExpectedMessage) // NOLINT
{
    DataMessage expectedMessage = createDataMessage();
    expectedMessage.m_command = Common::PluginProtocol::Commands::REQUEST_PLUGIN_DO_ACTION;
    std::string payload = "ActionXml";
    expectedMessage.m_payload.push_back(payload);
    DataMessage actualMessage = m_messageBuilder->requestDoActionMessage(defaultAppId, payload, "");

    EXPECT_PRED_FORMAT2(dataMessageSimilar, expectedMessage, actualMessage);
}

TEST_F(MessageBuilderTests, requestRequestPluginStatusMessageReturnsExpectedMessage) // NOLINT
{
    DataMessage expectedMessage = createDataMessage();
    expectedMessage.m_command = Common::PluginProtocol::Commands::REQUEST_PLUGIN_STATUS;

    DataMessage actualMessage = m_messageBuilder->requestRequestPluginStatusMessage(defaultAppId);

    EXPECT_PRED_FORMAT2(dataMessageSimilar, expectedMessage, actualMessage);
}

TEST_F(MessageBuilderTests, requestRequestTelemetryMessageReturnsExpectedMessage) // NOLINT
{
    DataMessage expectedMessage = createDataMessage();
    expectedMessage.m_command = Common::PluginProtocol::Commands::REQUEST_PLUGIN_TELEMETRY;
    expectedMessage.m_applicationId = "";
    DataMessage actualMessage = m_messageBuilder->requestRequestTelemetryMessage();

    EXPECT_PRED_FORMAT2(dataMessageSimilar, expectedMessage, actualMessage);
}

TEST_F(MessageBuilderTests, replyAckMessageReturnsExpectedMessage) // NOLINT
{
    DataMessage expectedMessage = createDataMessage();
    expectedMessage.m_command = Common::PluginProtocol::Commands::REQUEST_PLUGIN_TELEMETRY;

    DataMessage actualMessage = m_messageBuilder->replyAckMessage(expectedMessage);

    expectedMessage.m_acknowledge = true;

    EXPECT_PRED_FORMAT2(dataMessageSimilar, expectedMessage, actualMessage);
}

TEST_F(MessageBuilderTests, replySetErrorIfEmptyReturnsExpectedMessageWhenErrorIsEmpty) // NOLINT
{
    DataMessage expectedMessage = createDataMessage();
    expectedMessage.m_command = Common::PluginProtocol::Commands::REQUEST_PLUGIN_TELEMETRY;
    std::string errorMessage = "Something went wrong";

    DataMessage actualMessage = m_messageBuilder->replySetErrorIfEmpty(expectedMessage, errorMessage);

    expectedMessage.m_error = errorMessage;

    EXPECT_PRED_FORMAT2(dataMessageSimilar, expectedMessage, actualMessage);
}

TEST_F(MessageBuilderTests, replySetErrorIfEmptyReturnsExpectedMessageWhenErrorNotEmpty) // NOLINT
{
    DataMessage expectedMessage = createDataMessage();
    expectedMessage.m_command = Common::PluginProtocol::Commands::REQUEST_PLUGIN_TELEMETRY;
    std::string errorMessage = "Something went wrong";
    expectedMessage.m_error = errorMessage;

    DataMessage actualMessage = m_messageBuilder->replySetErrorIfEmpty(expectedMessage, "New Error");

    EXPECT_PRED_FORMAT2(dataMessageSimilar, expectedMessage, actualMessage);
}

TEST_F(MessageBuilderTests, replyCurrentPolicyReturnsExpectedMessage) // NOLINT
{
    DataMessage expectedMessage = createDataMessage();
    expectedMessage.m_command = Common::PluginProtocol::Commands::PLUGIN_QUERY_CURRENT_POLICY;

    std::string payload = "PolicyXml";
    expectedMessage.m_payload.push_back(payload);

    DataMessage actualMessage = m_messageBuilder->replyCurrentPolicy(expectedMessage, payload);

    EXPECT_PRED_FORMAT2(dataMessageSimilar, expectedMessage, actualMessage);
}

TEST_F(MessageBuilderTests, replyTelemetryReturnsExpectedMessage) // NOLINT
{
    DataMessage expectedMessage = createDataMessage();
    expectedMessage.m_command = Common::PluginProtocol::Commands::REQUEST_PLUGIN_TELEMETRY;

    std::string payload = "TelemetryXml";
    expectedMessage.m_payload.push_back(payload);

    DataMessage actualMessage = m_messageBuilder->replyTelemetry(expectedMessage, payload);

    EXPECT_PRED_FORMAT2(dataMessageSimilar, expectedMessage, actualMessage);
}

TEST_F(MessageBuilderTests, replyStatusReturnsExpectedMessage) // NOLINT
{
    DataMessage expectedMessage = createDataMessage();
    expectedMessage.m_command = Common::PluginProtocol::Commands::REQUEST_PLUGIN_STATUS;

    Common::PluginApi::StatusInfo statusInfo;
    statusInfo.statusXml = "StatusXml";
    statusInfo.statusWithoutTimestampsXml = "StatusWithoutTimeStampXml";

    expectedMessage.m_payload.push_back(statusInfo.statusXml);
    expectedMessage.m_payload.push_back(statusInfo.statusWithoutTimestampsXml);

    DataMessage actualMessage = m_messageBuilder->replyStatus(expectedMessage, statusInfo);

    EXPECT_PRED_FORMAT2(dataMessageSimilar, expectedMessage, actualMessage);
}

TEST_F(MessageBuilderTests, requestExtractEventReturnsExpectedMessage) // NOLINT
{
    DataMessage message = createDataMessage();
    message.m_command = Common::PluginProtocol::Commands::PLUGIN_SEND_EVENT;

    std::string expectedResult = "EventXml";

    message.m_payload.push_back(expectedResult);

    std::string actualResult = m_messageBuilder->requestExtractEvent(message);

    EXPECT_EQ(expectedResult, actualResult);
}

TEST_F(MessageBuilderTests, requestExtractStatusReturnsExpectedMessage) // NOLINT
{
    DataMessage message = createDataMessage();
    message.m_command = Common::PluginProtocol::Commands::REQUEST_PLUGIN_STATUS;

    Common::PluginApi::StatusInfo expectedStatusInfo;
    expectedStatusInfo.statusXml = "StatusXml";
    expectedStatusInfo.statusWithoutTimestampsXml = "StatusWithoutTimeStampXml";

    message.m_payload.push_back(expectedStatusInfo.statusXml);
    message.m_payload.push_back(expectedStatusInfo.statusWithoutTimestampsXml);

    Common::PluginApi::StatusInfo actualStatusInfo = m_messageBuilder->requestExtractStatus(message);

    EXPECT_EQ(expectedStatusInfo.statusXml, actualStatusInfo.statusXml);
    EXPECT_EQ(expectedStatusInfo.statusWithoutTimestampsXml, actualStatusInfo.statusWithoutTimestampsXml);
}

TEST_F(MessageBuilderTests, requestExtractPolicyReturnsExpectedMessage) // NOLINT
{
    DataMessage message = createDataMessage();
    message.m_command = Common::PluginProtocol::Commands::REQUEST_PLUGIN_APPLY_POLICY;

    std::string expectedResult = "PolicyXml";

    message.m_payload.push_back(expectedResult);

    std::string actualResult = m_messageBuilder->requestExtractPolicy(message);

    EXPECT_EQ(expectedResult, actualResult);
}

TEST_F(MessageBuilderTests, requestExtractActionReturnsExpectedMessage) // NOLINT
{
    DataMessage message = createDataMessage();
    message.m_command = Common::PluginProtocol::Commands::REQUEST_PLUGIN_DO_ACTION;

    std::string expectedResult = "ActionXml";

    message.m_payload.push_back(expectedResult);

    std::string actualResult = m_messageBuilder->requestExtractAction(message);

    EXPECT_EQ(expectedResult, actualResult);
}

TEST_F(MessageBuilderTests, replyExtractCurrentPolicyReturnsExpectedMessage) // NOLINT
{
    DataMessage message = createDataMessage();
    message.m_command = Common::PluginProtocol::Commands::PLUGIN_QUERY_CURRENT_POLICY;

    std::string expectedResult = "PolicyXml";

    message.m_payload.push_back(expectedResult);

    std::string actualResult = m_messageBuilder->replyExtractCurrentPolicy(message);

    EXPECT_EQ(expectedResult, actualResult);
}

TEST_F(MessageBuilderTests, replyExtractTelemetryReturnsExpectedMessage) // NOLINT
{
    DataMessage message = createDataMessage();
    message.m_command = Common::PluginProtocol::Commands::REQUEST_PLUGIN_TELEMETRY;

    std::string expectedResult = "TelemetryXml";

    message.m_payload.push_back(expectedResult);

    std::string actualResult = m_messageBuilder->replyExtractTelemetry(message);

    EXPECT_EQ(expectedResult, actualResult);
}

TEST_F(MessageBuilderTests, hasAckReturnsTrue) // NOLINT
{
    DataMessage message = createDataMessage();
    message.m_command = Common::PluginProtocol::Commands::PLUGIN_SEND_EVENT;

    message.m_acknowledge = true;

    EXPECT_TRUE(m_messageBuilder->hasAck(message));
}
