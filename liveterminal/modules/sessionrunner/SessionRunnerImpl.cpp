// Copyright 2023 Sophos Limited. All rights reserved.

#include "SessionRunnerImpl.h"
#include "Logger.h"
#include "common/TelemetryConsts.h"
#include "Common/Process/IProcess.h"
#include "Common/TelemetryHelperImpl/TelemetryHelper.h"

#include <string.h>

namespace sessionrunner
{
    SessionRunnerImpl::SessionRunnerImpl(const std::string& executablePath) : m_executablePath(executablePath)
    {
        m_runnerStatus.errorCode = ErrorCode::UNEXPECTEDERROR;
    }
    void SessionRunnerImpl::triggerSession(const std::string& id, std::function<void(std::string id)> notifyFinished)
    {
        m_id = id;
        m_fut = std::async(
            std::launch::async,
            [this, id, notifyFinished]()
            {
                try
                {
                    this->m_process = Common::Process::createProcess();
                    LOGINFO("Trigger live response session with id: " << id);
                    std::vector<std::string> arguments = { "--id", id };
                    this->m_process->exec(this->m_executablePath, arguments, {});
                    auto code = this->m_process->exitCode();
                    auto output = this->m_process->output();
                    this->setStatus(code, output);
                }
                catch (std::exception& ex)
                {
                    LOGERROR("Error while running LiveResponse: " << ex.what());
                    this->setStatus(1, "");
                }
                notifyFinished(id);
            });
    }
    std::string SessionRunnerImpl::id()
    {
        return m_id;
    }
    void SessionRunnerImpl::setStatusFromExitResult(
        const std::string& id,
        SessionRunnerStatus& sessionRunnerStatus,
        int exitCode,
        const std::string& output)
    {
        if (exitCode != 0)
        {
            if (output.empty())
            {
                LOGERROR(
                    "Execution of live response session id '" << id << "' failed with error code: " << exitCode << ", "
                                                              << strsignal(exitCode));
            }
            else
            {
                LOGERROR(
                    "Execution of live response session id '" << id << "' failed with error code: " << exitCode << ", "
                                                              << strsignal(exitCode) << " and output: " << output);
            }
            sessionRunnerStatus.errorCode = ErrorCode::UNEXPECTEDERROR;
            Common::Telemetry::TelemetryHelper::getInstance().increment(Plugin::Telemetry::failedSessionsTag, 1L);
        }
        else
        {
            sessionRunnerStatus.errorCode = ErrorCode::SUCCESS;
        }
    }

    void SessionRunnerImpl::setStatus(int exitCode, const std::string& output)
    {
        std::lock_guard<std::mutex> l{ m_mutex };
        setStatusFromExitResult(m_id, m_runnerStatus, exitCode, output);
    }

    void SessionRunnerImpl::requestAbort()
    {
        m_process->kill();
    };

    std::unique_ptr<ISessionRunner> SessionRunnerImpl::clone()
    {
        return std::unique_ptr<ISessionRunner>{ new SessionRunnerImpl(m_executablePath) };
    }
} // namespace sessionrunner

std::unique_ptr<sessionrunner::ISessionRunner> sessionrunner::createSessionRunner(const std::string& executablePath)
{
    return std::unique_ptr<sessionrunner::ISessionRunner>(new SessionRunnerImpl(executablePath));
}