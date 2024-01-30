// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "Common/Logging/ConsoleLoggingSetup.h"
#include "Common/PluginApi/ApiException.h"
#include "Common/PluginProtocol/DataMessage.h"
#include "Common/PluginProtocol/Protocol.h"
#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>

using namespace Common::PluginProtocol;
using DataMessage = Common::PluginProtocol::DataMessage;
class TestProtocol : public ::testing::Test
{
    Common::Logging::ConsoleLoggingSetup m_consoleLogging;

public:
    void SetUp() override {}
    void TearDown() override {}

    Common::PluginProtocol::DataMessage createDefaultMessage()
    {
        DataMessage message;

        message.m_applicationId = "1";
        message.m_pluginName = "p2";
        message.m_command = Common::PluginProtocol::Commands::PLUGIN_SEND_EVENT;
        message.m_payload = { "Hello" };

        return message;
    }

    std::string toString(Common::PluginProtocol::Commands command)
    {
        return Common::PluginProtocol::ConvertCommandEnumToString(command);
    }
};

TEST_F(TestProtocol, Serialise_ReturnsValidDataString)
{
    Protocol protocol;

    DataMessage message = createDefaultMessage();

    data_t serializedData = protocol.serialize(message);

    EXPECT_EQ(serializedData.size(), 1);
}

TEST_F(TestProtocol, Serialise_ReturnsValidDataStringWhenPayloadIsAnError)
{
    Protocol protocol;

    DataMessage message = createDefaultMessage();
    std::string error = "Some error";
    message.m_error = error;

    data_t serializedData = protocol.serialize(message);

    EXPECT_EQ(serializedData.size(), 1);
}

TEST_F(TestProtocol, Serialise_DoesNotThrowIfMessageDoesNotContainApplicationId)
{
    Protocol protocol;

    DataMessage message = { "", "1", Common::PluginProtocol::Commands::PLUGIN_SEND_EVENT, "", false, {} };

    ASSERT_NO_THROW(protocol.serialize(message));
}

TEST_F(TestProtocol, Serialise_ThrowsIfMessageDoesNotContainPluginName)
{
    Protocol protocol;

    DataMessage message = { "v1", "", Common::PluginProtocol::Commands::PLUGIN_SEND_EVENT, "", false, {} };

    EXPECT_THROW(protocol.serialize(message), Common::PluginApi::ApiException);
}

TEST_F(TestProtocol, Serialise_ThrowsIfMessageContainsUnsetCommand)
{
    Protocol protocol;

    DataMessage message = { "v1", "1", Common::PluginProtocol::Commands::UNSET, "", false, {} };

    EXPECT_THROW(protocol.serialize(message), Common::PluginApi::ApiException);
}

TEST_F(TestProtocol, SerialiseAndDeserialise_ReturnsValidDataString)
{
    Protocol protocol;

    DataMessage sendMessage = { "v1", "1",   Common::PluginProtocol::Commands::REQUEST_PLUGIN_DO_ACTION,
                                "",   false, { "Hello" } };

    data_t serialisedMessage = protocol.serialize(sendMessage);
    DataMessage receivedMessage = protocol.deserialize(serialisedMessage);

    EXPECT_EQ(receivedMessage.m_applicationId, sendMessage.m_applicationId);
    EXPECT_EQ(receivedMessage.m_pluginName, sendMessage.m_pluginName);
    EXPECT_EQ(receivedMessage.m_command, sendMessage.m_command);
    EXPECT_EQ(receivedMessage.m_error, sendMessage.m_error);
    EXPECT_EQ(receivedMessage.m_acknowledge, sendMessage.m_acknowledge);
    EXPECT_EQ(receivedMessage.m_payload[0], sendMessage.m_payload[0]);
}

TEST_F(TestProtocol, SerialiseAndDeserialise_ReturnsValidDataStringWhenPayloadIsAnError)
{
    Protocol protocol;

    DataMessage sendMessage = { "v1",         "1",  Common::PluginProtocol::Commands::PLUGIN_SEND_STATUS,
                                "Some error", true, { "Hello", "This", "Is", "The", "Payload" } };

    data_t serialisedMessage = protocol.serialize(sendMessage);
    DataMessage receivedMessage = protocol.deserialize(serialisedMessage);

    EXPECT_EQ(receivedMessage.m_applicationId, sendMessage.m_applicationId);
    EXPECT_EQ(receivedMessage.m_pluginName, sendMessage.m_pluginName);
    EXPECT_EQ(receivedMessage.m_command, sendMessage.m_command);
    EXPECT_EQ(receivedMessage.m_error, sendMessage.m_error);
    // On sent error the acknowledgement is ignored
    EXPECT_EQ(receivedMessage.m_acknowledge, false);
}

