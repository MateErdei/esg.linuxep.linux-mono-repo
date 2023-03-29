// Copyright 2023 Sophos Limited. All rights reserved.

#include "ProcessImpl/ProcessImpl.h"
#include "ResponseActions/ResponseActionsImpl/InvalidCommandFormat.h"
#include "ResponseActions/ResponseActionsImpl/RunCommandAction.h"
#include "modules/Common/ZipUtilities/ZipUtils.h"
#include "tests/Common/Helpers/MemoryAppender.h"
#include "tests/Common/Helpers/MockProcess.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

class RunCommandTests : public LogInitializedTests
{
};

using namespace ResponseActions::RACommon;

namespace
{
    std::string getDefaultCommandJsonString()
    {
        return R"({
            "type": "sophos.mgt.action.RunCommands",
            "commands": [
                "echo one",
                "echo two > /tmp/test.txt"
            ],
            "ignoreError": true,
            "timeout": 60,
            "expiration": 144444000000004
        })";
    }

    std::string getSingleCommandJsonString()
    {
        return R"({
            "type": "sophos.mgt.action.RunCommands",
            "commands": [
                "echo -n one"
            ],
            "ignoreError": true,
            "timeout": 60,
            "expiration": 144444000000004
        })";
    }

    std::string getCommandJsonStringWithWrongValueType()
    {
        return R"({
            "type": "sophos.mgt.action.RunCommands",
            "commands": [
                "echo one",
                "echo two > /tmp/test.txt"
            ],
            "ignoreError": true,
            "timeout": "60",
            "expiration": 144444000000004
        })";
    }

} // namespace

// to_json tests

TEST_F(RunCommandTests, SingleCommandResultToJson)
{

    ResponseActionsImpl::SingleCommandResult singleCommandResult;
    singleCommandResult.duration = 100000000;
    singleCommandResult.stdOut = "some output text";
    singleCommandResult.stdErr = "some error text";

    nlohmann::json result = singleCommandResult;
    EXPECT_EQ(result.dump(), R"({"duration":100000000,"exitCode":0,"stdErr":"some error text","stdOut":"some output text"})");
}

TEST_F(RunCommandTests, CommandResponseToJson)
{
    ResponseActionsImpl::CommandResponse commandResponse;
    commandResponse.duration = 100000000;
    commandResponse.result = ResponseResult::SUCCESS;
    ResponseActionsImpl::SingleCommandResult r1;
    r1.stdErr = "stderr";
    r1.stdOut = "stdout";
    r1.duration = 100;
    r1.exitCode = 123;
    commandResponse.commandResults = {r1};
    commandResponse.type = "type";
    commandResponse.startedAt = 123123;
    std::string  expected = R"({"commandResults":[{"duration":100,"exitCode":123,"stdErr":"stderr","stdOut":"stdout"}],"duration":100000000,"result":0,"startedAt":123123,"type":"type"})";
    nlohmann::json response = commandResponse;
    EXPECT_EQ(response.dump(), expected);
}


// parseCommandAction tests

TEST_F(RunCommandTests, testParseCommandActionSuccess)
{
    std::string commandJson = getDefaultCommandJsonString();
    ResponseActionsImpl::CommandRequest command =
        ResponseActionsImpl::RunCommandAction::parseCommandAction(commandJson);
    std::vector<std::string> expectedCommands = { { "echo one" }, { R"(echo two > /tmp/test.txt)" } };

    EXPECT_EQ(command.ignoreError, true);
    EXPECT_EQ(command.timeout, 60);
    EXPECT_EQ(command.expiration, 144444000000004);
    EXPECT_EQ(command.commands, expectedCommands);
}

