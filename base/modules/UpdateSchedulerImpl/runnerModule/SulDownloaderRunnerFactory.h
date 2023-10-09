// Copyright 2018-2023 Sophos Limited. All rights reserved.
#pragma once

#include "UpdateScheduler/IAsyncSulDownloaderRunner.h"
#include "UpdateScheduler/SchedulerTaskQueue.h"

#include <functional>

namespace UpdateSchedulerImpl
{
    namespace runnerModule
    {
        class SulDownloaderRunnerFactory
        {
            SulDownloaderRunnerFactory();

        public:
            using FunctionType = std::function<std::unique_ptr<UpdateScheduler::IAsyncSulDownloaderRunner>(
                std::shared_ptr<UpdateScheduler::SchedulerTaskQueue>,
                const std::string&)>;

            static SulDownloaderRunnerFactory& instance();

            std::unique_ptr<UpdateScheduler::IAsyncSulDownloaderRunner> createSulDownloaderRunner(
                std::shared_ptr<UpdateScheduler::SchedulerTaskQueue> schedulerTaskQueue,
                const std::string& dirPath);

            // for tests only
            void replaceCreator(FunctionType creator);

            void restoreCreator();

        private:
            FunctionType m_creator;
        };

    } // namespace runnerModule
} // namespace UpdateSchedulerImpl