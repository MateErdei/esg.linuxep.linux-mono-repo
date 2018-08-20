/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "SchedulerTaskQueue.h"
#include "ISulDownloaderRunner.h"
#include "SulDownloaderResultDirectoryListener.h"
#include <Common/DirectoryWatcher/IDirectoryWatcher.h>
#include <Common/DirectoryWatcherImpl/DirectoryWatcherImpl.h>
#include <Common/Process/IProcess.h>
#include <functional>


namespace UpdateScheduler
{
    class SulDownloaderRunner : public  virtual ISulDownloaderRunner
    {
    public:
        SulDownloaderRunner(
                std::shared_ptr<SchedulerTaskQueue> schedulerTaskQueue,
                const std::string& directoryToWatch,
                const std::string& nameOfFileToWaitFor,
                std::chrono::seconds timeout);

        /// Run the systemd update service and block until suldownloader outputs a report file or the timeout is reached.
        void run();

        /// Stop waiting for the update service to finish.
        /// The service will not be terminated and the report file may still be created.
        void abortWaitingForReport();

    private:
        SulDownloaderResultDirectoryListener m_listener;
        std::unique_ptr<Common::DirectoryWatcher::IDirectoryWatcher> m_directoryWatcher;
        std::shared_ptr<SchedulerTaskQueue> m_schedulerTaskQueue;
        std::chrono::seconds m_timeout;
        std::tuple<int, std::string> startUpdateService();
    };

    class SulDownloaderRunnerFactory
    {
        SulDownloaderRunnerFactory();

    public:
        using FunctionType = std::function<std::unique_ptr<ISulDownloaderRunner>(std::shared_ptr<SchedulerTaskQueue>, const std::string& ,
                                                                                 const std::string&, std::chrono::seconds)>;

        static SulDownloaderRunnerFactory & instance();
        std::unique_ptr<ISulDownloaderRunner> createSulDownloaderRunner(
                std::shared_ptr<SchedulerTaskQueue> schedulerTaskQueue,
                const std::string& directoryToWatch,
                const std::string& nameOfFileToWaitFor,
                std::chrono::seconds timeout);
        // for tests only
        void replaceCreator(FunctionType creator);

        void restoreCreator();

    private:
        FunctionType m_creator;
    };
}