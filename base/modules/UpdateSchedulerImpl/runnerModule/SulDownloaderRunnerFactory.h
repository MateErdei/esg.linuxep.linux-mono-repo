/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <UpdateScheduler/IAsyncSulDownloaderRunner.h>
#include <UpdateScheduler/SchedulerTaskQueue.h>
#include <functional>

namespace UpdateSchedulerImpl
{


    class SulDownloaderRunnerFactory
    {
        SulDownloaderRunnerFactory();

    public:
        using FunctionType = std::function<
                std::unique_ptr<IAsyncSulDownloaderRunner>(std::shared_ptr<SchedulerTaskQueue>, const std::string&)>;

        static SulDownloaderRunnerFactory& instance();

        std::unique_ptr<IAsyncSulDownloaderRunner> createSulDownloaderRunner(
                std::shared_ptr<SchedulerTaskQueue> schedulerTaskQueue, const std::string& dirPath);

        // for tests only
        void replaceCreator(FunctionType creator);

        void restoreCreator();

    private:
        FunctionType m_creator;
    };
}