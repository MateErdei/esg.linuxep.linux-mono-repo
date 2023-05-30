// Copyright 2023 Sophos Limited. All rights reserved.

#include "ProcessImpl/ProcessImpl.h"
#include "ResponseActions/ResponseActionsImpl/InvalidCommandFormat.h"
#include "ResponseActions/ResponseActionsImpl/RunCommandAction.h"
#include "ResponseActions/ResponsePlugin/PluginCallback.h"
#include "modules/Common/SystemCallWrapper/SystemCallWrapperFactory.h"
#include "modules/Common/ZipUtilities/ZipUtils.h"
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
                std::make_unique<RunCommandAction>(m_mockSignalHandler, m_mockSysCallFactory);
        }

        void setupMockSysCalls()
        {
            m_mockSysCallWrapper = std::make_shared<NiceMock<MockSystemCallWrapper>>();
            ON_CALL(*m_mockSysCallWrapper, ppoll).WillByDefault(pollReturnsWithRevents(1, POLLIN));

            m_mockSysCallFactory = std::make_shared<NiceMock<MockSystemCallWrapperFactory>>();
            ON_CALL(*m_mockSysCallFactory, createSystemCallWrapper).WillByDefault(Return(m_mockSysCallWrapper));
        }

        std::shared_ptr<MockSystemCallWrapperFactory> m_mockSysCallFactory;
        std::shared_ptr<MockSystemCallWrapper> m_mockSysCallWrapper;
        std::shared_ptr<MockSignalHandler> m_mockSignalHandler;
        std::unique_ptr<RunCommandAction> m_runCommandAction;
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
            auto mockProcess = new StrictMock<MockProcess>();
            std::vector<std::string> args = { "-c", "echo -n one" };
            EXPECT_CALL(*mockProcess, exec("/bin/bash", args, _)).Times(1);
            EXPECT_CALL(*mockProcess, standardOutput()).WillOnce(Return("output message"));
            EXPECT_CALL(*mockProcess, errorOutput()).WillOnce(Return(""));
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
            EXPECT_CALL(*mockProcess, exec("/bin/bash", args, _)).Times(1);
            EXPECT_CALL(*mockProcess, standardOutput()).WillOnce(Return("output message"));
            EXPECT_CALL(*mockProcess, errorOutput()).WillOnce(Return(""));
            EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));
            EXPECT_CALL(*mockProcess, getStatus()).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
            return std::unique_ptr<Common::Process::IProcess>(mockProcess);
        });

    std::string correlationId = "correlationID";
    auto response = m_runCommandAction->run("Not json string", correlationId);
    EXPECT_EQ(response.at("result"), 1);
}

TEST_F(RunCommandTests, runMethodReturnsTimeOut)
{
    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, _, _, _))
        .WillOnce(pollReturnsWithRevents(2, POLLIN));

    Common::ProcessImpl::ProcessFactory::instance().replaceCreator(
        []()
        {
            auto mockProcess = new StrictMock<MockProcess>();
            std::vector<std::string> args = { "-c", "echo -n one" };
            EXPECT_CALL(*mockProcess, exec("/bin/bash", args, _)).Times(1);
            EXPECT_CALL(*mockProcess, standardOutput()).WillOnce(Return("output message"));
            EXPECT_CALL(*mockProcess, errorOutput()).WillOnce(Return(""));
            EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(15));
            EXPECT_CALL(*mockProcess, getStatus()).WillOnce(Return(Common::Process::ProcessStatus::RUNNING));
            EXPECT_CALL(*mockProcess, kill()).Times(1);
            EXPECT_CALL(*mockProcess, waitUntilProcessEnds()).Times(1);
            return std::unique_ptr<Common::Process::IProcess>(mockProcess);
        });

    std::string actionJson = getSingleCommandJsonString();
    std::string correlationId = "correlationID";
    auto response = m_runCommandAction->run(actionJson, correlationId);

    EXPECT_EQ(response.at("type"), "sophos.mgt.response.RunCommands");
    EXPECT_GE(response.at("duration"), 0);
    EXPECT_EQ(response.at("result"), 2);
    EXPECT_GE(response.at("startedAt"), 1679057317);
    nlohmann::json cmdResults = nlohmann::json::array();
    nlohmann::json cmdResult;
    cmdResult["stdOut"] = "output message";
    cmdResult["stdErr"] = "";
    cmdResult["exitCode"] = 15;
    cmdResult["duration"] = 0;
    cmdResults.push_back(cmdResult);
    EXPECT_EQ(response.at("commandResults"), cmdResults);
}

