// Copyright 2023 Sophos Limited. All rights reserved.

#include "Common/ProcessImpl/ProcessImpl.h"
#include "ResponseActions/ResponseActionsImpl/InvalidCommandFormat.h"
#include "ResponseActions/ResponseActionsImpl/RunCommandAction.h"
#include "ResponseActions/ResponsePlugin/PluginCallback.h"
#include "Common/SystemCallWrapper/SystemCallWrapperFactory.h"
#include "Common/ZipUtilities/ZipUtils.h"
#include "tests/Common/Helpers/MemoryAppender.h"
#include "tests/Common/Helpers/MockProcess.h"
#include "tests/Common/Helpers/MockSignalHandler.h"
#include "tests/Common/Helpers/MockSysCalls.h"
#include "tests/Common/Helpers/MockSysCallsFactory.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace ResponseActions::RACommon;
using namespace ResponseActionsImpl;

class RunCommandTests : public MemoryAppenderUsingTests
{
    public:
        RunCommandTests()
            : MemoryAppenderUsingTests("ResponseActionsImpl")
        {}

        void SetUp() override
        {
            m_mockSignalHandler = std::make_shared<NiceMock<MockSignalHandler>>();

            setupMockSysCalls();

            m_runCommandAction =
                std::make_unique<RunCommandAction>(m_mockSignalHandler, m_mockSysCallWrapper);
        }

        void setupMockSysCalls()
        {
            m_mockSysCallWrapper = std::make_shared<NiceMock<MockSystemCallWrapper>>();
            ON_CALL(*m_mockSysCallWrapper, ppoll).WillByDefault(pollReturnsWithRevents(1, POLLIN));

        }

        std::shared_ptr<MockSystemCallWrapper> m_mockSysCallWrapper;
        std::shared_ptr<MockSignalHandler> m_mockSignalHandler;
        std::unique_ptr<RunCommandAction> m_runCommandAction;

        const std::string m_correlationId = "correlationID";
};

namespace
{
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

    std::tuple<std::string, std::string> getTestEucJpEncoding()
    {
        // echo -n "ありったけの夢をかき集め" | iconv -f utf-8 -t euc-jp | hexdump -C
        std::vector<unsigned char> threatPathBytes { 0xa4, 0xa2, 0xa4, 0xea, 0xa4, 0xc3, 0xa4, 0xbf, 0xa4, 0xb1, 0xa4, 0xce,
                                                    0xcc, 0xb4, 0xa4, 0xf2, 0xa4, 0xab, 0xa4, 0xad, 0xbd, 0xb8, 0xa4, 0xe1 };
        std::string threatPath(threatPathBytes.begin(), threatPathBytes.end());

        std::string threatPathUtf8 = "ありったけの夢をかき集め";
        return { threatPath, threatPathUtf8 };
    }
} // namespace

// to_json tests

TEST_F(RunCommandTests, SingleCommandResultToJson)
{

    SingleCommandResult singleCommandResult;
    singleCommandResult.duration = 100000000;
    singleCommandResult.stdOut = "some output text";
    singleCommandResult.stdErr = "some error text";

    nlohmann::json result = singleCommandResult;
    EXPECT_EQ(result.dump(), R"({"duration":100000000,"exitCode":0,"stdErr":"some error text","stdOut":"some output text"})");
}

TEST_F(RunCommandTests, SingleCommandResultToJson_EucJpEncoded)
{
    std::tuple<std::string, std::string> encoding = getTestEucJpEncoding();

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator(
        [&encoding]()
        {
            auto mockProcess = new StrictMock<MockProcess>();
            std::vector<std::string> args = { "-c", "echo -n one" };
            EXPECT_CALL(*mockProcess, exec("/bin/bash", args, _)).Times(1);
            EXPECT_CALL(*mockProcess, standardOutput()).WillOnce(Return(std::get<0>(encoding)));
            EXPECT_CALL(*mockProcess, errorOutput()).WillOnce(Return(std::get<0>(encoding)));
            EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));
            EXPECT_CALL(*mockProcess, wait(_,_)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
            EXPECT_CALL(*mockProcess, getStatus()).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
            return std::unique_ptr<Common::Process::IProcess>(mockProcess);
        });

    std::string actionJson = getSingleCommandJsonString();
    std::string correlationId = "correlationID";
    auto response = m_runCommandAction->run(actionJson, correlationId);

    EXPECT_EQ(response.at("type"), "sophos.mgt.response.RunCommands");
    EXPECT_GE(response.at("duration"), 0);
    EXPECT_EQ(response.at("result"), 0);
    EXPECT_GE(response.at("startedAt"), 1679057317);
    EXPECT_EQ(response.at("commandResults")[0]["stdOut"], std::get<1>(encoding) + " (EUC-JP)");
    EXPECT_EQ(response.at("commandResults")[0]["stdErr"], std::get<1>(encoding) + " (EUC-JP)");
    EXPECT_EQ(response.at("commandResults")[0]["exitCode"], 0);
    EXPECT_GE(response.at("commandResults")[0]["duration"], 0);
}