TEST_F(RunCommandTests, testParseCommandHandlesEscapedCharsInCmd)
{
    std::string commandJson = R"({
            "type": "sophos.mgt.action.RunCommands",
            "commands": [
                "echo {\"number\":13, \"string\":\"a string and a quote \" }"
            ],
            "ignoreError": true,
            "timeout": 60,
            "expiration": 144444000000004
            })";
    auto command = ResponseActionsImpl::RunCommandAction::parseCommandAction(commandJson);
    std::vector<std::string> expectedCommands = {  { R"(echo {"number":13, "string":"a string and a quote " })" } };
    EXPECT_EQ(command.ignoreError, true);
    EXPECT_EQ(command.timeout, 60);
    EXPECT_EQ(command.expiration, 144444000000004);
    EXPECT_EQ(command.commands, expectedCommands);
}

TEST_F(RunCommandTests, testParseCommandFailsDueToMissingTypeRequiredField)
{
    std::string commandJson = R"({
        "commands": [
            "echo one",
            "echo two > /tmp/test.txt"
        ],
        "ignoreError": true,
        "timeout": 60,
        "expiration": 144444000000004
        })";
    EXPECT_THAT([&commandJson]() { ResponseActionsImpl::RunCommandAction::parseCommandAction(commandJson); },
                ThrowsMessage<ResponseActionsImpl::InvalidCommandFormat>(HasSubstr("Invalid command format. No 'type' in run command action JSON")));
}

TEST_F(RunCommandTests, testParseCommandFailsDueToMissingCommandsRequiredField)
{
    std::string commandJson = R"({
        "type": "sophos.mgt.action.RunCommands",
        "ignoreError": true,
        "timeout": 60,
        "expiration": 144444000000004
        })";
    EXPECT_THAT([&commandJson]() { ResponseActionsImpl::RunCommandAction::parseCommandAction(commandJson); },
                ThrowsMessage<ResponseActionsImpl::InvalidCommandFormat>(HasSubstr("Invalid command format. No 'commands' in run command action JSON")));
}

TEST_F(RunCommandTests, testParseCommandFailsDueToMissingIgnoreErrorRequiredField)
{
    std::string commandJson = R"({
        "type": "sophos.mgt.action.RunCommands",
        "commands": [
            "echo one",
            "echo two > /tmp/test.txt"
        ],
        "timeout": 60,
        "expiration": 144444000000004
        })";
    EXPECT_THAT([&commandJson]() { ResponseActionsImpl::RunCommandAction::parseCommandAction(commandJson); },
                ThrowsMessage<ResponseActionsImpl::InvalidCommandFormat>(HasSubstr("Invalid command format. No 'ignoreError' in run command action JSON")));
}

TEST_F(RunCommandTests, testParseCommandFailsDueToMissingTimeoutRequiredField)
{
    std::string commandJson = R"({
        "type": "sophos.mgt.action.RunCommands",
        "commands": [
            "echo one",
            "echo two > /tmp/test.txt"
        ],
        "ignoreError": true,
        "expiration": 144444000000004
        })";
    EXPECT_THAT([&commandJson]() { ResponseActionsImpl::RunCommandAction::parseCommandAction(commandJson); },
                ThrowsMessage<ResponseActionsImpl::InvalidCommandFormat>(HasSubstr("Invalid command format. No 'timeout' in run command action JSON")));
}

TEST_F(RunCommandTests, testParseCommandFailsDueToMissingExpirationRequiredField)
{
    std::string commandJson = R"({
        "type": "sophos.mgt.action.RunCommands",
        "commands": [
            "echo one",
            "echo two > /tmp/test.txt"
        ],
        "ignoreError": true,
        "timeout": 60
        })";

    EXPECT_THAT([&commandJson]() { ResponseActionsImpl::RunCommandAction::parseCommandAction(commandJson); },
                ThrowsMessage<ResponseActionsImpl::InvalidCommandFormat>(HasSubstr("Invalid command format. No 'expiration' in run command action JSON")));
}

TEST_F(RunCommandTests, testParseCommandFailsDueToTypeError)
{
    std::string commandJson = getCommandJsonStringWithWrongValueType();
    EXPECT_THAT([&commandJson]() { ResponseActionsImpl::RunCommandAction::parseCommandAction(commandJson); },
                ThrowsMessage<ResponseActionsImpl::InvalidCommandFormat>(HasSubstr("Failed to create Command Request object from run command JSON")));
}