// runCommands

TEST_F(RunCommandTests, runCommandsSingleCommandWithNoErrors)
{
    Common::ProcessImpl::ProcessFactory::instance().replaceCreator(
        []()
        {
            auto mockProcess = new StrictMock<MockProcess>();
            std::vector<std::string> args = { "-c", "echo -n one" };
            EXPECT_CALL(*mockProcess, exec("/bin/bash", args, _)).Times(1);
            EXPECT_CALL(*mockProcess, standardOutput()).WillOnce(Return("output message"));
            EXPECT_CALL(*mockProcess, errorOutput()).WillOnce(Return(""));
            EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));
            EXPECT_CALL(*mockProcess, wait(_,_)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
            EXPECT_CALL(*mockProcess, getStatus()).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));

            return std::unique_ptr<Common::Process::IProcess>(mockProcess);
        });

    std::string correlationId = "correlationID";
    CommandRequest cmd{
        .commands = { "echo -n one" }, .timeout = 10, .ignoreError = true, .expiration = 99999999999
    };

    auto response = m_runCommandAction->runCommands(cmd, correlationId);
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

TEST_F(RunCommandTests, runCommandsMultipleCommandsWithNoErrors)
{
    Common::ProcessImpl::ProcessFactory::instance().replaceCreator(
        []()
        {
            static int runCount = 1;
            auto mockProcess = new StrictMock<MockProcess>();
            std::vector<std::string> args;
            args = { "-c", "echo -n " + std::to_string(runCount++) };
            EXPECT_CALL(*mockProcess, exec("/bin/bash", args, _)).Times(1);
            EXPECT_CALL(*mockProcess, standardOutput()).WillOnce(Return("output message"));
            EXPECT_CALL(*mockProcess, errorOutput()).WillOnce(Return(""));
            EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));
            EXPECT_CALL(*mockProcess, wait(_,_)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
            EXPECT_CALL(*mockProcess, getStatus()).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
            return std::unique_ptr<Common::Process::IProcess>(mockProcess);
        });

    std::string correlationId = "correlationID";
    CommandRequest cmd{
        .commands = { "echo -n 1", "echo -n 2" }, .timeout = 10, .ignoreError = true, .expiration = 99999999999
    };

    auto response = m_runCommandAction->runCommands(cmd, correlationId);
    EXPECT_EQ(response.type, "sophos.mgt.response.RunCommands");
    EXPECT_EQ(response.result, ResponseResult::SUCCESS);
    EXPECT_GT(response.startedAt, 1679055498);
    EXPECT_GE(response.duration, 0);

    EXPECT_EQ(response.commandResults.size(), 2);
    EXPECT_EQ(response.commandResults[0].stdOut, "output message");
    EXPECT_EQ(response.commandResults[0].stdErr, "");
    EXPECT_EQ(response.commandResults[0].exitCode, 0);
    EXPECT_GE(response.commandResults[0].duration, 0);

    EXPECT_EQ(response.commandResults[1].stdOut, "output message");
    EXPECT_EQ(response.commandResults[1].stdErr, "");
    EXPECT_EQ(response.commandResults[1].exitCode, 0);
    EXPECT_GE(response.commandResults[1].duration, 0);
}