TEST_F(RunCommandTests, CommandResponseToJson)
{
    CommandResponse commandResponse;
    commandResponse.duration = 100000000;
    commandResponse.result = ResponseResult::SUCCESS;
    SingleCommandResult r1;
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

// run

TEST_F(RunCommandTests, runMethodProducesValidOutForSinglecommand)
{
    Common::ProcessImpl::ProcessFactory::instance().replaceCreator(
        []()
        {
            auto mockProcess = std::make_unique<StrictMock<MockProcess>>();
            std::vector<std::string> args = { "-c", "echo -n one" };
            EXPECT_CALL(*mockProcess, exec("/bin/bash", args, _)).Times(1);
            EXPECT_CALL(*mockProcess, standardOutput()).WillOnce(Return("output message"));
            EXPECT_CALL(*mockProcess, errorOutput()).WillOnce(Return(""));
            EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));
            EXPECT_CALL(*mockProcess, wait(_,_)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
            EXPECT_CALL(*mockProcess, getStatus()).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
            return mockProcess;
        });

    std::string actionJson = getSingleCommandJsonString();
    auto response = m_runCommandAction->run(actionJson, m_correlationId);

    EXPECT_EQ(response.at("type"), RUN_COMMAND_RESPONSE_TYPE);
    EXPECT_GE(response.at("duration"), 0);
    EXPECT_EQ(response.at("result"), 0);
    EXPECT_GE(response.at("startedAt"), 1679057317);
    EXPECT_EQ(response.at("commandResults").at(0).at("stdOut"), "output message");
    EXPECT_EQ(response.at("commandResults")[0].at("stdErr"), "");
    EXPECT_EQ(response.at("commandResults")[0].at("exitCode"), 0);
    EXPECT_GE(response.at("commandResults")[0].at("duration"), 0);
}

TEST_F(RunCommandTests, runMethodHandlesInvalidJson)
{
    Common::ProcessImpl::ProcessFactory::instance().replaceCreator(
        []()
        {
            auto mockProcess = std::make_unique<StrictMock<MockProcess>>();
            std::vector<std::string> args = { "-c", "echo -n one" };
            EXPECT_CALL(*mockProcess, exec("/bin/bash", args, _)).Times(1);
            EXPECT_CALL(*mockProcess, standardOutput()).WillOnce(Return("output message"));
            EXPECT_CALL(*mockProcess, errorOutput()).WillOnce(Return(""));
            EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));
            EXPECT_CALL(*mockProcess, getStatus()).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
            return mockProcess;
        });

    auto response = m_runCommandAction->run("Not json string", m_correlationId);
    EXPECT_EQ(response.at("result"), 1);
}

TEST_F(RunCommandTests, runMethodHandlesJsonOutputOrError)
{
    const std::string output = R"({"json": "output"})";
    const std::string error = R"({"json": "error"})";

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator(
        [output, error]()
        {
            auto mockProcess = std::make_unique<StrictMock<MockProcess>>();
            std::vector<std::string> args = { "-c", "echo -n one" };
            EXPECT_CALL(*mockProcess, exec("/bin/bash", args, _)).Times(1);
            EXPECT_CALL(*mockProcess, standardOutput()).WillOnce(Return(output));
            EXPECT_CALL(*mockProcess, errorOutput()).WillOnce(Return(error));
            EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));
            EXPECT_CALL(*mockProcess, wait(_,_)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
            EXPECT_CALL(*mockProcess, getStatus()).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
            return mockProcess;
        });

    std::string actionJson = getSingleCommandJsonString();
    auto response = m_runCommandAction->run(actionJson, m_correlationId);

    EXPECT_EQ(response.at("type"), RUN_COMMAND_RESPONSE_TYPE);
    EXPECT_GE(response.at("duration"), 0);
    EXPECT_EQ(response.at("result"), 0);
    EXPECT_GE(response.at("startedAt"), 1679057317);
    const auto& actualCmdResults = response.at("commandResults");
    ASSERT_EQ(actualCmdResults.size(), 1);
    const auto& actualCmdResult = actualCmdResults.at(0);
    EXPECT_EQ(actualCmdResult.at("stdOut"), output);
    EXPECT_EQ(actualCmdResult.at("stdErr"), error);
    EXPECT_EQ(actualCmdResult.at("exitCode"), 0);
    EXPECT_GE(actualCmdResult.at("duration"), 0); // Can't rely on the duration always being 0
    EXPECT_LE(actualCmdResult.at("duration"), 5);
}

