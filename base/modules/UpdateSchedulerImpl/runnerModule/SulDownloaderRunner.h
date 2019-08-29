/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "SulDownloaderResultDirectoryListener.h"

#include <Common/DirectoryWatcher/IDirectoryWatcher.h>
#include <Common/Process/IProcess.h>
#include <UpdateScheduler/SchedulerTaskQueue.h>

namespace UpdateSchedulerImpl
{
    namespace runnerModule
    {
        class SulDownloaderRunner
        {
        public:
            SulDownloaderRunner(
                std::shared_ptr<UpdateScheduler::SchedulerTaskQueue> schedulerTaskQueue,
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
            SulDownloaderResultDirectoryListener m_listener;
            std::unique_ptr<Common::DirectoryWatcher::IDirectoryWatcher> m_directoryWatcher;
            std::shared_ptr<UpdateScheduler::SchedulerTaskQueue> m_schedulerTaskQueue;
            std::chrono::seconds m_timeout;

            std::tuple<int, std::string> startUpdateService();
            void logIfUpdateServiceFailed();
        };

    } // namespace runnerModule
} // namespace UpdateSchedulerImpl