// Copyright 2023 Sophos Limited. All rights reserved.

#include "RunCommandAction.h"

#include "ActionsUtils.h"
#include "InvalidCommandFormat.h"
#include "Logger.h"

#include "Common/Process/IProcess.h"
#include "Common/ProxyUtils/ProxyUtils.h"
#include "Common/UtilityImpl/TimeUtils.h"
#include "ResponseActions/RACommon/toUtf8Exception.h"

#include <sys/poll.h>

using namespace ResponseActionsImpl;

RunCommandAction::RunCommandAction(
    Common::ISignalHandlerSharedPtr sigHandler,
        Common::SystemCallWrapper::ISystemCallWrapperSharedPtr systemCallWrapper) :
    m_SignalHandler(std::move(sigHandler)),
    m_SysCallWrapper(std::move(systemCallWrapper))
{
}

nlohmann::json RunCommandAction::run(const std::string& actionJson, const std::string& correlationId)
{
    nlohmann::json response;
    response["type"] = ResponseActions::RACommon::RUN_COMMAND_RESPONSE_TYPE;
    try
    {
        CommandRequest action = ActionsUtils::readCommandAction(actionJson);

        // Check if command has expired
        if (ActionsUtils::isExpired(action.expiration))
        {
            std::string error = "Command " + correlationId + " has expired so will not be run.";
            LOGWARN(error);
            ActionsUtils::setErrorInfo(
                response, static_cast<int>(ResponseActions::RACommon::ResponseResult::EXPIRED), error);
            return response;
        }

        CommandResponse results = runCommands(action);
        // CommandResponse -> json
        response = results;
    }
    catch (const InvalidCommandFormat& ex)
    {
        LOGWARN(ex.what());
        ActionsUtils::setErrorInfo(
            response,
            static_cast<int>(ResponseActions::RACommon::ResponseResult::ERROR),
            "Error parsing command from Central: " + correlationId);
    }
    return response;
}

CommandResponse RunCommandAction::runCommands(const CommandRequest& action)
{
    CommandResponse response;

    // Record start time of commands, so we can work out overall duration
    Common::UtilityImpl::FormattedTime time;
    u_int64_t start = time.currentEpochTimeInSecondsAsInteger();
    response.startedAt = start;

    if (action.commands.size() > 1)
    {
        LOGINFO("Running " << action.commands.size() << " commands");
    }
    else
    {
        LOGINFO("Running " << action.commands.size() << " command");
    }

    // Counter used for logging only
    int cmdCounter = 1;

    // Run the commands and save the result
    for (const auto& command : action.commands)
    {
        LOGINFO("Running command " << cmdCounter);
        SingleCommandResult cmdResult = runCommand(command);
        response.commandResults.push_back(cmdResult);
        LOGINFO("Command " << cmdCounter << " exit code: " << cmdResult.exitCode);
        cmdCounter++;

        if (m_terminate || m_timeout)
        {
            break;
        }

        if (cmdResult.exitCode != 0 && !action.ignoreError)
        {
            if (action.commands.size() > response.commandResults.size())
            {
                LOGINFO("Got nonzero exit code from command and ignore errors is set to off, so not running "
                        "further commands for this action.");
            }
            break;
        }
    }

    if (m_timeout)
    {
        response.result = ResponseActions::RACommon::ResponseResult::TIMEOUT;
    }
    else
    {
        // Check if any commands did not exit with success (0 exitcode)
        bool anyErrors = std::find_if(
                             response.commandResults.cbegin(),
                             response.commandResults.cend(),
                             [](const auto& result) { return result.exitCode != 0; }) != response.commandResults.cend();

        response.result = anyErrors ? ResponseActions::RACommon::ResponseResult::ERROR
                                    : ResponseActions::RACommon::ResponseResult::SUCCESS;
    }
    LOGINFO("Overall command result: " << static_cast<int>(response.result));

    u_int64_t finish = time.currentEpochTimeInSecondsAsInteger();
    response.duration = finish - start;
    return response;
}

SingleCommandResult RunCommandAction::runCommand(const std::string& command)
{
    Common::UtilityImpl::FormattedTime time;
    auto start = std::chrono::high_resolution_clock::now();

    SingleCommandResult response;

    m_SignalHandler->clearSubProcessExitPipe();
    m_SignalHandler->clearTerminationPipe();
    m_SignalHandler->clearUSR1Pipe();

    auto process = ::Common::Process::createProcess();
    std::vector<std::string> cmd = { "-c", command };
    Common::Process::EnvPairVector env;
    env.emplace_back(std::make_pair<std::string, std::string>("LANG", "C"));
    process->exec("/bin/bash", cmd, env);

    assert(process != nullptr);

    struct pollfd fds[]{ { .fd = m_SignalHandler->terminationFileDescriptor(), .events = POLLIN, .revents = 0 },
                         { .fd = m_SignalHandler->subprocessExitFileDescriptor(), .events = POLLIN, .revents = 0 },
                         { .fd = m_SignalHandler->usr1FileDescriptor(), .events = POLLIN, .revents = 0 } };

    while (true)
    {
        auto ret = m_SysCallWrapper->ppoll(fds, std::size(fds), nullptr, nullptr);

        // Shouldnt be timing out
        assert(ret != 0);

        if (ret < 0)
        {
            int err = errno;
            if (err == EINTR)
            {
                LOGDEBUG("Ignoring EINTR from ppoll");
                continue;
            }
            LOGERROR(
                "Error from ppoll while waiting for command to finish: " << std::strerror(err) << "(" << err << ")");
            break;
        }
        else
        {
            if ((fds[0].revents & POLLIN) != 0)
            {
                LOGINFO("RunCommandAction has received termination command");
                m_terminate = true;
                break;
            }
            if ((fds[1].revents & POLLIN) != 0)
            {
                LOGDEBUG("Child Process has received termination command");
                // There is a delay between the child getting the signal and boost reporting the process as finished
                process->wait(Common::Process::Milliseconds{ 100 }, 10);
                break;
            }
            if ((fds[2].revents & POLLIN) != 0)
            {
                LOGINFO("RunCommandAction has received termination command due to timeout");
                m_timeout = true;
                break;
            }
        }
    }

    if (process->getStatus() != Common::Process::ProcessStatus::FINISHED)
    {
        LOGINFO("Child process is still running, killing process");
        if (process->kill())
        {
            LOGINFO("Child process killed as it took longer than 2 seconds to stop");
        }
        else
        {
            LOGINFO("Child process exited cleanly");
        }
        process->waitUntilProcessEnds();
    }

    std::optional<std::string> stdOutput = ResponseActions::RACommon::toUtf8(process->standardOutput());
    if (stdOutput.has_value())
    {
        response.stdOut = stdOutput.value();
    }
    else
    {
        LOGERROR("Failed to convert standard output to utf8");
        response.stdOut = "N/A due to conversion failure";
    }

    std::optional<std::string> errOutput = ResponseActions::RACommon::toUtf8(process->errorOutput());
    if (errOutput.has_value())
    {
        response.stdErr = errOutput.value();
    }
    else
    {
        LOGERROR("Failed to convert error output to utf8");
        response.stdErr = "N/A due to conversion failure";
    }

    response.exitCode = process->exitCode();

    // Unlike CommandResponse.duration, CommandResponse.SingleCommandResult.duration is in milliseconds not seconds
    auto finish = std::chrono::high_resolution_clock::now();
    response.duration = std::chrono::duration_cast<std::chrono::milliseconds>(finish - start).count();
    return response;
}