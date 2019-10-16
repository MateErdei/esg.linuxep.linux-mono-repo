/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include "SulDownloaderRunner.h"

#include <UpdateScheduler/IAsyncSulDownloaderRunner.h>
#include <UpdateScheduler/SchedulerTaskQueue.h>

#include <future>
#include <memory>

namespace UpdateSchedulerImpl
{
    namespace runnerModule
    {
        class AsyncSulDownloaderRunner : public virtual UpdateScheduler::IAsyncSulDownloaderRunner
        {
        public:
            AsyncSulDownloaderRunner(std::shared_ptr<UpdateScheduler::SchedulerTaskQueue>, const std::string& dirPath);

            void triggerSulDownloader() override;

            bool isRunning() override;

            void triggerAbort() override;

            bool hasTimedOut() override;

        private:
            std::string m_dirPath;
            std::shared_ptr<UpdateScheduler::SchedulerTaskQueue> m_taskQueue;
            std::unique_ptr<SulDownloaderRunner> m_sulDownloaderRunner;
            std::future<void> m_sulDownloaderExecHandle;

            // used to check the timeout so that the sulDownloader can be forced to restart
            std::chrono::system_clock::time_point m_sulDownloaderRunnerStartTime;
        };
    } // namespace runnerModule
} // namespace UpdateSchedulerImpl
