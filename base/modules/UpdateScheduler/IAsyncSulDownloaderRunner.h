/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include "SchedulerTaskQueue.h"

namespace UpdateScheduler
{
    class IAsyncSulDownloaderRunner
    {
    public:
        virtual ~IAsyncSulDownloaderRunner() = default;

        virtual void triggerSulDownloader() = 0;

        virtual bool isRunning() = 0;

        virtual void triggerAbort() = 0;
    };

    std::unique_ptr<IAsyncSulDownloaderRunner> createSulDownloaderRunner(
        std::shared_ptr<SchedulerTaskQueue>,
        std::string dirPath);

} // namespace UpdateScheduler