TEST_F(RunCommandTests, runCommandsMultiplCommandsWithErrorsAndIgnoreErrorsTrue)
{
    Common::ProcessImpl::ProcessFactory::instance().replaceCreator(
        []()
        {
            static int runCount = 1;
            auto mockProcess = new StrictMock<MockProcess>();
            std::vector<std::string> args;
            args = { "-c", "echo -n " + std::to_string(runCount++) };
            EXPECT_CALL(*mockProcess, exec("/bin/bash", args, _)).Times(1);
            EXPECT_CALL(*mockProcess, standardOutput()).WillOnce(Return("output message"));
            EXPECT_CALL(*mockProcess, errorOutput()).WillOnce(Return(""));
            EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(1));
            EXPECT_CALL(*mockProcess, wait(_,_)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
            EXPECT_CALL(*mockProcess, getStatus()).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
            return std::unique_ptr<Common::Process::IProcess>(mockProcess);
        });

    std::string correlationId = "correlationID";
    CommandRequest cmd{
        .commands = { "echo -n 1", "echo -n 2" }, .timeout = 10, .ignoreError = true, .expiration = 99999999999
    };

    auto response = m_runCommandAction->runCommands(cmd, correlationId);
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
            auto mockProcess = new StrictMock<MockProcess>();
            std::vector<std::string> args;
            args = { "-c", "echo -n " + std::to_string(runCount++) };
            EXPECT_CALL(*mockProcess, exec("/bin/bash", args, _)).Times(1);
            EXPECT_CALL(*mockProcess, standardOutput()).WillOnce(Return("output message"));
            EXPECT_CALL(*mockProcess, errorOutput()).WillOnce(Return(""));
            EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(1));
            EXPECT_CALL(*mockProcess, wait(_,_)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
            EXPECT_CALL(*mockProcess, getStatus()).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
            return std::unique_ptr<Common::Process::IProcess>(mockProcess);
        });

    std::string correlationId = "correlationID";
    CommandRequest cmd{
        .commands = { "echo -n 1", "echo -n 2" }, .timeout = 10, .ignoreError = false, .expiration = 99999999999
    };

    auto response = m_runCommandAction->runCommands(cmd, correlationId);
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
    CommandRequest cmd{
        .commands = { "echo -n one" }, .timeout = 10, .ignoreError = true, .expiration = 123
    };

    auto response = m_runCommandAction->runCommands(cmd, correlationId);
    EXPECT_EQ(response.type, "sophos.mgt.response.RunCommands");
    EXPECT_EQ(response.result, ResponseResult::EXPIRED);
    EXPECT_EQ(response.startedAt, 0);
    EXPECT_GE(response.duration, 0);
    EXPECT_EQ(response.commandResults.size(), 0);
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
            auto mockProcess = new StrictMock<MockProcess>();
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
            return std::unique_ptr<Common::Process::IProcess>(mockProcess);
        });

    std::string correlationId = "correlationID";
    CommandRequest cmd{
        .commands = { "echo -n 1", "echo -n 2", "echo -n 3" }, .timeout = 10, .ignoreError = true, .expiration = 99999999999
    };
    EXPECT_EQ(cmd.commands.size(), 3);
    auto response = m_runCommandAction->runCommands(cmd, correlationId);
    EXPECT_EQ(response.commandResults.size(), 2);
}

// runCommand