TEST_F(RunCommandTests, testParseCommandFailsDueToMalformedJson)
{
    std::string commandJson = "this is not json";
    EXPECT_THAT([&commandJson]() { ResponseActionsImpl::RunCommandAction::parseCommandAction(commandJson); },
                ThrowsMessage<ResponseActionsImpl::InvalidCommandFormat>(HasSubstr("syntax error")));
}

TEST_F(RunCommandTests, testParseCommandFailsDueToEmptyJsonString)
{
    std::string commandJson = "";
        EXPECT_THAT([&commandJson]() { ResponseActionsImpl::RunCommandAction::parseCommandAction(commandJson); },
                    ThrowsMessage<ResponseActionsImpl::InvalidCommandFormat>(HasSubstr("Run command action JSON is empty")));
}

TEST_F(RunCommandTests, testParseCommandFailsDueToEmptyJsonObject)
{
    std::string commandJson = "{}";
        EXPECT_THAT([&commandJson]() { ResponseActionsImpl::RunCommandAction::parseCommandAction(commandJson); },
                    ThrowsMessage<ResponseActionsImpl::InvalidCommandFormat>(HasSubstr("Invalid command format. No 'type' in run command action JSON")));
}

// run

TEST_F(RunCommandTests, runMethodProducesValidOutForSinglecommand)
{
    Common::ProcessImpl::ProcessFactory::instance().replaceCreator(
        []()
        {
            auto mockProcess = new StrictMock<MockProcess>();
            std::vector<std::string> args = { "-c", "echo -n one" };
            EXPECT_CALL(*mockProcess, exec("/bin/bash", args)).Times(1);
            EXPECT_CALL(*mockProcess, waitUntilProcessEnds()).Times(1);
            EXPECT_CALL(*mockProcess, standardOutput()).WillOnce(Return("output message"));
            EXPECT_CALL(*mockProcess, errorOutput()).WillOnce(Return(""));
            EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));
            return std::unique_ptr<Common::Process::IProcess>(mockProcess);
        });

    std::string actionJson = getSingleCommandJsonString();
    std::string correlationId = "correlationID";
    nlohmann::json response = ResponseActionsImpl::RunCommandAction::run(actionJson, correlationId);

    EXPECT_EQ(response.at("type"), "sophos.mgt.response.RunCommands");
    EXPECT_GE(response.at("duration"), 0);
    EXPECT_EQ(response.at("result"), 0);
    EXPECT_GE(response.at("startedAt"), 1679057317);
    nlohmann::json cmdResults = nlohmann::json::array();
    nlohmann::json cmdResult;
    cmdResult["stdOut"] = "output message";
    cmdResult["stdErr"] = "";
    cmdResult["exitCode"] = 0;
    cmdResult["duration"] = 0;
    cmdResults.push_back(cmdResult);
    EXPECT_EQ(response.at("commandResults"), cmdResults);
}

TEST_F(RunCommandTests, runMethodHandlesInvalidJson)
{
    Common::ProcessImpl::ProcessFactory::instance().replaceCreator(
        []()
        {
            auto mockProcess = new StrictMock<MockProcess>();
            std::vector<std::string> args = { "-c", "echo -n one" };
            EXPECT_CALL(*mockProcess, exec("/bin/bash", args)).Times(1);
            EXPECT_CALL(*mockProcess, waitUntilProcessEnds()).Times(1);
            EXPECT_CALL(*mockProcess, standardOutput()).WillOnce(Return("output message"));
            EXPECT_CALL(*mockProcess, errorOutput()).WillOnce(Return(""));
            EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));
            return std::unique_ptr<Common::Process::IProcess>(mockProcess);
        });

    std::string correlationId = "correlationID";
    nlohmann::json response = ResponseActionsImpl::RunCommandAction::run("Not json string", correlationId);

    EXPECT_EQ(response.at("result"), 1);
}

