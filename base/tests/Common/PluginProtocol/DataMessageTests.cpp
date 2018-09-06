/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#include <gtest/gtest.h>
#include "gmock/gmock-matchers.h"
#include <Common/PluginProtocol/DataMessage.h>
#include <Common/Logging/ConsoleLoggingSetup.h>

using DataMessage = Common::PluginProtocol::DataMessage;

class DataMessageTests : public ::testing::Test
{
    Common::Logging::ConsoleLoggingSetup m_consoleLogging;
public:


    void SetUp() override
    {

    }

    void TearDown() override
    {

    }
};

TEST_F(DataMessageTests, DataMessageCanBeCreatedSuccessfully) //NOLINT
{
    std::string protocolVersion = "ProtocolVersion";
    std::string messageId = "MessageId";
    std::string applicationId = "ApplicationId";
    std::string errorMessage  = "errorMessage";
    std::vector<std::string> payload;
    payload.emplace_back("payloadItem");
    Common::PluginProtocol::Commands command = Common::PluginProtocol::Commands::PLUGIN_SEND_STATUS;

    DataMessage dataMessage;
    dataMessage.ProtocolVersion = protocolVersion;
    dataMessage.ApplicationId = applicationId;
    dataMessage.MessageId = messageId;
    dataMessage.Error = errorMessage;
    dataMessage.Payload = payload;
    dataMessage.Command = command;


    EXPECT_EQ(dataMessage.ProtocolVersion, protocolVersion);
    EXPECT_EQ(dataMessage.ApplicationId, applicationId);
    EXPECT_EQ(dataMessage.MessageId, messageId);
    EXPECT_EQ(dataMessage.Error,errorMessage);
    EXPECT_EQ(dataMessage.Payload, payload);
    EXPECT_EQ(dataMessage.Command,command);
}

TEST_F(DataMessageTests, DataMessageCommandPLUGIN_SEND_EVENTCanBeConvertedCorrectly) //NOLINT
{
    Common::PluginProtocol::Commands command = Common::PluginProtocol::Commands::PLUGIN_SEND_EVENT;
    std::string commandString = "SendEvent";
    EXPECT_EQ(Common::PluginProtocol::SerializeCommand(command), commandString);
    EXPECT_EQ(Common::PluginProtocol::DeserializeCommand(commandString), command);
}

TEST_F(DataMessageTests, DataMessageCommandPLUGIN_SEND_STATUSCanBeConvertedCorrectly) //NOLINT
{
    Common::PluginProtocol::Commands command = Common::PluginProtocol::Commands::PLUGIN_SEND_STATUS;
    std::string commandString = "SendStatus";
    EXPECT_EQ(Common::PluginProtocol::SerializeCommand(command), commandString);
    EXPECT_EQ(Common::PluginProtocol::DeserializeCommand(commandString), command);
}

TEST_F(DataMessageTests, DataMessageCommandPLUGIN_SEND_REGISTERCanBeConvertedCorrectly) //NOLINT
{
    Common::PluginProtocol::Commands command = Common::PluginProtocol::Commands::PLUGIN_SEND_REGISTER;
    std::string commandString = "Registration";
    EXPECT_EQ(Common::PluginProtocol::SerializeCommand(command), commandString);
    EXPECT_EQ(Common::PluginProtocol::DeserializeCommand(commandString), command);
}

TEST_F(DataMessageTests, DataMessageCommandREQUEST_PLUGIN_APPLY_POLICYCanBeConvertedCorrectly) //NOLINT
{
    Common::PluginProtocol::Commands command = Common::PluginProtocol::Commands::REQUEST_PLUGIN_APPLY_POLICY;
    std::string commandString = "ApplyPolicy";
    EXPECT_EQ(Common::PluginProtocol::SerializeCommand(command), commandString);
    EXPECT_EQ(Common::PluginProtocol::DeserializeCommand(commandString), command);
}

TEST_F(DataMessageTests, DataMessageCommandREQUEST_PLUGIN_DO_ACTIONCanBeConvertedCorrectly) //NOLINT
{
    Common::PluginProtocol::Commands command = Common::PluginProtocol::Commands::REQUEST_PLUGIN_DO_ACTION;
    std::string commandString = "DoAction";
    EXPECT_EQ(Common::PluginProtocol::SerializeCommand(command), commandString);
    EXPECT_EQ(Common::PluginProtocol::DeserializeCommand(commandString), command);
}

TEST_F(DataMessageTests, DataMessageCommandREQUEST_PLUGIN_STATUSCanBeConvertedCorrectly) //NOLINT
{
    Common::PluginProtocol::Commands command = Common::PluginProtocol::Commands::REQUEST_PLUGIN_STATUS;
    std::string commandString = "RequestStatus";
    EXPECT_EQ(Common::PluginProtocol::SerializeCommand(command), commandString);
    EXPECT_EQ(Common::PluginProtocol::DeserializeCommand(commandString), command);
}

TEST_F(DataMessageTests, DataMessageCommandREQUEST_PLUGIN_TELEMETRYCanBeConvertedCorrectly) //NOLINT
{
    Common::PluginProtocol::Commands command = Common::PluginProtocol::Commands::REQUEST_PLUGIN_TELEMETRY;
    std::string commandString = "Telemetry";
    EXPECT_EQ(Common::PluginProtocol::SerializeCommand(command), commandString);
    EXPECT_EQ(Common::PluginProtocol::DeserializeCommand(commandString), command);
}

TEST_F(DataMessageTests, DataMessageCommandPLUGIN_QUERY_CURRENT_POLICYCanBeConvertedCorrectly) //NOLINT
{
    Common::PluginProtocol::Commands command = Common::PluginProtocol::Commands::PLUGIN_QUERY_CURRENT_POLICY;
    std::string commandString = "RequestCurrentPolicy";
    EXPECT_EQ(Common::PluginProtocol::SerializeCommand(command), commandString);
    EXPECT_EQ(Common::PluginProtocol::DeserializeCommand(commandString), command);
}

TEST_F(DataMessageTests, DataMessageCommandUNKNOWNCanBeConvertedCorrectly) //NOLINT
{
    Common::PluginProtocol::Commands command = Common::PluginProtocol::Commands::UNKNOWN;
    std::string commandString = "InvalidCommand";
    EXPECT_EQ(Common::PluginProtocol::SerializeCommand(command), commandString);
    EXPECT_EQ(Common::PluginProtocol::DeserializeCommand(commandString), command);
}
