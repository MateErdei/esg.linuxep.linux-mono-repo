/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gtest/gtest.h>
#include "gmock/gmock-matchers.h"
#include "Common/PluginApi/DataMessage.h"
#include "Common/PluginApiImpl/Protocol.h"
#include "Common/PluginApiImpl/ProtocolSerializerFactory.h"

using namespace Common::PluginApiImpl;
using DataMessage = Common::PluginApi::DataMessage;
class TestProtocol : public ::testing::Test
{
public:


    void SetUp() override
    {

    }
    void TearDown() override
    {

    }


    Common::PluginApi::DataMessage createDefaultMessage()
    {
        DataMessage message;

        message.ProtocolVersion = "v1";
        message.ApplicationId = "1";
        message.Command = Common::PluginApi::Commands::PLUGIN_SEND_EVENT;
        message.MessageId = "1";
        message.Payload = {"Hello"};

        return message;
    }

    std::string toString( Common::PluginApi::Commands command)
    {
        return Common::PluginApi::SerializeCommand(command); 
    }

};

TEST_F(TestProtocol, Serialise_ReturnsValidDataString)
{
    Protocol protocol;

    DataMessage message = createDefaultMessage();

    data_t serializedData = protocol.serialize(message);

    EXPECT_EQ(serializedData[0], message.ProtocolVersion);
    EXPECT_EQ(serializedData[1], message.ApplicationId);
    EXPECT_EQ(serializedData[2], toString(message.Command));
    EXPECT_EQ(serializedData[3], message.MessageId);
    EXPECT_EQ(serializedData[4], message.Payload[0]);

}

TEST_F(TestProtocol, Serialise_ReturnsValidDataStringWhenPayloadIsAnError)
{
    Protocol protocol;

    DataMessage message = createDefaultMessage();
    std::string error = "Some error";
    message.Error = error;

    data_t serializedData = protocol.serialize(message);

    EXPECT_EQ(serializedData[0], message.ProtocolVersion);
    EXPECT_EQ(serializedData[1], message.ApplicationId);
    EXPECT_EQ(serializedData[2], toString(message.Command));
    EXPECT_EQ(serializedData[3], message.MessageId);
    EXPECT_EQ(serializedData[4], "Error");
    EXPECT_EQ(serializedData[5], error);
}

TEST_F(TestProtocol, Serialise_ReturnsValidDataStringWhenPayloadContainsMultipleItems)
{
    Protocol protocol;

    DataMessage message = createDefaultMessage();
    message.Payload.push_back("World");
    message.Payload.push_back("how");
    message.Payload.push_back("are");
    message.Payload.push_back("you");

    data_t serializedData = protocol.serialize(message);

    EXPECT_EQ(serializedData[0], message.ProtocolVersion);
    EXPECT_EQ(serializedData[1], message.ApplicationId);
    EXPECT_EQ(serializedData[2], toString(message.Command));
    EXPECT_EQ(serializedData[3], message.MessageId);

    // copy Payload into another vector to make it easy to compare.
    data_t payload(serializedData.begin()+4, serializedData.end());

    EXPECT_THAT(payload, message.Payload);
}

TEST_F(TestProtocol, Deserialise_ReturnsValidDataString)
{
    Protocol protocol;

    data_t expectedData = {"v1", "1", "SendEvent", "1", "Hello"};

    DataMessage message = protocol.deserialize(expectedData);

    EXPECT_EQ(message.ProtocolVersion, "v1");
    EXPECT_EQ(message.ApplicationId, expectedData[1]);
    EXPECT_EQ(toString(message.Command), expectedData[2]);
    EXPECT_EQ(message.MessageId, expectedData[3]);
    EXPECT_EQ(message.Payload[0], expectedData[4]);
}

TEST_F(TestProtocol, Deserialise_ReturnsValidDataStringWhenPayloadIsAnError)
{
    Protocol protocol;

    data_t expectedData = {"v1", "1", "SendEvent", "1", "Error", "Some error"};

    DataMessage message = protocol.deserialize(expectedData);

    EXPECT_EQ(message.ProtocolVersion, "v1");
    EXPECT_EQ(message.ApplicationId, expectedData[1]);
    EXPECT_EQ(toString(message.Command), expectedData[2]);
    EXPECT_EQ(message.MessageId, expectedData[3]);
    EXPECT_EQ("Error", expectedData[4]);
    EXPECT_EQ(message.Error, expectedData[5]);
}

TEST_F(TestProtocol, Deserialise_ReturnsValidDataStringWhenPayloadContainsMultipleItems)
{
    Protocol protocol;

    data_t expectedData = {"v1", "1", "SendEvent", "1", "Hello", "World", "how", "are", "you"};

    DataMessage message = protocol.deserialize(expectedData);

    EXPECT_EQ(message.ProtocolVersion, "v1");
    EXPECT_EQ(message.ApplicationId, expectedData[1]);
    EXPECT_EQ(toString(message.Command), expectedData[2]);
    EXPECT_EQ(message.MessageId, expectedData[3]);

    // copy Payload into another vector to make it easy to compare.
    data_t payload(expectedData.begin()+4, expectedData.end());

    EXPECT_THAT(payload, message.Payload);
}

TEST_F(TestProtocol, Deserialise_ReturnsErrorShownInMessageWhenNoDataProvided)
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


TEST_F(TestProtocol, Deserialise_ReturnsErrorShownInMessageWhenNoPayloadProvided)
{
    Protocol protocol;

    data_t expectedData = {"v1", "app1", "Test", "1"};

    DataMessage message = protocol.deserialize(expectedData);

    EXPECT_EQ(message.ProtocolVersion, "v1");
    EXPECT_EQ(message.ApplicationId, "app1");
    EXPECT_EQ(toString(message.Command), "InvalidCommand");
    EXPECT_EQ(message.MessageId, "1");
    EXPECT_THAT(message.Payload.size(), 0);
    EXPECT_EQ(message.Error, "Invalid request");
}

TEST_F(TestProtocol, Deserialise_ReturnsErrorShownInMessageWhenWrongProtocolProvided)
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
