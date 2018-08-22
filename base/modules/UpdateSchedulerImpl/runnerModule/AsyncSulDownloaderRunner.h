/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <UpdateScheduler/IAsyncSulDownloaderRunner.h>
#include <UpdateScheduler/SchedulerTaskQueue.h>
#include "SulDownloaderRunner.h"
#include <memory>
#include <future>

namespace UpdateSchedulerImpl
{
    class AsyncSulDownloaderRunner
            : public virtual IAsyncSulDownloaderRunner
    {
    public:
        AsyncSulDownloaderRunner(std::shared_ptr<SchedulerTaskQueue>, const std::string& dirPath);

        void triggerSulDownloader() override;

        bool isRunning() override;

        void triggerAbort() override;

    private:
        std::string m_dirPath;
        std::shared_ptr<SchedulerTaskQueue> m_taskQueue;
        std::unique_ptr<SulDownloaderRunner> m_sulDownloaderRunner;
        std::future<void> m_sulDownloaderExecHandle;
    };
}



