// Copyright 2023 Sophos Limited. All rights reserved.

#include "ActionRunner.h"

#include "Logger.h"
#include "json.hpp"

#include "Common/FileSystem/IFilePermissions.h"
#include "Common/FileSystem/IFileSystem.h"
#include "Common/ApplicationConfiguration/IApplicationPathManager.h"

using namespace ResponseActions::RACommon;
using namespace Common::Process;

namespace ResponsePlugin
{
    void sendFailedResponse(
        ResponseResult result,
        const std::string& requestType,
        const std::string& correlationId)
    {
        LOGINFO("Response Actions plugin sending failed response to Central on behalf of Action Runner process");
        nlohmann::json response;
        if (requestType == RUN_COMMAND_REQUEST_TYPE)
        {
            response["type"] = RUN_COMMAND_RESPONSE_TYPE;
        }
        else if (requestType == UPLOAD_FILE_REQUEST_TYPE)
        {
            response["type"] = UPLOAD_FILE_RESPONSE_TYPE;
        }
        else if (requestType == UPLOAD_FOLDER_REQUEST_TYPE)
        {
            response["type"] = UPLOAD_FOLDER_RESPONSE_TYPE;
        }
        response["result"] = static_cast<int>(result);
        sendResponse(correlationId, response.dump());
    }

    void ActionRunner::runAction(
        const std::string& action,
        const std::string& correlationId,
        const std::string& type,
        int timeout)
    {
        isRunning = true;

        m_fut = std::async(
            std::launch::async,
            [this, correlationId, action, type, timeout]()
            {
                std::string exePath =
                    Common::ApplicationConfiguration::applicationPathManager().getResponseActionRunnerPath();
                this->m_process = createProcess();
                LOGINFO("Trigger process at: " << exePath << " for action: " << correlationId);
                std::vector<std::string> arguments = { correlationId, action, type };
                this->m_process->exec(exePath, arguments, {});
                ProcessStatus processStatus = this->m_process->wait(std::chrono::seconds(timeout), 1);
                bool timedOut = false;
                if (processStatus != ProcessStatus::FINISHED)
                {
                    std::stringstream msg;
                    msg << "reaching timeout of " << timeout << " secs, correlation ID: " << correlationId;
                    this->m_process->sendSIGUSR1();
                    //Wait for process to handle signal and exit which should be done within 3 seconds
                    this->m_process->wait(std::chrono::seconds(3), 1);
                    if (this->m_process->getStatus() != ProcessStatus::FINISHED)
                    {
                        timedOut = !kill(msg.str());
                    }
                    else
                    {
                        LOGWARN("Action Runner was stopped after " + msg.str());
                        timedOut = true;
                    }
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

                    if (code != 1 && code != 4)
                    {
                        auto result = timedOut ? ResponseActions::RACommon::ResponseResult::TIMEOUT
                                               : ResponseActions::RACommon::ResponseResult::INTERNAL_ERROR;
                        sendFailedResponse(result, type, correlationId);
                    }
                }
                isRunning = false;
            });
    }

    void ActionRunner::killAction()
    {
        kill("plugin received stop request");
    }

    bool ActionRunner::getIsRunning()
    {
        return isRunning;
    }

    bool ActionRunner::kill(const std::string& msg)
    {
        if (this->m_process->kill())
        {
            LOGWARN("Action Runner had to be killed after " + msg);
            return true;
        }
        else
        {
            LOGWARN("Action Runner was stopped after " + msg);
            return false;
        }
    }

} // namespace ResponsePlugin