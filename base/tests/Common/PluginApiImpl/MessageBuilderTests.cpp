/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include <memory>
#include <Common/PluginProtocol/ProtocolSerializerFactory.h>
#include "Common/PluginProtocol/MessageBuilder.h"
#include "TestCompare.h"

using Common::PluginProtocol::MessageBuilder;
using Common::PluginProtocol::DataMessage;


class MessageBuilderTests : public TestCompare
{
public:


    void SetUp() override
    {
        m_messageBuilder = std::unique_ptr<MessageBuilder>(
                new MessageBuilder("testApp", Common::PluginProtocol::ProtocolSerializerFactory::ProtocolVersion));
    }

    void TearDown() override
    {
        m_messageBuilder.reset();
    }

    DataMessage createDataMessage()
    {
        DataMessage dataMessage;
        dataMessage.ProtocolVersion = Common::PluginProtocol::ProtocolSerializerFactory::ProtocolVersion;
        dataMessage.ApplicationId = "testApp";
        dataMessage.MessageId = "";
        return dataMessage;
    }

    std::unique_ptr<MessageBuilder> m_messageBuilder;

};

TEST_F(MessageBuilderTests, requestSendEventMessageReturnsExpectedMessage)
{

    DataMessage expectedMessage = createDataMessage();
    expectedMessage.Command = Common::PluginProtocol::Commands::PLUGIN_SEND_EVENT;
    std::string payload = "EventXml";
    expectedMessage.Payload.push_back(payload);

    DataMessage actualMessage = m_messageBuilder->requestSendEventMessage(payload);

    EXPECT_PRED_FORMAT2( dataMessageSimilar, expectedMessage, actualMessage );
}

TEST_F(MessageBuilderTests, requestSendStatusMessageReturnsExpectedMessage)
{

    DataMessage expectedMessage = createDataMessage();
    expectedMessage.Command = Common::PluginProtocol::Commands::PLUGIN_SEND_STATUS;
    Common::PluginProtocol::StatusInfo statusInfo;
    statusInfo.statusXml = "StatusXml";
    statusInfo.statusWithoutXml = "StatusWithoutTimeStampXml";

    expectedMessage.Payload.push_back(statusInfo.statusXml);
    expectedMessage.Payload.push_back(statusInfo.statusWithoutXml);


    DataMessage actualMessage = m_messageBuilder->requestSendStatusMessage(statusInfo.statusXml, statusInfo.statusWithoutXml);

    EXPECT_PRED_FORMAT2( dataMessageSimilar, expectedMessage, actualMessage );
}


TEST_F(MessageBuilderTests, requestRegisterMessageReturnsExpectedMessage)
{

    DataMessage expectedMessage = createDataMessage();
    expectedMessage.Command = Common::PluginProtocol::Commands::PLUGIN_SEND_REGISTER;

    DataMessage actualMessage = m_messageBuilder->requestRegisterMessage();

    EXPECT_PRED_FORMAT2( dataMessageSimilar, expectedMessage, actualMessage );
}

TEST_F(MessageBuilderTests, requestCurrentPolicyMessageReturnsExpectedMessage)
{

    DataMessage expectedMessage = createDataMessage();
    expectedMessage.Command = Common::PluginProtocol::Commands::PLUGIN_QUERY_CURRENT_POLICY;

    DataMessage actualMessage = m_messageBuilder->requestCurrentPolicyMessage();

    EXPECT_PRED_FORMAT2( dataMessageSimilar, expectedMessage, actualMessage );
}

TEST_F(MessageBuilderTests, requestApplyPolicyMessageReturnsExpectedMessage)
{
    DataMessage expectedMessage = createDataMessage();
    expectedMessage.Command = Common::PluginProtocol::Commands::REQUEST_PLUGIN_APPLY_POLICY;
    std::string payload = "PolicyXml";
    expectedMessage.Payload.push_back(payload);

    DataMessage actualMessage = m_messageBuilder->requestApplyPolicyMessage(payload);

    EXPECT_PRED_FORMAT2( dataMessageSimilar, expectedMessage, actualMessage );
}

TEST_F(MessageBuilderTests, requestDoActionMessageReturnsExpectedMessage)
{
    DataMessage expectedMessage = createDataMessage();
    expectedMessage.Command = Common::PluginProtocol::Commands::REQUEST_PLUGIN_DO_ACTION;
    std::string payload = "ActionXml";
    expectedMessage.Payload.push_back(payload);

    DataMessage actualMessage = m_messageBuilder->requestDoActionMessage(payload);

    EXPECT_PRED_FORMAT2( dataMessageSimilar, expectedMessage, actualMessage );
}

