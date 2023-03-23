// Copyright 2023 Sophos Limited. All rights reserved.

#include "RunCommandAction.h"

#include "ActionsUtils.h"
#include "InvalidCommandFormat.h"
#include "Logger.h"

#include "Common/ProxyUtils/ProxyUtils.h"
#include "Process/IProcess.h"
#include "UtilityImpl/TimeUtils.h"

namespace ResponseActionsImpl
{
    void to_json(nlohmann::json& j, const SingleCommandResult& singleCommandResult)
    {
        j = nlohmann::json{ { "duration", singleCommandResult.duration },
                            { "exitCode", singleCommandResult.exitCode },
                            { "stdErr", singleCommandResult.stdErr },
                            { "stdOut", singleCommandResult.stdOut } };
    }

    void to_json(nlohmann::json& j, const CommandResponse& cmdResponse)
    {
        j = nlohmann::json{ { "type", cmdResponse.type },
                            { "result", cmdResponse.result },
                            { "commandResults", cmdResponse.commandResults },
                            { "duration", cmdResponse.duration },
                            { "startedAt", cmdResponse.startedAt } };
    }

    namespace RunCommandAction
    {
        std::string run(const std::string& actionJson, const std::string& correlationId)
        {
            nlohmann::json response;
            try
            {
                ResponseActionsImpl::CommandRequest action = parseCommandAction(actionJson);
                CommandResponse results = runCommands(action, correlationId);
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
            return response.dump();
        }

        CommandResponse runCommands(const CommandRequest& action, const std::string& correlationId)
        {
            CommandResponse response;

            // Check if command has expired
            if (ActionsUtils::isExpired(action.expiration))
            {
                LOGWARN("Command " << correlationId << " has expired so will not be run.");
                response.result = ResponseActions::RACommon::ResponseResult::EXPIRED;
                return response;
            }

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
                ResponseActionsImpl::SingleCommandResult cmdResult = RunCommandAction::runCommand(command);
                response.commandResults.push_back(cmdResult);
                LOGINFO("Command " << cmdCounter << " exit code: " << cmdResult.exitCode);
                cmdCounter++;
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

            // Check if any commands did not exit with success (0 exitcode)
            bool anyErrors =
                std::find_if(
                    response.commandResults.cbegin(),
                    response.commandResults.cend(),
                    [](const auto& result) { return result.exitCode != 0; }) != response.commandResults.cend();

            response.result = anyErrors ? ResponseActions::RACommon::ResponseResult::ERROR
                                        : ResponseActions::RACommon::ResponseResult::SUCCESS;
            LOGINFO("Overall command result: " << static_cast<int>(response.result));

            u_int64_t finish = time.currentEpochTimeInSecondsAsInteger();
            response.duration = finish - start;
            return response;
        }

        ResponseActionsImpl::SingleCommandResult runCommand(const std::string& command)
        {
            Common::UtilityImpl::FormattedTime time;
            u_int64_t start = time.currentEpochTimeInSecondsAsInteger();

            ResponseActionsImpl::SingleCommandResult response;

            auto process = ::Common::Process::createProcess();
            std::vector<std::string> cmd = { "-c", command };
            process->exec("/bin/bash", cmd);

            // Currently RA Plugin will kill this process so we can just wait indefinitely
            // TODO LINUXDAR-7020 add signal handler https://sophos.atlassian.net/browse/LINUXDAR-7020
            process->waitUntilProcessEnds();

            response.stdOut = process->standardOutput();
            response.stdErr = process->errorOutput();
            response.exitCode = process->exitCode();

            u_int64_t finish = time.currentEpochTimeInSecondsAsInteger();
            response.duration = finish - start;
            return response;
        }

        ResponseActionsImpl::CommandRequest parseCommandAction(const std::string& actionJson)
        {
            ResponseActionsImpl::CommandRequest action;
            nlohmann::json actionObject;
            if (actionJson.empty())
            {
                throw InvalidCommandFormat("Run command action JSON is empty");
            }

            try
            {
                actionObject = nlohmann::json::parse(actionJson);
            }
            catch (const nlohmann::json::exception& exception)
            {
                throw InvalidCommandFormat(
                    "Cannot parse run command action with JSON error: " + std::string(exception.what()));
            }

            // Check all required keys are present
            std::vector<std::string> requiredKeys = { "type", "commands", "timeout", "ignoreError", "expiration" };
            for (const auto& key : requiredKeys)
            {
                if (!actionObject.contains(key))
                {
                    throw InvalidCommandFormat("No '" + key + "' in run command action JSON");
                }
            }

            try
            {
                action.commands = actionObject["commands"].get<std::vector<std::string>>();
                action.timeout = actionObject.at("timeout");
                action.ignoreError = actionObject.at("ignoreError");
                action.expiration = actionObject.at("expiration");
            }
            catch (const std::exception& exception)
            {
                throw InvalidCommandFormat(
                    "Failed to create Command Request object from run command JSON: " + std::string(exception.what()));
            }

            if (action.commands.empty())
            {
                throw InvalidCommandFormat("No commands to perform in run command JSON: " + actionJson);
            }

            return action;
        }
    } // namespace RunCommandAction
} // namespace ResponseActionsImpl