TEST_F(RunCommandTests, runCommandWithNoErrors)
{
    Common::ProcessImpl::ProcessFactory::instance().replaceCreator(
        []()
        {
            auto mockProcess = new StrictMock<MockProcess>();
            std::vector<std::string> args = { "-c", "echo -n one" };
            EXPECT_CALL(*mockProcess, exec("/bin/bash", args, _)).Times(1);
            EXPECT_CALL(*mockProcess, standardOutput()).WillOnce(Return("output message"));
            EXPECT_CALL(*mockProcess, errorOutput()).WillOnce(Return(""));
            EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(0));
            EXPECT_CALL(*mockProcess, wait(_,_)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
            EXPECT_CALL(*mockProcess, getStatus()).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
            return std::unique_ptr<Common::Process::IProcess>(mockProcess);
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
            auto mockProcess = new StrictMock<MockProcess>();
            std::vector<std::string> args = { "-c", "echo -n one" };
            EXPECT_CALL(*mockProcess, exec("/bin/bash", args, _)).Times(1);
            EXPECT_CALL(*mockProcess, standardOutput()).WillOnce(Return("output message"));
            EXPECT_CALL(*mockProcess, errorOutput()).WillOnce(Return(""));
            EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(5));
            EXPECT_CALL(*mockProcess, wait(_,_)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
            EXPECT_CALL(*mockProcess, getStatus()).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
            return std::unique_ptr<Common::Process::IProcess>(mockProcess);
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
            auto mockProcess = new StrictMock<MockProcess>();
            std::vector<std::string> args = { "-c", "echo -n one" };
            EXPECT_CALL(*mockProcess, exec("/bin/bash", args, _)).Times(1);
            EXPECT_CALL(*mockProcess, standardOutput()).WillOnce(Return("output message"));
            EXPECT_CALL(*mockProcess, errorOutput()).WillOnce(Return("error message"));
            EXPECT_CALL(*mockProcess, exitCode()).WillOnce(Return(5));
            EXPECT_CALL(*mockProcess, wait(_,_)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
            EXPECT_CALL(*mockProcess, getStatus()).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
            return std::unique_ptr<Common::Process::IProcess>(mockProcess);
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
            auto mockProcess = new StrictMock<MockProcess>();
            std::vector<std::string> args = { "-c", "echo -n one" };
            EXPECT_CALL(*mockProcess, exec("/bin/bash", args, _)).Times(1);
            EXPECT_CALL(*mockProcess, standardOutput()).Times(1);
            EXPECT_CALL(*mockProcess, errorOutput()).Times(1);
            EXPECT_CALL(*mockProcess, exitCode()).Times(1);
            EXPECT_CALL(*mockProcess, getStatus()).WillOnce(Return(Common::Process::ProcessStatus::RUNNING));
            EXPECT_CALL(*mockProcess, kill()).Times(1);
            EXPECT_CALL(*mockProcess, waitUntilProcessEnds()).Times(1);
            return std::unique_ptr<Common::Process::IProcess>(mockProcess);
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
            auto mockProcess = new StrictMock<MockProcess>();
            std::vector<std::string> args = { "-c", "echo -n one" };
            EXPECT_CALL(*mockProcess, exec("/bin/bash", args, _)).Times(1);
            EXPECT_CALL(*mockProcess, standardOutput()).Times(1);
            EXPECT_CALL(*mockProcess, errorOutput()).Times(1);
            EXPECT_CALL(*mockProcess, exitCode()).Times(1);
            EXPECT_CALL(*mockProcess, wait(_,_)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
            EXPECT_CALL(*mockProcess, getStatus()).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
            return std::unique_ptr<Common::Process::IProcess>(mockProcess);
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
            auto mockProcess = new StrictMock<MockProcess>();
            std::vector<std::string> args = { "-c", "echo -n one" };
            EXPECT_CALL(*mockProcess, exec("/bin/bash", args, _)).Times(1);
            EXPECT_CALL(*mockProcess, standardOutput()).Times(1);
            EXPECT_CALL(*mockProcess, errorOutput()).Times(1);
            EXPECT_CALL(*mockProcess, exitCode()).Times(1);
            EXPECT_CALL(*mockProcess, getStatus()).WillOnce(Return(Common::Process::ProcessStatus::RUNNING));
            EXPECT_CALL(*mockProcess, kill()).Times(1);
            EXPECT_CALL(*mockProcess, waitUntilProcessEnds()).Times(1);
            return std::unique_ptr<Common::Process::IProcess>(mockProcess);
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
            auto mockProcess = new StrictMock<MockProcess>();
            std::vector<std::string> args = { "-c", "echo -n one" };
            EXPECT_CALL(*mockProcess, exec("/bin/bash", args, _)).Times(1);
            EXPECT_CALL(*mockProcess, standardOutput()).Times(1);
            EXPECT_CALL(*mockProcess, errorOutput()).Times(1);
            EXPECT_CALL(*mockProcess, exitCode()).Times(1);
            EXPECT_CALL(*mockProcess, wait(_,_)).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
            EXPECT_CALL(*mockProcess, getStatus()).WillOnce(Return(Common::Process::ProcessStatus::FINISHED));
            return std::unique_ptr<Common::Process::IProcess>(mockProcess);
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
            auto mockProcess = new StrictMock<MockProcess>();
            std::vector<std::string> args = { "-c", "echo -n one" };
            EXPECT_CALL(*mockProcess, exec("/bin/bash", args, _)).Times(1);
            EXPECT_CALL(*mockProcess, standardOutput()).Times(1);
            EXPECT_CALL(*mockProcess, errorOutput()).Times(1);
            EXPECT_CALL(*mockProcess, exitCode()).Times(1);
            EXPECT_CALL(*mockProcess, kill()).WillOnce(Return(true));
            EXPECT_CALL(*mockProcess, waitUntilProcessEnds()).Times(1);
            EXPECT_CALL(*mockProcess, getStatus()).WillOnce(Return(Common::Process::ProcessStatus::RUNNING));
            return std::unique_ptr<Common::Process::IProcess>(mockProcess);
        });

    m_runCommandAction->runCommand("echo -n one");

    EXPECT_TRUE(appenderContains("RunCommandAction has received termination command due to timeout"));
    EXPECT_TRUE(appenderContains("Child process killed as it took longer than 2 seconds to stop"));
}