/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once
#include "SchedulerTaskQueue.h"
#include <chrono>
#include <string>
#include <memory>


namespace UpdateScheduler
{
    class ISulDownloaderRunner
    {
    public:
        virtual ~ISulDownloaderRunner() = default;
        virtual void run() = 0;
        virtual void abortWaitingForReport() = 0;
    };

    using ISulDownloaderRunnerPtr = std::unique_ptr<ISulDownloaderRunner>;
    extern ISulDownloaderRunnerPtr createSulDownloaderRunner(std::shared_ptr<SchedulerTaskQueue> schedulerTaskQueue,
                                                             const std::string& directoryToWatch,
                                                             const std::string& nameOfFileToWaitFor,
                                                             std::chrono::seconds timeout);

}