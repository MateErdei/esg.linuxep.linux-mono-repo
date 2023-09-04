// Copyright 2023 Sophos Limited. All rights reserved.

#include "ActionRunner.h"

#include "Logger.h"
#include "Telemetry.h"

#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "Common/FileSystem/IFilePermissions.h"
#include "Common/FileSystem/IFileSystem.h"

#include <json.hpp>

using namespace ResponseActions::RACommon;
using namespace ResponsePlugin::Telemetry;
using namespace Common::Process;

namespace ResponsePlugin
{
    ActionRunner::ActionRunner(std::shared_ptr<ResponsePlugin::TaskQueue> task)
        : m_task(std::move(task)) {}

    void sendFailedResponse(ResponseResult result, const std::string &requestType,
                        const std::string &correlationId)
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
        else if (requestType == DOWNLOAD_FILE_REQUEST_TYPE)
        {
            response["type"] = DOWNLOAD_FILE_RESPONSE_TYPE;
        }
        else
        {
            throw std::logic_error("Unknown action type provided to sendFailedResponse: " + requestType);
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
        m_isRunning = true;
        incrementTotalActions(type);

        m_fut = std::async(
            std::launch::async,
            [this, correlationId, action, type, timeout]()
            {
                std::string exePath =
                    Common::ApplicationConfiguration::applicationPathManager().getResponseActionRunnerPath();
                this->m_process = createProcess();
                LOGINFO(
                    "Trigger process at: " << exePath << " for action: " << correlationId
                                           << " with timeout: " << timeout);
                std::vector<std::string> arguments = { correlationId, action, type };
                this->m_process->exec(exePath, arguments, {});
                ProcessStatus processStatus = this->m_process->wait(std::chrono::seconds(timeout), 1);
                if (processStatus == ProcessStatus::TIMEOUT)
                {
                    LOGWARN(
                        "Action runner reached time out of " << timeout << " secs, correlation ID: " << correlationId);

                    this->m_process->sendSIGUSR1();
                    // Wait for process to handle signal and exit which should be done within 3 seconds
                    this->m_process->wait(std::chrono::seconds(3), 1);
                }

                if (this->m_process->getStatus() != ProcessStatus::FINISHED)
                {
                    kill("it carried on running unexpectedly");
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
                    incrementFailedActions(type);

                    if (code != static_cast<int>(ResponseActions::RACommon::ResponseResult::ERROR) &&
                        code != static_cast<int>(ResponseActions::RACommon::ResponseResult::EXPIRED))
                    {
                        auto result = ResponseResult::INTERNAL_ERROR;
                        if (processStatus == ProcessStatus::TIMEOUT)
                        {
                            result = ResponseResult::TIMEOUT;
                            incrementTimedOutActions(type);
                        }
                        sendFailedResponse(result, type, correlationId);
                    }
                    if (code == static_cast<int>(ResponseActions::RACommon::ResponseResult::EXPIRED))
                    {
                        incrementExpiredActions(type);
                    }
                }
                LOGINFO("Finished action: " << correlationId);
                m_isRunning = false;
          m_task->push(ResponsePlugin::Task{
              ResponsePlugin::Task::TaskType::CHECK_QUEUE, ""});
        });
    }

    void ActionRunner::killAction()
    {
        kill("plugin received stop request");
        awaitPostAction();
    }

    bool ActionRunner::getIsRunning()
    {
        return m_isRunning;
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

    void ActionRunner::awaitPostAction()
    {
        using namespace std::chrono_literals;

        auto status = m_fut.wait_for(1s);
        if (status != std::future_status::ready)
        {
            LOGWARN("Failed to complete post-action work in 1 second");
        }
    }
} // namespace ResponsePlugin