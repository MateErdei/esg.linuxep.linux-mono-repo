// Copyright 2021-2023 Sophos Limited. All rights reserved.

#pragma once

#include "DiagnoseResultDirectoryListener.h"

#include "Common/DirectoryWatcher/IDirectoryWatcher.h"
#include "sdu/taskQueue/ITaskQueue.h"

namespace RemoteDiagnoseImpl::runnerModule
{
    class DiagnoseRunner
    {
    public:
        DiagnoseRunner(
            std::shared_ptr<RemoteDiagnoseImpl::ITaskQueue> schedulerTaskQueue,
            const std::string& directoryToWatch,
            const std::string& nameOfFileToWaitFor,
            std::chrono::seconds timeout);

        /// Run the systemd update service and block until suldownloader outputs a report file or the timeout is
        /// reached.
        void run();

        /// Stop waiting for the update service to finish.
        /// The service will not be terminated and the report file may still be created.
        void abortWaitingForReport();

    private:
        DiagnoseResultDirectoryListener m_listener;
        std::unique_ptr<Common::DirectoryWatcher::IDirectoryWatcher> m_directoryWatcher;
        std::shared_ptr<RemoteDiagnoseImpl::ITaskQueue> m_schedulerTaskQueue;
        std::chrono::seconds m_timeout;

        std::tuple<int, std::string> startDiagnoseService();
        void logIfDiagnoseServiceFailed();
    };
} // namespace RemoteDiagnoseImpl::runnerModule