TEST_F(RunCommandTests, runMethodHandlesLargeOutputOrError)
{
    const std::string output(30000, 'a');
    const std::string error(30000, 'c');

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator(
        [output, error]()
        {
            auto mockProcess = std::make_unique<StrictMock<MockProcess>>();
            std::vector<std::string> args = { "-c", "echo -n one" };
            EXPECT_CALL(*mockProcess, exec("/bin/bash", args, _)).Times(1);
            EXPECT_CALL(*mockProcess, standardOutput()).WillOnce(Return(output));
            EXPECT_CALL(*mockProcess, errorOutput()).WillOnce(Return(error));
            EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));
            EXPECT_CALL(*mockProcess, wait(_,_)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
            EXPECT_CALL(*mockProcess, getStatus()).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
            return mockProcess;
        });

    std::string actionJson = getSingleCommandJsonString();
    auto response = m_runCommandAction->run(actionJson, m_correlationId);

    EXPECT_EQ(response.at("type"), RUN_COMMAND_RESPONSE_TYPE);
    EXPECT_GE(response.at("duration"), 0);
    EXPECT_EQ(response.at("result"), 0);
    EXPECT_GE(response.at("startedAt"), 1679057317);
    EXPECT_EQ(response.at("commandResults")[0]["stdOut"], output);
    EXPECT_EQ(response.at("commandResults")[0]["stdErr"], error);
    EXPECT_EQ(response.at("commandResults")[0]["exitCode"], 0);
    EXPECT_GT(response.at("commandResults")[0]["duration"], 0);
    EXPECT_LT(response.at("commandResults")[0]["duration"], 1000);
}

TEST_F(RunCommandTests, runMethodReturnsTimeOut)
{
    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, _, _, _))
        .WillOnce(pollReturnsWithRevents(2, POLLIN));

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator(
        []()
        {
            auto mockProcess = std::make_unique<StrictMock<MockProcess>>();
            std::vector<std::string> args = { "-c", "echo -n one" };
            EXPECT_CALL(*mockProcess, exec("/bin/bash", args, _)).Times(1);
            EXPECT_CALL(*mockProcess, standardOutput()).WillOnce(Return("output message"));
            EXPECT_CALL(*mockProcess, errorOutput()).WillOnce(Return(""));
            EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(15));
            EXPECT_CALL(*mockProcess, getStatus()).WillOnce(Return(Common::Process::ProcessStatus::RUNNING));
            EXPECT_CALL(*mockProcess, kill(_)).Times(1);
            EXPECT_CALL(*mockProcess, waitUntilProcessEnds()).Times(1);
            return mockProcess;
        });

    std::string actionJson = getSingleCommandJsonString();
    auto response = m_runCommandAction->run(actionJson, m_correlationId);

    EXPECT_EQ(response.at("type"), RUN_COMMAND_RESPONSE_TYPE);
    EXPECT_GE(response.at("duration"), 0);
    EXPECT_EQ(response.at("result"), 2);
    EXPECT_GE(response.at("startedAt"), 1679057317);
    const auto& actualCmdResults = response.at("commandResults");
    ASSERT_EQ(actualCmdResults.size(), 1);
    const auto& actualCmdResult = actualCmdResults.at(0);
    EXPECT_EQ(actualCmdResult.at("stdOut"), "output message");
    EXPECT_EQ(actualCmdResult.at("stdErr"), "");
    EXPECT_EQ(actualCmdResult.at("exitCode"), 15);
    EXPECT_GE(actualCmdResult.at("duration"), 0); // Can't rely on the duration always being 0
    EXPECT_LE(actualCmdResult.at("duration"), 5);
}