TEST_F(TestProtocol, SerialiseAndDeserialise_ReturnsValidDataStringWhenPayloadContainsOneItems)
{
    Protocol protocol;

    DataMessage sendMessage = { "v1", "1",   Common::PluginProtocol::Commands::PLUGIN_QUERY_CURRENT_POLICY,
                                "",   false, { "Hello" } };
    data_t serialisedMessage = protocol.serialize(sendMessage);
    DataMessage receivedMessage = protocol.deserialize(serialisedMessage);

    EXPECT_EQ(receivedMessage.m_applicationId, sendMessage.m_applicationId);
    EXPECT_EQ(receivedMessage.m_pluginName, sendMessage.m_pluginName);
    EXPECT_EQ(receivedMessage.m_command, sendMessage.m_command);
    EXPECT_EQ(receivedMessage.m_error, sendMessage.m_error);
    EXPECT_EQ(receivedMessage.m_acknowledge, false);
    EXPECT_EQ(receivedMessage.m_payload.size(), 1);
    EXPECT_EQ(receivedMessage.m_payload[0], "Hello");
}

TEST_F(TestProtocol, SerialiseAndDeserialise_ReturnsValidDataStringWhenPayloadContainsMultipleItems)
{
    Protocol protocol;

    DataMessage sendMessage = { "v1", "1",   Common::PluginProtocol::Commands::PLUGIN_SEND_REGISTER,
                                "",   false, { "Hello", "This", "Is", "The", "Payload" } };
    data_t serialisedMessage = protocol.serialize(sendMessage);
    DataMessage receivedMessage = protocol.deserialize(serialisedMessage);

    EXPECT_EQ(receivedMessage.m_applicationId, sendMessage.m_applicationId);
    EXPECT_EQ(receivedMessage.m_pluginName, sendMessage.m_pluginName);
    EXPECT_EQ(receivedMessage.m_command, sendMessage.m_command);
    EXPECT_EQ(receivedMessage.m_error, sendMessage.m_error);
    EXPECT_EQ(receivedMessage.m_acknowledge, false);
    EXPECT_EQ(receivedMessage.m_payload.size(), 5);
    EXPECT_EQ(receivedMessage.m_payload[0], "Hello");
    EXPECT_EQ(receivedMessage.m_payload[1], "This");
    EXPECT_EQ(receivedMessage.m_payload[2], "Is");
    EXPECT_EQ(receivedMessage.m_payload[3], "The");
    EXPECT_EQ(receivedMessage.m_payload[4], "Payload");
}

TEST_F(TestProtocol, SerialiseAndDeserialise_ThrowsWhenPassedMessageWithNoData)
{
    Protocol protocol;
    data_t serialisedMessage;
    try
    {
        protocol.deserialize(serialisedMessage);
        FAIL() << "Protocol::deserialize failed to throw!";
    }
    catch (const Common::PluginApi::ApiException& apiException)
    {
        std::string errorMessage(apiException.what());
        EXPECT_EQ(errorMessage, "Bad formed message: Protobuf parse error");
    }
    catch (...)
    {
        FAIL() << "Unexpected Exception";
    }
}

TEST_F(TestProtocol, SerialiseAndDeserialise_ThrowsWhenGarbageDataIsPassed)
{
    Protocol protocol;
    try
    {
        data_t serialisedMessage = { "ThisIsAGarbageString" };
        protocol.deserialize(serialisedMessage);
        FAIL() << "Protocol::deserialize failed to throw!";
    }
    catch (const Common::PluginApi::ApiException& apiException)
    {
        std::string errorMessage(apiException.what());
        EXPECT_EQ(errorMessage, "Bad formed message: Protobuf parse error");
    }
    catch (...)
    {
        FAIL() << "Unexpected Exception";
    }
}

TEST_F(TestProtocol, SerialiseAndDeserialise_ReturnsErrorShownInMessageWhenDataIsMissingRequiredField)
{
    Protocol protocol;

    DataMessage sendMessage = { "v1", "PluginName", Common::PluginProtocol::Commands::REQUEST_PLUGIN_APPLY_POLICY,
                                "",   true,         { "Hello", "This", "Is", "The", "Payload" } };
    data_t serialisedMessage = protocol.serialize(sendMessage);
    // Remove the PluginName from the protobuf message
    serialisedMessage[0].erase(serialisedMessage[0].find("\n\nPluginName"), 12);
    DataMessage receivedMessage = protocol.deserialize(serialisedMessage);

    EXPECT_EQ(receivedMessage.m_applicationId, "");
    EXPECT_EQ(receivedMessage.m_pluginName, "");
    EXPECT_EQ(receivedMessage.m_command, Common::PluginProtocol::Commands::UNSET);
    EXPECT_EQ(receivedMessage.m_error, "Bad formed message, missing required fields : PluginName ");
    // On deserialise error payload and acknowledgement is ignored
    EXPECT_EQ(receivedMessage.m_acknowledge, false);
    EXPECT_THAT(receivedMessage.m_payload.size(), 0);
}