// runCommands

TEST_F(RunCommandTests, runCommandsSingleCommandWithNoErrors)
{
    Common::ProcessImpl::ProcessFactory::instance().replaceCreator(
        []()
        {
            auto mockProcess = new StrictMock<MockProcess>();
            std::vector<std::string> args = { "-c", "echo -n one" };
            EXPECT_CALL(*mockProcess, exec("/bin/bash", args)).Times(1);
            EXPECT_CALL(*mockProcess, waitUntilProcessEnds()).Times(1);
            EXPECT_CALL(*mockProcess, standardOutput()).WillOnce(Return("output message"));
            EXPECT_CALL(*mockProcess, errorOutput()).WillOnce(Return(""));
            EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));
            return std::unique_ptr<Common::Process::IProcess>(mockProcess);
        });

    std::string correlationId = "correlationID";
    ResponseActionsImpl::CommandRequest cmd{
        .commands = { "echo -n one" }, .timeout = 10, .ignoreError = true, .expiration = 99999999999
    };

    auto response = ResponseActionsImpl::RunCommandAction::runCommands(cmd, correlationId);
    EXPECT_EQ(response.type, "sophos.mgt.response.RunCommands");
    EXPECT_EQ(response.result, ResponseResult::SUCCESS);
    EXPECT_GT(response.startedAt, 1679055498);
    EXPECT_GE(response.duration, 0);

    EXPECT_EQ(response.commandResults.size(), 1);
    EXPECT_EQ(response.commandResults[0].stdOut, "output message");
    EXPECT_EQ(response.commandResults[0].stdErr, "");
    EXPECT_EQ(response.commandResults[0].exitCode, 0);
    EXPECT_GE(response.commandResults[0].duration, 0);
}

TEST_F(RunCommandTests, runCommandsMultiplCommandsWithNoErrors)
{
    Common::ProcessImpl::ProcessFactory::instance().replaceCreator(
        []()
        {
            static int runCount = 1;
            auto mockProcess = new StrictMock<MockProcess>();
            std::vector<std::string> args;
            args = { "-c", "echo -n " + std::to_string(runCount++) };
            EXPECT_CALL(*mockProcess, exec("/bin/bash", args)).Times(1);
            EXPECT_CALL(*mockProcess, waitUntilProcessEnds()).Times(1);
            EXPECT_CALL(*mockProcess, standardOutput()).WillOnce(Return("output message"));
            EXPECT_CALL(*mockProcess, errorOutput()).WillOnce(Return(""));
            EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));
            return std::unique_ptr<Common::Process::IProcess>(mockProcess);
        });

    std::string correlationId = "correlationID";
    ResponseActionsImpl::CommandRequest cmd{
        .commands = { "echo -n 1", "echo -n 2" }, .timeout = 10, .ignoreError = true, .expiration = 99999999999
    };

    auto response = ResponseActionsImpl::RunCommandAction::runCommands(cmd, correlationId);
    EXPECT_EQ(response.type, "sophos.mgt.response.RunCommands");
    EXPECT_EQ(response.result, ResponseResult::SUCCESS);
    EXPECT_GT(response.startedAt, 1679055498);
    EXPECT_GE(response.duration, 0);

    EXPECT_EQ(response.commandResults.size(), 2);
    EXPECT_EQ(response.commandResults[0].stdOut, "output message");
    EXPECT_EQ(response.commandResults[0].stdErr, "");
    EXPECT_EQ(response.commandResults[0].exitCode, 0);
    EXPECT_GE(response.commandResults[0].duration, 0);

    EXPECT_EQ(response.commandResults[0].stdOut, "output message");
    EXPECT_EQ(response.commandResults[0].stdErr, "");
    EXPECT_EQ(response.commandResults[0].exitCode, 0);
    EXPECT_GE(response.commandResults[0].duration, 0);
}