TEST_F(MessageBuilderTests, requestRequestPluginStatusMessageReturnsExpectedMessage)
{

    DataMessage expectedMessage = createDataMessage();
    expectedMessage.Command = Common::PluginProtocol::Commands::REQUEST_PLUGIN_STATUS;

    DataMessage actualMessage = m_messageBuilder->requestRequestPluginStatusMessage();

    EXPECT_PRED_FORMAT2( dataMessageSimilar, expectedMessage, actualMessage );
}

TEST_F(MessageBuilderTests, requestRequestTelemetryMessageReturnsExpectedMessage)
{

    DataMessage expectedMessage = createDataMessage();
    expectedMessage.Command = Common::PluginProtocol::Commands::REQUEST_PLUGIN_TELEMETRY;

    DataMessage actualMessage = m_messageBuilder->requestRequestTelemetryMessage();

    EXPECT_PRED_FORMAT2( dataMessageSimilar, expectedMessage, actualMessage );
}


TEST_F(MessageBuilderTests, replyAckMessageReturnsExpectedMessage)
{
    DataMessage expectedMessage = createDataMessage();
    expectedMessage.Command = Common::PluginProtocol::Commands::REQUEST_PLUGIN_TELEMETRY;

    DataMessage actualMessage = m_messageBuilder->replyAckMessage(expectedMessage);

    expectedMessage.Payload.push_back("ACK");

    EXPECT_PRED_FORMAT2( dataMessageSimilar, expectedMessage, actualMessage );
}

TEST_F(MessageBuilderTests, replySetErrorIfEmptyReturnsExpectedMessageWhenErrorIsEmpty)
{
    DataMessage expectedMessage = createDataMessage();
    expectedMessage.Command = Common::PluginProtocol::Commands::REQUEST_PLUGIN_TELEMETRY;
    std::string errorMessage = "Something went wrong";

    DataMessage actualMessage = m_messageBuilder->replySetErrorIfEmpty(expectedMessage, errorMessage);

    expectedMessage.Error = errorMessage;

    EXPECT_PRED_FORMAT2( dataMessageSimilar, expectedMessage, actualMessage );
}

TEST_F(MessageBuilderTests, replySetErrorIfEmptyReturnsExpectedMessageWhenErrorNotEmpty)
{
    DataMessage expectedMessage = createDataMessage();
    expectedMessage.Command = Common::PluginProtocol::Commands::REQUEST_PLUGIN_TELEMETRY;
    std::string errorMessage = "Something went wrong";
    expectedMessage.Error = errorMessage;

    DataMessage actualMessage = m_messageBuilder->replySetErrorIfEmpty(expectedMessage, "New Error");

    EXPECT_PRED_FORMAT2( dataMessageSimilar, expectedMessage, actualMessage );
}

TEST_F(MessageBuilderTests, replyCurrentPolicyReturnsExpectedMessage)
{
    DataMessage expectedMessage = createDataMessage();
    expectedMessage.Command = Common::PluginProtocol::Commands::PLUGIN_QUERY_CURRENT_POLICY;

    std::string payload = "PolicyXml";
    expectedMessage.Payload.push_back(payload);

    DataMessage actualMessage = m_messageBuilder->replyCurrentPolicy(expectedMessage, payload);

    EXPECT_PRED_FORMAT2( dataMessageSimilar, expectedMessage, actualMessage );
}

TEST_F(MessageBuilderTests, replyTelemetryReturnsExpectedMessage)
{
    DataMessage expectedMessage = createDataMessage();
    expectedMessage.Command = Common::PluginProtocol::Commands::REQUEST_PLUGIN_TELEMETRY;

    std::string payload = "TelemetryXml";
    expectedMessage.Payload.push_back(payload);

    DataMessage actualMessage = m_messageBuilder->replyTelemetry(expectedMessage, payload);

    EXPECT_PRED_FORMAT2( dataMessageSimilar, expectedMessage, actualMessage );
}

TEST_F(MessageBuilderTests, replyStatusReturnsExpectedMessage)
{
    DataMessage expectedMessage = createDataMessage();
    expectedMessage.Command = Common::PluginProtocol::Commands::REQUEST_PLUGIN_STATUS;

    Common::PluginProtocol::StatusInfo statusInfo;
    statusInfo.statusXml = "StatusXml";
    statusInfo.statusWithoutXml = "StatusWithoutTimeStampXml";

    expectedMessage.Payload.push_back(statusInfo.statusXml);
    expectedMessage.Payload.push_back(statusInfo.statusWithoutXml);

    DataMessage actualMessage = m_messageBuilder->replyStatus(expectedMessage, statusInfo);

    EXPECT_PRED_FORMAT2( dataMessageSimilar, expectedMessage, actualMessage );
}