TEST_F(RunCommandTests, runCommandsExpired)
{
    std::string action (
        R"({"type": "sophos.mgt.action.RunCommands",
            "commands": ["echo -n one"],
            "ignoreError": true,
            "timeout": 10,
            "expiration": 123
        })");

    auto response = m_runCommandAction->run(action, m_correlationId);
    EXPECT_EQ(response.at("type"), RUN_COMMAND_RESPONSE_TYPE);
    EXPECT_EQ(response.at("result"), ResponseResult::EXPIRED);
    EXPECT_FALSE(response.contains("commandResults"));
    EXPECT_FALSE(response.contains("duration"));
    EXPECT_FALSE(response.contains("startedAt"));
}

TEST_F(RunCommandTests, runSingleCommandExpiryLarge)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    const std::string expectedMsg = "Command " + m_correlationId + " has expired so will not be run.";

    std::string action (
        R"({"type": "sophos.mgt.action.RunCommands",
            "commands": [
                "echo one",
                "echo two > /tmp/test.txt"
            ],
            "ignoreError": true,
            "timeout": 60,
            "expiration": 18446744073709551616
        })");

    auto response = m_runCommandAction->run(action, m_correlationId);

    EXPECT_EQ(response["type"], RUN_COMMAND_RESPONSE_TYPE);
    EXPECT_EQ(response["result"], ResponseResult::EXPIRED);
    EXPECT_EQ(response["errorMessage"], expectedMsg);
    EXPECT_FALSE(response.contains("commandResults"));
    EXPECT_FALSE(response.contains("duration"));
    EXPECT_FALSE(response.contains("startedAt"));

    EXPECT_TRUE(appenderContains(expectedMsg));
}

TEST_F(RunCommandTests, runSingleCommandExpiryNegative)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    std::string action (
        R"({"type": "sophos.mgt.action.RunCommands",
            "commands": [
                "echo one",
                "echo two > /tmp/test.txt"
            ],
            "ignoreError": true,
            "timeout": 60,
            "expiration": -1234
        })");

    auto response = m_runCommandAction->run(action, m_correlationId);
    EXPECT_EQ(response["type"], RUN_COMMAND_RESPONSE_TYPE);
    EXPECT_EQ(response["result"], ResponseResult::ERROR);
    EXPECT_EQ(response["errorMessage"], "Error parsing command from Central: " + m_correlationId);
    EXPECT_FALSE(response.contains("commandResults"));
    EXPECT_FALSE(response.contains("duration"));
    EXPECT_FALSE(response.contains("startedAt"));

    EXPECT_TRUE(appenderContains("Invalid command format. Failed to process CommandRequest from action JSON: expiration is a negative value: -1234"));
}

// runCommands

TEST_F(RunCommandTests, runCommandsSingleCommandWithNoErrors)
{
    Common::ProcessImpl::ProcessFactory::instance().replaceCreator(
        []()
        {
            auto mockProcess = std::make_unique<StrictMock<MockProcess>>();
            std::vector<std::string> args = { "-c", "echo -n one" };
            EXPECT_CALL(*mockProcess, exec("/bin/bash", args, _)).Times(1);
            EXPECT_CALL(*mockProcess, standardOutput()).WillOnce(Return("output message"));
            EXPECT_CALL(*mockProcess, errorOutput()).WillOnce(Return(""));
            EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));
            EXPECT_CALL(*mockProcess, wait(_,_)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
            EXPECT_CALL(*mockProcess, getStatus()).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));

            return mockProcess;
        });

    CommandRequest cmd{
        .commands = { "echo -n one" }, .timeout = 10, .ignoreError = true, .expiration = 99999999999
    };

    auto response = m_runCommandAction->runCommands(cmd);
    EXPECT_EQ(response.type, "sophos.mgt.response.RunCommands");
    EXPECT_EQ(response.result, ResponseResult::SUCCESS);
    EXPECT_GT(response.startedAt, 1679055498);
    EXPECT_GE(response.duration, 0);

    ASSERT_EQ(response.commandResults.size(), 1);
    EXPECT_EQ(response.commandResults.at(0).stdOut, "output message");
    EXPECT_EQ(response.commandResults[0].stdErr, "");
    EXPECT_EQ(response.commandResults[0].exitCode, 0);
    EXPECT_GE(response.commandResults[0].duration, 0);
}