TEST_F(RunCommandTests, runCommandsMultiplCommandsWithErrorsAndIgnoreErrosTrue)
{
    Common::ProcessImpl::ProcessFactory::instance().replaceCreator(
        []()
        {
            static int runCount = 1;
            auto mockProcess = new StrictMock<MockProcess>();
            std::vector<std::string> args;
            args = { "-c", "echo -n " + std::to_string(runCount++) };
            EXPECT_CALL(*mockProcess, exec("/bin/bash", args)).Times(1);
            EXPECT_CALL(*mockProcess, waitUntilProcessEnds()).Times(1);
            EXPECT_CALL(*mockProcess, standardOutput()).WillOnce(Return("output message"));
            EXPECT_CALL(*mockProcess, errorOutput()).WillOnce(Return(""));
            EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(1));
            return std::unique_ptr<Common::Process::IProcess>(mockProcess);
        });

    std::string correlationId = "correlationID";
    ResponseActionsImpl::CommandRequest cmd{
        .commands = { "echo -n 1", "echo -n 2" }, .timeout = 10, .ignoreError = true, .expiration = 99999999999
    };

    auto response = ResponseActionsImpl::RunCommandAction::runCommands(cmd, correlationId);
    EXPECT_EQ(response.type, "sophos.mgt.response.RunCommands");
    EXPECT_EQ(response.result, ResponseResult::ERROR);
    EXPECT_GT(response.startedAt, 1679055498);
    EXPECT_GE(response.duration, 0);

    EXPECT_EQ(response.commandResults.size(), 2);
    EXPECT_EQ(response.commandResults[0].stdOut, "output message");
    EXPECT_EQ(response.commandResults[0].stdErr, "");
    EXPECT_EQ(response.commandResults[0].exitCode, 1);
    EXPECT_GE(response.commandResults[0].duration, 0);

    EXPECT_EQ(response.commandResults[0].stdOut, "output message");
    EXPECT_EQ(response.commandResults[0].stdErr, "");
    EXPECT_EQ(response.commandResults[0].exitCode, 1);
    EXPECT_GE(response.commandResults[0].duration, 0);
}

TEST_F(RunCommandTests, runCommandsMultiplCommandsWithErrorsAndIgnoreErrorsFalse)
{
    Common::ProcessImpl::ProcessFactory::instance().replaceCreator(
        []()
        {
            static int runCount = 1;
            auto mockProcess = new StrictMock<MockProcess>();
            std::vector<std::string> args;
            args = { "-c", "echo -n " + std::to_string(runCount++) };
            EXPECT_CALL(*mockProcess, exec("/bin/bash", args)).Times(1);
            EXPECT_CALL(*mockProcess, waitUntilProcessEnds()).Times(1);
            EXPECT_CALL(*mockProcess, standardOutput()).WillOnce(Return("output message"));
            EXPECT_CALL(*mockProcess, errorOutput()).WillOnce(Return(""));
            EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(1));
            return std::unique_ptr<Common::Process::IProcess>(mockProcess);
        });

    std::string correlationId = "correlationID";
    ResponseActionsImpl::CommandRequest cmd{
        .commands = { "echo -n 1", "echo -n 2" }, .timeout = 10, .ignoreError = false, .expiration = 99999999999
    };

    auto response = ResponseActionsImpl::RunCommandAction::runCommands(cmd, correlationId);
    EXPECT_EQ(response.type, "sophos.mgt.response.RunCommands");
    EXPECT_EQ(response.result, ResponseResult::ERROR);
    EXPECT_GT(response.startedAt, 1679055498);
    EXPECT_GE(response.duration, 0);

    EXPECT_EQ(response.commandResults.size(), 1);
    EXPECT_EQ(response.commandResults[0].stdOut, "output message");
    EXPECT_EQ(response.commandResults[0].stdErr, "");
    EXPECT_EQ(response.commandResults[0].exitCode, 1);
    EXPECT_GE(response.commandResults[0].duration, 0);
}

