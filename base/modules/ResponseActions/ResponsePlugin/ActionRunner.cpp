// Copyright 2023 Sophos Limited. All rights reserved.

#include "ActionRunner.h"

#include "Logger.h"

#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
namespace ResponsePlugin
{

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
                LOGINFO("Trigger process at: " << exePath << " for action : " << correlationId);
                std::vector<std::string> arguments = { correlationId, action, type };
                this->m_process->exec(exePath, arguments, {});
                Common::Process::ProcessStatus processStatus = this->m_process->wait(std::chrono::seconds(timeout), 1);
                if (processStatus != Common::Process::ProcessStatus::FINISHED)
                {
                    this->m_process->kill();
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
                if (code != 0)
                {
                    LOGWARN("Failed action " << correlationId << " with exit code " << code);
                }
                else
                {
                    LOGINFO("Action " << correlationId << " has succeeded");
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