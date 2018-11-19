/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/PluginProtocol/DataMessage.h>
#include <Common/PluginProtocol/Protocol.h>
#include <Common/Logging/ConsoleLoggingSetup.h>
#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>

using namespace Common::PluginProtocol;
using DataMessage = Common::PluginProtocol::DataMessage;
class TestProtocol : public ::testing::Test
{
    Common::Logging::ConsoleLoggingSetup m_consoleLogging;
public:


    void SetUp() override
    {

    }
    void TearDown() override
    {

    }


    Common::PluginProtocol::DataMessage createDefaultMessage()
    {
        DataMessage message;

        message.ApplicationId = "1";
        message.PluginName = "p2";
        message.Command = Common::PluginProtocol::Commands::PLUGIN_SEND_EVENT;
        message.MessageId = "m1";
        message.Payload = {"Hello"};

        return message;
    }

    std::string toString( Common::PluginProtocol::Commands command)
    {
        return Common::PluginProtocol::ConvertCommandEnumToString(command);
    }

};

TEST_F(TestProtocol, Serialise_ReturnsValidDataString)  // NOLINT
{
    Protocol protocol;

    DataMessage message = createDefaultMessage();

    data_t serializedData = protocol.serialize(message);

    EXPECT_EQ(serializedData[1], message.ApplicationId);
    EXPECT_EQ(serializedData[2], message.PluginName);
    EXPECT_EQ(serializedData[3], toString(message.Command));
    EXPECT_EQ(serializedData[4], message.MessageId);
    EXPECT_EQ(serializedData[5], message.Payload[0]);

}

TEST_F(TestProtocol, Serialise_ReturnsValidDataStringWhenPayloadIsAnError)  // NOLINT
{
    Protocol protocol;

    DataMessage message = createDefaultMessage();
    std::string error = "Some error";
    message.Error = error;

    data_t serializedData = protocol.serialize(message);

    EXPECT_EQ(serializedData[1], message.ApplicationId);
    EXPECT_EQ(serializedData[3], toString(message.Command));
    EXPECT_EQ(serializedData[4], message.MessageId);
    EXPECT_EQ(serializedData[5], "Error");
    EXPECT_EQ(serializedData[6], error);
}

TEST_F(TestProtocol, Serialise_ReturnsValidDataStringWhenPayloadContainsMultipleItems)  // NOLINT
{
    Protocol protocol;

    DataMessage message = createDefaultMessage();
    message.Payload.push_back("World");
    message.Payload.push_back("how");
    message.Payload.push_back("are");
    message.Payload.push_back("you");

    data_t serializedData = protocol.serialize(message);

    EXPECT_EQ(serializedData[1], message.ApplicationId);
    EXPECT_EQ(serializedData[3], toString(message.Command));
    EXPECT_EQ(serializedData[4], message.MessageId);

    // copy Payload into another vector to make it easy to compare.
    data_t payload(serializedData.begin() + 5, serializedData.end());

    EXPECT_THAT(payload, message.Payload);
}

TEST_F(TestProtocol, Deserialise_ReturnsValidDataString)  // NOLINT
{
    Protocol protocol;

    data_t expectedData = {"v1", "1", "p1", "SendEvent", "1", "Hello"};

    DataMessage message = protocol.deserialize(expectedData);

    EXPECT_EQ(message.ApplicationId, expectedData[1]);
    EXPECT_EQ(message.PluginName, expectedData[2]);
    EXPECT_EQ(toString(message.Command), expectedData[3]);
    EXPECT_EQ(message.MessageId, expectedData[4]);
    EXPECT_EQ(message.Payload[0], expectedData[5]);
}

TEST_F(TestProtocol, Deserialise_ReturnsValidDataStringWhenPayloadIsAnError)  // NOLINT
{
    Protocol protocol;

    data_t expectedData = {"v1", "1", "p1", "SendEvent", "1", "Error", "Some error"};

    DataMessage message = protocol.deserialize(expectedData);

    EXPECT_EQ(message.ApplicationId, expectedData[1]);
    EXPECT_EQ(message.PluginName, expectedData[2]);
    EXPECT_EQ(toString(message.Command), expectedData[3]);
    EXPECT_EQ(message.MessageId, expectedData[4]);
    EXPECT_EQ("Error", expectedData[5]);
    EXPECT_EQ(message.Error, expectedData[6]);
}

TEST_F(TestProtocol, Deserialise_ReturnsValidDataStringWhenPayloadContainsMultipleItems)  // NOLINT
{
    Protocol protocol;

    data_t expectedData = {"v1", "1", "p1", "SendEvent", "1", "Hello", "World", "how", "are", "you"};

    DataMessage message = protocol.deserialize(expectedData);

    EXPECT_EQ(message.ApplicationId, expectedData[1]);
    EXPECT_EQ(message.PluginName, expectedData[2]);
    EXPECT_EQ(toString(message.Command), expectedData[3]);
    EXPECT_EQ(message.MessageId, expectedData[4]);

    // copy Payload into another vector to make it easy to compare.
    data_t payload(expectedData.begin() + 5, expectedData.end());

    EXPECT_THAT(payload, message.Payload);
}

TEST_F(TestProtocol, Deserialise_ReturnsErrorShownInMessageWhenNoDataProvided)  // NOLINT
{
    Protocol protocol;

    data_t expectedData;

    DataMessage message = protocol.deserialize(expectedData);

    EXPECT_EQ(message.ApplicationId, "");
    EXPECT_EQ(toString(message.Command), "InvalidCommand");
    EXPECT_EQ(message.MessageId, "");
    EXPECT_THAT(message.Payload.size(), 0);
    EXPECT_EQ(message.Error, "Bad formed message");
}


TEST_F(TestProtocol, Deserialise_ReturnsErrorShownInMessageWhenNoPayloadProvided)  // NOLINT
{
    Protocol protocol;

    data_t expectedData = {"v1", "app1", "plugin", "Test", "mid1"};

    DataMessage message = protocol.deserialize(expectedData);

    EXPECT_EQ(message.ApplicationId, "app1");
    EXPECT_EQ(message.PluginName, "plugin");
    EXPECT_EQ(toString(message.Command), "InvalidCommand");
    EXPECT_EQ(message.MessageId, "mid1");
    EXPECT_THAT(message.Payload.size(), 0);
    EXPECT_EQ(message.Error, "Invalid request");
}

TEST_F(TestProtocol, Deserialise_ReturnsErrorShownInMessageWhenWrongProtocolProvided)  // NOLINT
{
    Protocol protocol;

    data_t expectedData = {"vNotKnown", "app1", "SendEvent", "1", "Hello", "World", "how", "are", "you"};

    DataMessage message = protocol.deserialize(expectedData);

    EXPECT_EQ(message.ApplicationId, "");
    EXPECT_EQ(toString(message.Command), "InvalidCommand");
    EXPECT_EQ(message.MessageId, "");
    EXPECT_THAT(message.Payload.size(), 0);
    EXPECT_THAT(message.Error, testing::HasSubstr("Protocol not supported"));
}
