/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once
#include "ISessionRunner.h"

#include <Common/Process/IProcess.h>

#include <future>
namespace sessionrunner
{
    class SessionRunnerImpl : public ISessionRunner
    {
        std::string m_executablePath;
        Common::Process::IProcessPtr m_process;
        std::string m_id;
        std::future<void> m_fut;
        std::mutex m_mutex;
        SessionRunnerStatus m_runnerStatus;

    public:
        static void setStatusFromExitResult(const std::string& id, SessionRunnerStatus& sessionRunnerStatus, int exitCode, const std::string& output);
        explicit SessionRunnerImpl(const std::string& executablePath);
        ~SessionRunnerImpl(){};
        void triggerSession(const std::string& id, std::function<void(std::string id)> notifyFinished) override;
        virtual std::string id() override;
        void requestAbort() override;
        virtual std::unique_ptr<ISessionRunner> clone() override;

    private:
        void setStatus(int exitCode, const std::string& output);
    };
} // namespace sessionrunner