TEST_F(RunCommandTests, runCommandsExpired)
{
    std::string correlationId = "correlationID";
    ResponseActionsImpl::CommandRequest cmd{
        .commands = { "echo -n one" }, .timeout = 10, .ignoreError = true, .expiration = 123
    };

    auto response = ResponseActionsImpl::RunCommandAction::runCommands(cmd, correlationId);
    EXPECT_EQ(response.type, "sophos.mgt.response.RunCommands");
    EXPECT_EQ(response.result, ResponseResult::EXPIRED);
    EXPECT_EQ(response.startedAt, 0);
    EXPECT_GE(response.duration, 0);
    EXPECT_EQ(response.commandResults.size(), 0);
}

// runCommand

TEST_F(RunCommandTests, runCommandWithNoErrors)
{
    Common::ProcessImpl::ProcessFactory::instance().replaceCreator(
        []()
        {
            auto mockProcess = new StrictMock<MockProcess>();
            std::vector<std::string> args = { "-c", "echo -n one" };
            EXPECT_CALL(*mockProcess, exec("/bin/bash", args)).Times(1);
            EXPECT_CALL(*mockProcess, waitUntilProcessEnds()).Times(1);
            EXPECT_CALL(*mockProcess, standardOutput()).WillOnce(Return("output message"));
            EXPECT_CALL(*mockProcess, errorOutput()).WillOnce(Return(""));
            EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));
            return std::unique_ptr<Common::Process::IProcess>(mockProcess);
        });

    auto response = ResponseActionsImpl::RunCommandAction::runCommand("echo -n one");

    EXPECT_EQ(response.stdOut, "output message");
    EXPECT_EQ(response.stdErr, "");
    EXPECT_EQ(response.exitCode, 0);
    EXPECT_GE(response.duration, 0);

}

TEST_F(RunCommandTests, runCommandWithErrors)
{
    Common::ProcessImpl::ProcessFactory::instance().replaceCreator(
        []()
        {
            auto mockProcess = new StrictMock<MockProcess>();
            std::vector<std::string> args = { "-c", "echo -n one" };
            EXPECT_CALL(*mockProcess, exec("/bin/bash", args)).Times(1);
            EXPECT_CALL(*mockProcess, waitUntilProcessEnds()).Times(1);
            EXPECT_CALL(*mockProcess, standardOutput()).WillOnce(Return("output message"));
            EXPECT_CALL(*mockProcess, errorOutput()).WillOnce(Return(""));
            EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(5));
            return std::unique_ptr<Common::Process::IProcess>(mockProcess);
        });

    auto response = ResponseActionsImpl::RunCommandAction::runCommand("echo -n one");

    EXPECT_EQ(response.stdOut, "output message");
    EXPECT_EQ(response.stdErr, "");
    EXPECT_EQ(response.exitCode, 5);
    EXPECT_GE(response.duration, 0);
}

TEST_F(RunCommandTests, runCommandWithErrorsAndStdErr)
{
    Common::ProcessImpl::ProcessFactory::instance().replaceCreator(
        []()
        {
            auto mockProcess = new StrictMock<MockProcess>();
            std::vector<std::string> args = { "-c", "echo -n one" };
            EXPECT_CALL(*mockProcess, exec("/bin/bash", args)).Times(1);
            EXPECT_CALL(*mockProcess, waitUntilProcessEnds()).Times(1);
            EXPECT_CALL(*mockProcess, standardOutput()).WillOnce(Return("output message"));
            EXPECT_CALL(*mockProcess, errorOutput()).WillOnce(Return("error message"));
            EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(5));
            return std::unique_ptr<Common::Process::IProcess>(mockProcess);
        });

    auto response = ResponseActionsImpl::RunCommandAction::runCommand("echo -n one");

    EXPECT_EQ(response.stdOut, "output message");
    EXPECT_EQ(response.stdErr, "error message");
    EXPECT_EQ(response.exitCode, 5);
    EXPECT_GE(response.duration, 0);
}