TEST_F(RunCommandTests, runCommandsMultipleCommandsWithNoErrors)
{
    Common::ProcessImpl::ProcessFactory::instance().replaceCreator(
        []()
        {
            static int runCount = 1;
            auto mockProcess = std::make_unique<StrictMock<MockProcess>>();
            std::vector<std::string> args;
            args = { "-c", "echo -n " + std::to_string(runCount++) };
            EXPECT_CALL(*mockProcess, exec("/bin/bash", args, _)).Times(1);
            EXPECT_CALL(*mockProcess, standardOutput()).WillOnce(Return("output message"));
            EXPECT_CALL(*mockProcess, errorOutput()).WillOnce(Return(""));
            EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));
            EXPECT_CALL(*mockProcess, wait(_,_)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
            EXPECT_CALL(*mockProcess, getStatus()).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
            return mockProcess;
        });

    CommandRequest cmd{
        .commands = { "echo -n 1", "echo -n 2" }, .timeout = 10, .ignoreError = true, .expiration = 99999999999
    };

    auto response = m_runCommandAction->runCommands(cmd);
    EXPECT_EQ(response.type, "sophos.mgt.response.RunCommands");
    EXPECT_EQ(response.result, ResponseResult::SUCCESS);
    EXPECT_GT(response.startedAt, 1679055498);
    EXPECT_GE(response.duration, 0);

    ASSERT_EQ(response.commandResults.size(), 2);
    EXPECT_EQ(response.commandResults[0].stdOut, "output message");
    EXPECT_EQ(response.commandResults[0].stdErr, "");
    EXPECT_EQ(response.commandResults[0].exitCode, 0);
    EXPECT_GE(response.commandResults[0].duration, 0);

    EXPECT_EQ(response.commandResults[1].stdOut, "output message");
    EXPECT_EQ(response.commandResults[1].stdErr, "");
    EXPECT_EQ(response.commandResults[1].exitCode, 0);
    EXPECT_GE(response.commandResults[1].duration, 0);
}

TEST_F(RunCommandTests, runCommandsManyCommandsInOrderWithNoErrors)
{
    const int testQuantity = 2000;
    const std::string outputMsg = "output message";

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator(
        [outputMsg]()
        {
            static int runCount = 1;
            auto mockProcess = std::make_unique<StrictMock<MockProcess>>();
            std::vector<std::string> args;
            args = { "-c", "echo -n " + std::to_string(runCount) };
            EXPECT_CALL(*mockProcess, exec("/bin/bash", args, _)).Times(1);
            EXPECT_CALL(*mockProcess, standardOutput()).WillOnce(Return(outputMsg + std::to_string(runCount++)));
            EXPECT_CALL(*mockProcess, errorOutput()).WillOnce(Return(""));
            EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));
            EXPECT_CALL(*mockProcess, wait(_,_)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
            EXPECT_CALL(*mockProcess, getStatus()).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
            return mockProcess;
        });

    std::vector<std::string> commands;
    for (int i = 1; i <= testQuantity; i++)
    {
        commands.push_back("echo -n " + std::to_string(i));
    }
    CommandRequest cmd{
        .commands = commands, .timeout = 10, .ignoreError = true, .expiration = 99999999999
    };

    auto response = m_runCommandAction->runCommands(cmd);
    EXPECT_EQ(response.type, "sophos.mgt.response.RunCommands");
    EXPECT_EQ(response.result, ResponseResult::SUCCESS);
    EXPECT_GT(response.startedAt, 1679055498);
    EXPECT_GE(response.duration, 0);
    //Check they are in order
    ASSERT_EQ(response.commandResults.size(), testQuantity);
    for (int i = 1; i <= testQuantity; i++)
    {
        EXPECT_EQ(response.commandResults.at(i - 1).stdOut, outputMsg + std::to_string(i));
    }
}

TEST_F(RunCommandTests, runCommandsHugeCommandWithNoErrors)
{
    const std::string largeStr(30000, 'a');
    const std::string largeCommand("echo " + largeStr);

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator(
        [largeCommand]()
        {
            auto mockProcess = std::make_unique<StrictMock<MockProcess>>();
            std::vector<std::string> args;
            args = { "-c", largeCommand };
            EXPECT_CALL(*mockProcess, exec("/bin/bash", args, _)).Times(1);
            EXPECT_CALL(*mockProcess, standardOutput()).WillOnce(Return("output message"));
            EXPECT_CALL(*mockProcess, errorOutput()).WillOnce(Return(""));
            EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));
            EXPECT_CALL(*mockProcess, wait(_,_)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
            EXPECT_CALL(*mockProcess, getStatus()).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
            return mockProcess;
        });


    CommandRequest cmd{
        .commands = { largeCommand }, .timeout = 10, .ignoreError = true, .expiration = 99999999999
    };

    auto response = m_runCommandAction->runCommands(cmd);
    EXPECT_EQ(response.type, "sophos.mgt.response.RunCommands");
    EXPECT_EQ(response.result, ResponseResult::SUCCESS);
    EXPECT_GT(response.startedAt, 1679055498);
    EXPECT_GE(response.duration, 0);

    EXPECT_EQ(response.commandResults.size(), 1);
}

