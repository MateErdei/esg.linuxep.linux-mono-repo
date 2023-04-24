// Copyright 2023 Sophos Limited. All rights reserved.

#include "ActionRunner.h"

#include "Logger.h"
#include "Telemetry.h"
#include "json.hpp"

#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "Common/FileSystem/IFilePermissions.h"
#include "Common/FileSystem/IFileSystem.h"

namespace ResponsePlugin
{

    void sendFailedResponse(
        ResponseActions::RACommon::ResponseResult result,
        const std::string& requestType,
        const std::string& correlationId)
    {
        LOGINFO("Response Actions plugin sending failed response to Central on behalf of Action Runner process");
        nlohmann::json response;
        if (requestType == ResponseActions::RACommon::RUN_COMMAND_REQUEST_TYPE)
        {
            response["type"] = ResponseActions::RACommon::RUN_COMMAND_RESPONSE_TYPE;
        }
        else if (requestType == ResponseActions::RACommon::UPLOAD_FILE_REQUEST_TYPE)
        {
            response["type"] = ResponseActions::RACommon::UPLOAD_FILE_RESPONSE_TYPE;
        }
        else if (requestType == ResponseActions::RACommon::UPLOAD_FOLDER_REQUEST_TYPE)
        {
            response["type"] = ResponseActions::RACommon::UPLOAD_FOLDER_RESPONSE_TYPE;
        }
        response["result"] = static_cast<int>(result);
        ResponseActions::RACommon::sendResponse(correlationId, response.dump());
    }

    void ActionRunner::runAction(
        const std::string& action,
        const std::string& correlationId,
        const std::string& type,
        int timeout)
    {
        isRunning = true;
        TelemetryUtils::incrementTotalActions(type);

        m_fut = std::async(
            std::launch::async,
            [this, correlationId, action, type, timeout]()
            {
                std::string exePath =
                    Common::ApplicationConfiguration::applicationPathManager().getResponseActionRunnerPath();
                this->m_process = Common::Process::createProcess();
                LOGINFO("Trigger process at: " << exePath << " for action: " << correlationId);
                std::vector<std::string> arguments = { correlationId, action, type };
                this->m_process->exec(exePath, arguments, {});
                Common::Process::ProcessStatus processStatus = this->m_process->wait(std::chrono::seconds(timeout), 1);
                bool timedOut = false;
                if (processStatus != Common::Process::ProcessStatus::FINISHED)
                {
                    this->m_process->kill();
                    timedOut = true;
                    LOGWARN(
                        "Action process was stopped due to a timeout after "
                        << timeout << " secs, correlation ID: " << correlationId);
                }
                auto output = this->m_process->output();
                if (!output.empty())
                {
                    LOGINFO("output: " << output);
                }
                auto code = this->m_process->exitCode();
                if (code == 0)
                {
                    LOGINFO("Action " << correlationId << " has succeeded");
                }
                else
                {
                    LOGWARN("Failed action " << correlationId << " with exit code " << code);
                    TelemetryUtils::incrementFailedActions(type);

                    if (code != 1)
                    {
                        ResponseActions::RACommon::ResponseResult result;
                        if (timedOut)
                        {
                            result = ResponseActions::RACommon::ResponseResult::TIMEOUT;
                            TelemetryUtils::incrementTimedOutActions(type);
                        }
                        else if (code == 4)
                        {
                            result = ResponseActions::RACommon::ResponseResult::EXPIRED;
                            TelemetryUtils::incrementExpiredActions(type);
                        }
                        else
                        {
                            result = ResponseActions::RACommon::ResponseResult::INTERNAL_ERROR;
                        }
                        sendFailedResponse(result, type, correlationId);
                    }
                }
                isRunning = false;
            });
    }

    void ActionRunner::killAction()
    {
        m_process->kill();
    }

    bool ActionRunner::getIsRunning()
    {
        return isRunning;
    }
} // namespace ResponsePlugin