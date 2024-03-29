// Copyright 2021-2023 Sophos Limited. All rights reserved.
#pragma once

#include "DiagnoseRunner.h"
#include "IAsyncDiagnoseRunner.h"

#include "sdu/taskQueue/ITaskQueue.h"

#include <future>
#include <memory>

namespace RemoteDiagnoseImpl::runnerModule
{
    class AsyncDiagnoseRunner : public virtual RemoteDiagnoseImpl::IAsyncDiagnoseRunner
    {
    public:
        AsyncDiagnoseRunner(std::shared_ptr<RemoteDiagnoseImpl::ITaskQueue>, const std::string& dirPath);

        void triggerDiagnose() override;

        bool isRunning() override;

        void triggerAbort() override;

        bool hasTimedOut() override;

    private:
        std::string m_dirPath;
        std::shared_ptr<RemoteDiagnoseImpl::ITaskQueue> m_taskQueue;
        std::unique_ptr<DiagnoseRunner> m_diagnoseRunner;
        std::future<void> m_diagnoseExecHandle;

        // used to check the timeout so that the diagnose can be forced to restart
        std::chrono::system_clock::time_point m_diagnoseRunnerStartTime;
    };
} // namespace RemoteDiagnoseImpl::runnerModule