TEST_F(MessageBuilderTests, requestExtractEventReturnsExpectedMessage)
{
    DataMessage message = createDataMessage();
    message.Command = Common::PluginProtocol::Commands::PLUGIN_SEND_EVENT;

    std::string expectedResult = "EventXml";

    message.Payload.push_back(expectedResult);

    std::string actualResult =  m_messageBuilder->requestExtractEvent(message);

    EXPECT_EQ( expectedResult, actualResult );
}

TEST_F(MessageBuilderTests, requestExtractStatusReturnsExpectedMessage)
{
    DataMessage message = createDataMessage();
    message.Command = Common::PluginProtocol::Commands::REQUEST_PLUGIN_STATUS;

    Common::PluginProtocol::StatusInfo expectedStatusInfo;
    expectedStatusInfo.statusXml = "StatusXml";
    expectedStatusInfo.statusWithoutXml = "StatusWithoutTimeStampXml";

    message.Payload.push_back(expectedStatusInfo.statusXml);
    message.Payload.push_back(expectedStatusInfo.statusWithoutXml);

    Common::PluginProtocol::StatusInfo actualStatusInfo =  m_messageBuilder->requestExtractStatus(message);

    EXPECT_EQ( expectedStatusInfo.statusXml, actualStatusInfo.statusXml );
    EXPECT_EQ( expectedStatusInfo.statusWithoutXml, actualStatusInfo.statusWithoutXml );
}

TEST_F(MessageBuilderTests, requestExtractPolicyReturnsExpectedMessage)
{
    DataMessage message = createDataMessage();
    message.Command = Common::PluginProtocol::Commands::REQUEST_PLUGIN_APPLY_POLICY;

    std::string expectedResult = "PolicyXml";

    message.Payload.push_back(expectedResult);

    std::string actualResult =  m_messageBuilder->requestExtractPolicy(message);

    EXPECT_EQ( expectedResult, actualResult );
}

TEST_F(MessageBuilderTests, requestExtractActionReturnsExpectedMessage)
{
    DataMessage message = createDataMessage();
    message.Command = Common::PluginProtocol::Commands::REQUEST_PLUGIN_DO_ACTION;

    std::string expectedResult = "ActionXml";

    message.Payload.push_back(expectedResult);

    std::string actualResult =  m_messageBuilder->requestExtractAction(message);

    EXPECT_EQ( expectedResult, actualResult );
}

TEST_F(MessageBuilderTests, requestExtractPluginNameReturnsExpectedMessage)
{
    DataMessage expectedMessage = createDataMessage();
    expectedMessage.Command = Common::PluginProtocol::Commands::PLUGIN_SEND_REGISTER;

    std::string expectedResult = "ActionXml";

    expectedMessage.Payload.push_back(expectedResult);

    std::string actualResult =  m_messageBuilder->requestExtractPluginName(expectedMessage);

    EXPECT_EQ( expectedMessage.ApplicationId, actualResult );
}

TEST_F(MessageBuilderTests, replyExtractCurrentPolicyReturnsExpectedMessage)
{
    DataMessage message = createDataMessage();
    message.Command = Common::PluginProtocol::Commands::PLUGIN_QUERY_CURRENT_POLICY;

    std::string expectedResult = "PolicyXml";

    message.Payload.push_back(expectedResult);

    std::string actualResult =  m_messageBuilder->replyExtractCurrentPolicy(message);

    EXPECT_EQ( expectedResult, actualResult );
}

TEST_F(MessageBuilderTests, replyExtractTelemetryReturnsExpectedMessage)
{
    DataMessage message = createDataMessage();
    message.Command = Common::PluginProtocol::Commands::REQUEST_PLUGIN_TELEMETRY;

    std::string expectedResult = "TelemetryXml";

    message.Payload.push_back(expectedResult);

    std::string actualResult =  m_messageBuilder->replyExtractTelemetry(message);

    EXPECT_EQ( expectedResult, actualResult );
}

TEST_F(MessageBuilderTests, hasAckReturnsTrue)
{
    DataMessage message = createDataMessage();
    message.Command = Common::PluginProtocol::Commands::PLUGIN_SEND_EVENT;

    std::string expectedResult = "ACK";

    message.Payload.push_back(expectedResult);

    EXPECT_TRUE( m_messageBuilder->hasAck(message) );
}