TEST_F(RunCommandTests, runCommandsMultiplCommandsWithErrorsAndIgnoreErrorsTrue)
{
    Common::ProcessImpl::ProcessFactory::instance().replaceCreator(
        []()
        {
            static int runCount = 1;
            auto mockProcess = std::make_unique<StrictMock<MockProcess>>();
            std::vector<std::string> args;
            args = { "-c", "echo -n " + std::to_string(runCount++) };
            EXPECT_CALL(*mockProcess, exec("/bin/bash", args, _)).Times(1);
            EXPECT_CALL(*mockProcess, standardOutput()).WillOnce(Return("output message"));
            EXPECT_CALL(*mockProcess, errorOutput()).WillOnce(Return(""));
            EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(1));
            EXPECT_CALL(*mockProcess, wait(_,_)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
            EXPECT_CALL(*mockProcess, getStatus()).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
            return mockProcess;
        });

    CommandRequest cmd{
        .commands = { "echo -n 1", "echo -n 2" }, .timeout = 10, .ignoreError = true, .expiration = 99999999999
    };

    auto response = m_runCommandAction->runCommands(cmd);
    EXPECT_EQ(response.type, "sophos.mgt.response.RunCommands");
    EXPECT_EQ(response.result, ResponseResult::ERROR);
    EXPECT_GT(response.startedAt, 1679055498);
    EXPECT_GE(response.duration, 0);

    EXPECT_EQ(response.commandResults.size(), 2);
    EXPECT_EQ(response.commandResults[0].stdOut, "output message");
    EXPECT_EQ(response.commandResults[0].stdErr, "");
    EXPECT_EQ(response.commandResults[0].exitCode, 1);
    EXPECT_GE(response.commandResults[0].duration, 0);

    EXPECT_EQ(response.commandResults[1].stdOut, "output message");
    EXPECT_EQ(response.commandResults[1].stdErr, "");
    EXPECT_EQ(response.commandResults[1].exitCode, 1);
    EXPECT_GE(response.commandResults[1].duration, 0);
}

TEST_F(RunCommandTests, runCommandsMultipleCommandsWithErrorsAndIgnoreErrorsFalse)
{
    Common::ProcessImpl::ProcessFactory::instance().replaceCreator(
        []()
        {
            static int runCount = 1;
            auto mockProcess = std::make_unique<StrictMock<MockProcess>>();
            std::vector<std::string> args;
            args = { "-c", "echo -n " + std::to_string(runCount++) };
            EXPECT_CALL(*mockProcess, exec("/bin/bash", args, _)).Times(1);
            EXPECT_CALL(*mockProcess, standardOutput()).WillOnce(Return("output message"));
            EXPECT_CALL(*mockProcess, errorOutput()).WillOnce(Return(""));
            EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(1));
            EXPECT_CALL(*mockProcess, wait(_,_)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
            EXPECT_CALL(*mockProcess, getStatus()).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
            return mockProcess;
        });

    CommandRequest cmd{
        .commands = { "echo -n 1", "echo -n 2" }, .timeout = 10, .ignoreError = false, .expiration = 99999999999
    };

    auto response = m_runCommandAction->runCommands(cmd);
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

TEST_F(RunCommandTests, runCommandsMultipleExitsWhenTerminatedBeforeComplete)
{
    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, _, _, _))
        .WillOnce(pollReturnsWithRevents(1, POLLIN))
        .WillOnce(pollReturnsWithRevents(0, POLLIN));

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator(
        []()
        {
            static int runCount = 1;
            auto mockProcess = std::make_unique<StrictMock<MockProcess>>();
            std::vector<std::string> args;
            args = { "-c", "echo -n " + std::to_string(runCount++) };
            EXPECT_CALL(*mockProcess, exec("/bin/bash", args, _)).Times(1);
            EXPECT_CALL(*mockProcess, standardOutput()).WillOnce(Return("output message"));
            EXPECT_CALL(*mockProcess, errorOutput()).WillOnce(Return(""));
            EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));
            if (runCount == 2)
            {
                EXPECT_CALL(*mockProcess, wait(_,_ )).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
            }
            EXPECT_CALL(*mockProcess, getStatus()).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
            return mockProcess;
        });

    CommandRequest cmd{
        .commands = { "echo -n 1", "echo -n 2", "echo -n 3" }, .timeout = 10, .ignoreError = true, .expiration = 99999999999
    };
    EXPECT_EQ(cmd.commands.size(), 3);
    auto response = m_runCommandAction->runCommands(cmd);
    EXPECT_EQ(response.commandResults.size(), 2);
}