TEST_F(TestProtocol, SerialiseAndDeserialise_ReturnsErrorShownInMessageWhenCommandIsUnknown)
{
    Protocol protocol;

    DataMessage sendMessage = { "v1", "100", Common::PluginProtocol::Commands::PLUGIN_SEND_EVENT,
                                "",   false, { "Hello", "This", "Is", "The", "Payload" } };
    data_t serialisedMessage = protocol.serialize(sendMessage);
    // Change the command to unknown in the protobuf message
    serialisedMessage[0].replace(serialisedMessage[0].find(char(1)), 1, 1, '\t');
    DataMessage receivedMessage = protocol.deserialize(serialisedMessage);

    EXPECT_EQ(receivedMessage.m_applicationId, sendMessage.m_applicationId);
    EXPECT_EQ(receivedMessage.m_pluginName, sendMessage.m_pluginName);
    EXPECT_EQ(receivedMessage.m_command, Common::PluginProtocol::Commands::UNKNOWN);
    EXPECT_EQ(receivedMessage.m_error, "Invalid request");
    EXPECT_EQ(receivedMessage.m_acknowledge, sendMessage.m_acknowledge);
}

TEST_F(TestProtocol, SerialiseAndDeserialise_ReceivedMessageContainsAcknowledgeWhenSent)
{
    Protocol protocol;

    DataMessage sendMessage = { "v1", "1",  Common::PluginProtocol::Commands::REQUEST_PLUGIN_STATUS,
                                "",   true, { "Hello", "This", "Is", "The", "Payload" } };
    data_t serialisedMessage = protocol.serialize(sendMessage);
    DataMessage receivedMessage = protocol.deserialize(serialisedMessage);

    EXPECT_EQ(receivedMessage.m_applicationId, sendMessage.m_applicationId);
    EXPECT_EQ(receivedMessage.m_pluginName, sendMessage.m_pluginName);
    EXPECT_EQ(receivedMessage.m_command, sendMessage.m_command);
    EXPECT_EQ(receivedMessage.m_error, sendMessage.m_error);
    EXPECT_EQ(receivedMessage.m_acknowledge, sendMessage.m_acknowledge);
}

TEST_F(TestProtocol, SerialiseAndDeserialise_NoPayloadReceivedWhenNoneSent)
{
    Protocol protocol;

    DataMessage sendMessage = { "v1", "1", Common::PluginProtocol::Commands::REQUEST_PLUGIN_TELEMETRY, "", false, {} };
    data_t serialisedMessage = protocol.serialize(sendMessage);
    DataMessage receivedMessage = protocol.deserialize(serialisedMessage);

    EXPECT_EQ(receivedMessage.m_applicationId, sendMessage.m_applicationId);
    EXPECT_EQ(receivedMessage.m_pluginName, sendMessage.m_pluginName);
    EXPECT_EQ(receivedMessage.m_command, sendMessage.m_command);
    EXPECT_EQ(receivedMessage.m_error, sendMessage.m_error);
    EXPECT_EQ(receivedMessage.m_acknowledge, sendMessage.m_acknowledge);
    EXPECT_THAT(receivedMessage.m_payload.size(), 0);
}

TEST_F(TestProtocol, SerialiseAndDeserialise_SerialisingAndDeserialisingUnknownCommandSetsAnError)
{
    Protocol protocol;
    DataMessage sendMessage = { "v1", "1", static_cast<Common::PluginProtocol::Commands>(20), "", false, {} };
    data_t serialisedMessage = protocol.serialize(sendMessage);
    DataMessage receivedMessage = protocol.deserialize(serialisedMessage);

    EXPECT_EQ(receivedMessage.m_applicationId, sendMessage.m_applicationId);
    EXPECT_EQ(receivedMessage.m_pluginName, sendMessage.m_pluginName);
    EXPECT_EQ(receivedMessage.m_command, Common::PluginProtocol::Commands::UNKNOWN);
    EXPECT_EQ(receivedMessage.m_error, "Invalid request");
    EXPECT_EQ(receivedMessage.m_acknowledge, sendMessage.m_acknowledge);
    EXPECT_THAT(receivedMessage.m_payload.size(), 0);
}
