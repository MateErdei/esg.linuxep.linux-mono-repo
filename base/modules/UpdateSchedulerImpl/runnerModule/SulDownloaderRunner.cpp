/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SulDownloaderRunner.h"

#include "../Logger.h"

#include <watchdog/watchdogimpl/IWatchdogRequest.h>

namespace UpdateSchedulerImpl
{
    namespace runnerModule
    {
        using namespace UpdateScheduler;

        SulDownloaderRunner::SulDownloaderRunner(
            std::shared_ptr<SchedulerTaskQueue> schedulerTaskQueue,
            const std::string& directoryToWatch,
            const std::string& nameOfFileToWaitFor,
            std::chrono::seconds timeout) :
            m_listener(directoryToWatch, nameOfFileToWaitFor),
            m_directoryWatcher(Common::DirectoryWatcher::createDirectoryWatcher()),
            m_schedulerTaskQueue(std::move(schedulerTaskQueue)),
            m_timeout(timeout)
        {
            m_directoryWatcher->addListener(m_listener);
        }

        void SulDownloaderRunner::run()
        {
            m_directoryWatcher->startWatch();
            SchedulerTask schedulerTask;

            // Run SUL Downloader service, report error and return if it fails to start.
            std::tuple<int, std::string> startUpdateServiceResult = startUpdateService();
            if (std::get<0>(startUpdateServiceResult) != 0)
            {
                schedulerTask.taskType = SchedulerTask::TaskType::SulDownloaderFailedToStart;
                schedulerTask.content = std::get<1>(startUpdateServiceResult);
                m_schedulerTaskQueue->push(schedulerTask);
                LOGINFO("Update Service failed to start with error: " + std::get<1>(startUpdateServiceResult));
                return;
            }

            // Wait for the SUL Downloader results file to be created or a timeout or abortWaitingForReport is called.
            std::string reportFileLocation = m_listener.waitForFile(m_timeout);

            // If the file failed to be created it means either an abortWaitingForReport was called or the wait timed
            // out and no file was found.
            if (reportFileLocation.empty())
            {
                if (m_listener.wasAborted())
                {
                    schedulerTask.taskType = SchedulerTask::TaskType::SulDownloaderMonitorDetached;
                    LOGINFO("Update Service not monitoring SulDownloader execution.");
                }
                else
                {
                    schedulerTask.taskType = SchedulerTask::TaskType::SulDownloaderTimedOut;
                    LOGWARN("Update Service timed out.");
                }

                m_schedulerTaskQueue->push(schedulerTask);
                return;
            }

            logIfUpdateServiceFailed();

            // If update result was successfully generated within timeout period then report file location and return.
            schedulerTask.taskType = SchedulerTask::TaskType::SulDownloaderFinished;
            schedulerTask.content = reportFileLocation;
            m_schedulerTaskQueue->push(schedulerTask);
            LOGSUPPORT("Update Service finished.");
        }

        void SulDownloaderRunner::logIfUpdateServiceFailed()
        {
            auto process = Common::Process::createProcess();
            process->exec("/bin/systemctl", { "is-failed", "sophos-spl-update.service" });
            auto result = std::make_tuple(process->exitCode(), process->output());
            int exitCode = std::get<0>(result);

            if (exitCode == 0)
            {
                LOGERROR("Update Service (sophos-spl-update.service) failed.");
            }
        }

        std::tuple<int, std::string> SulDownloaderRunner::startUpdateService()
        {
            try
            {
                auto request = watchdog::watchdogimpl::factory().create();
                request->requestUpdateService();
            }
            catch (std::exception& ex)
            {
                return std::make_tuple(1, ex.what());
            }
            return std::make_tuple(0, std::string());
        }

        void SulDownloaderRunner::abortWaitingForReport() { m_listener.abort(); }

    } // namespace runnerModule

} // namespace UpdateSchedulerImpl