// runCommand

TEST_F(RunCommandTests, runCommandWithNoErrors)
{
    Common::ProcessImpl::ProcessFactory::instance().replaceCreator(
        []()
        {
            auto mockProcess = std::make_unique<StrictMock<MockProcess>>();
            std::vector<std::string> args = { "-c", "echo -n one" };
            EXPECT_CALL(*mockProcess, exec("/bin/bash", args, _)).Times(1);
            EXPECT_CALL(*mockProcess, standardOutput()).WillOnce(Return("output message"));
            EXPECT_CALL(*mockProcess, errorOutput()).WillOnce(Return(""));
            EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));
            EXPECT_CALL(*mockProcess, wait(_,_)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
            EXPECT_CALL(*mockProcess, getStatus()).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
            return mockProcess;
        });

    auto response = m_runCommandAction->runCommand("echo -n one");

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
            auto mockProcess = std::make_unique<StrictMock<MockProcess>>();
            std::vector<std::string> args = { "-c", "echo -n one" };
            EXPECT_CALL(*mockProcess, exec("/bin/bash", args, _)).Times(1);
            EXPECT_CALL(*mockProcess, standardOutput()).WillOnce(Return("output message"));
            EXPECT_CALL(*mockProcess, errorOutput()).WillOnce(Return(""));
            EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(5));
            EXPECT_CALL(*mockProcess, wait(_,_)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
            EXPECT_CALL(*mockProcess, getStatus()).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
            return mockProcess;
        });

    auto response = m_runCommandAction->runCommand("echo -n one");

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
            auto mockProcess = std::make_unique<StrictMock<MockProcess>>();
            std::vector<std::string> args = { "-c", "echo -n one" };
            EXPECT_CALL(*mockProcess, exec("/bin/bash", args, _)).Times(1);
            EXPECT_CALL(*mockProcess, standardOutput()).WillOnce(Return("output message"));
            EXPECT_CALL(*mockProcess, errorOutput()).WillOnce(Return("error message"));
            EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(5));
            EXPECT_CALL(*mockProcess, wait(_,_)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
            EXPECT_CALL(*mockProcess, getStatus()).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
            return mockProcess;
        });

    auto response = m_runCommandAction->runCommand("echo -n one");

    EXPECT_EQ(response.stdOut, "output message");
    EXPECT_EQ(response.stdErr, "error message");
    EXPECT_EQ(response.exitCode, 5);
    EXPECT_GE(response.duration, 0);
}



//run command ppoll specific

TEST_F(RunCommandTests, runCommandWithParentTerminatedChildRunning)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, _, _, _))
        .WillOnce(pollReturnsWithRevents(0, POLLIN));

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator(
        []()
        {
            auto mockProcess = std::make_unique<StrictMock<MockProcess>>();
            std::vector<std::string> args = { "-c", "echo -n one" };
            EXPECT_CALL(*mockProcess, exec("/bin/bash", args, _)).Times(1);
            EXPECT_CALL(*mockProcess, standardOutput()).Times(1);
            EXPECT_CALL(*mockProcess, errorOutput()).Times(1);
            EXPECT_CALL(*mockProcess, exitCode()).Times(1);
            EXPECT_CALL(*mockProcess, getStatus()).WillOnce(Return(Common::Process::ProcessStatus::RUNNING));
            EXPECT_CALL(*mockProcess, kill(_)).Times(1);
            EXPECT_CALL(*mockProcess, waitUntilProcessEnds()).Times(1);
            return mockProcess;
        });

    m_runCommandAction->runCommand("echo -n one");

    EXPECT_TRUE(appenderContains("RunCommandAction has received termination command"));
    EXPECT_TRUE(appenderContains("Child process is still running, killing process"));
}

