// Copyright 2023 Sophos Limited. All rights reserved.

#include "ActionRunner.h"

#include "Logger.h"
#include "json.hpp"

#include "Common/FileSystem/IFilePermissions.h"
#include "Common/FileSystem/IFileSystem.h"
#include "Common/ApplicationConfiguration/IApplicationPathManager.h"

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
                    std::stringstream msg;
                    msg << "reaching timeout of " << timeout << " secs, correlation ID: " << correlationId;
                    timedOut = !kill(msg.str());
                }
                auto output = this->m_process->output();
                if (!output.empty())
                {
                    LOGINFO("output: " << output);
                }
                auto code = this->m_process->exitCode();
                if (code != 0)
                {
                    LOGWARN("Failed action " << correlationId << " with exit code " << code);

                    auto result = timedOut ? ResponseActions::RACommon::ResponseResult::TIMEOUT
                                           : ResponseActions::RACommon::ResponseResult::INTERNAL_ERROR;
                    sendFailedResponse(result, type, correlationId);
                }
                else
                {
                    LOGINFO("Action " << correlationId << " has succeeded");
                }
                isRunning = false;
            });
        //TODO Remove
        m_fut.get();
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
        //TODO Test this
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