TEST_F(RunCommandTests, runCommandWithChildTerminated)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator(
        []()
        {
            auto mockProcess = std::make_unique<StrictMock<MockProcess>>();
            std::vector<std::string> args = { "-c", "echo -n one" };
            EXPECT_CALL(*mockProcess, exec("/bin/bash", args, _)).Times(1);
            EXPECT_CALL(*mockProcess, standardOutput()).Times(1);
            EXPECT_CALL(*mockProcess, errorOutput()).Times(1);
            EXPECT_CALL(*mockProcess, exitCode()).Times(1);
            EXPECT_CALL(*mockProcess, wait(_,_)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
            EXPECT_CALL(*mockProcess, getStatus()).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
            return mockProcess;
        });

    m_runCommandAction->runCommand("echo -n one");

    EXPECT_TRUE(appenderContains("Child Process has received termination command"));
}

TEST_F(RunCommandTests, runCommandPpollErrorExitAndKillWhenNotEINTR)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, _, _, _))
        .WillOnce(SetErrnoAndReturn(EFAULT , -1));

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator(
        []()
        {
            auto mockProcess = std::make_unique<StrictMock<MockProcess>>();
            std::vector<std::string> args = { "-c", "echo -n one" };
            EXPECT_CALL(*mockProcess, exec("/bin/bash", args, _)).Times(1);
            EXPECT_CALL(*mockProcess, standardOutput()).Times(1);
            EXPECT_CALL(*mockProcess, errorOutput()).Times(1);
            EXPECT_CALL(*mockProcess, exitCode()).Times(1);
            EXPECT_CALL(*mockProcess, getStatus()).WillOnce(Return(Common::Process::ProcessStatus::RUNNING));
            EXPECT_CALL(*mockProcess, kill(_)).Times(1);
            EXPECT_CALL(*mockProcess, waitUntilProcessEnds()).Times(1);
            return mockProcess;
        });

    m_runCommandAction->runCommand("echo -n one");

    EXPECT_TRUE(appenderContains("Error from ppoll while waiting for command to finish: Bad address(14)"));
    EXPECT_TRUE(appenderContains("Child process is still running, killing process"));
}

TEST_F(RunCommandTests, runCommandPpollErrorContinueWhenEINTR)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, _, _, _))
        .WillOnce(SetErrnoAndReturn(EINTR, -1))
        .WillOnce(pollReturnsWithRevents(1, POLLIN));

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator(
        []()
        {
            auto mockProcess = std::make_unique<StrictMock<MockProcess>>();
            std::vector<std::string> args = { "-c", "echo -n one" };
            EXPECT_CALL(*mockProcess, exec("/bin/bash", args, _)).Times(1);
            EXPECT_CALL(*mockProcess, standardOutput()).Times(1);
            EXPECT_CALL(*mockProcess, errorOutput()).Times(1);
            EXPECT_CALL(*mockProcess, exitCode()).Times(1);
            EXPECT_CALL(*mockProcess, wait(_,_)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
            EXPECT_CALL(*mockProcess, getStatus()).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
            return mockProcess;
        });

    m_runCommandAction->runCommand("echo -n one");

    EXPECT_TRUE(appenderContains("Ignoring EINTR from ppoll"));
    EXPECT_TRUE(appenderContains("Child Process has received termination command"));
}

TEST_F(RunCommandTests, runCommandPpollSIGUSR1timesout)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, _, _, _)).WillOnce(pollReturnsWithRevents(2, POLLIN));

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator(
        []()
        {
            auto mockProcess = std::make_unique<StrictMock<MockProcess>>();
            std::vector<std::string> args = { "-c", "echo -n one" };
            EXPECT_CALL(*mockProcess, exec("/bin/bash", args, _)).Times(1);
            EXPECT_CALL(*mockProcess, standardOutput()).Times(1);
            EXPECT_CALL(*mockProcess, errorOutput()).Times(1);
            EXPECT_CALL(*mockProcess, exitCode()).Times(1);
            EXPECT_CALL(*mockProcess, kill(_)).WillOnce(Return(true));
            EXPECT_CALL(*mockProcess, waitUntilProcessEnds()).Times(1);
            EXPECT_CALL(*mockProcess, getStatus()).WillOnce(Return(Common::Process::ProcessStatus::RUNNING));
            return mockProcess;
        });

    m_runCommandAction->runCommand("echo -n one");

    EXPECT_TRUE(appenderContains("RunCommandAction has received termination command due to timeout"));
    EXPECT_TRUE(appenderContains("Child process killed as it took longer than 26 seconds to stop"));
}

