/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SulDownloaderRunner.h"
#include "Logger.h"

namespace UpdateSchedulerImpl
{

    SulDownloaderRunner::SulDownloaderRunner(
            std::shared_ptr<SchedulerTaskQueue> schedulerTaskQueue,
            const std::string& directoryToWatch,
            const std::string& nameOfFileToWaitFor,
            std::chrono::seconds timeout)
        :
            m_listener(directoryToWatch, nameOfFileToWaitFor),
            m_directoryWatcher(std::unique_ptr<Common::DirectoryWatcher::IDirectoryWatcher>(new Common::DirectoryWatcherImpl::DirectoryWatcher())),
            m_schedulerTaskQueue(schedulerTaskQueue),
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

        // If the file failed to be created it means either an abortWaitingForReport was called or the wait timed out and no file was found.
        if (reportFileLocation.empty())
        {
            if (m_listener.wasAborted())
            {
                schedulerTask.taskType = SchedulerTask::TaskType::SulDownloaderWasAborted;
                LOGINFO("Update Service was aborted.");
            }
            else
            {
                schedulerTask.taskType = SchedulerTask::TaskType::SulDownloaderTimedOut;
                LOGWARN("Update Service timed out.");
            }

            m_schedulerTaskQueue->push(schedulerTask);
            return;
        }

        // If update result was successfully generated within timeout period then report file location and return.
        schedulerTask.taskType = SchedulerTask::TaskType::SulDownloaderFinished;
        schedulerTask.content = reportFileLocation;
        m_schedulerTaskQueue->push(schedulerTask);
        LOGINFO("Update Service finished.");
    }

    std::tuple<int, std::string> SulDownloaderRunner::startUpdateService()
    {
        auto process = Common::Process::createProcess();
        process->exec( "/bin/systemctl", {"start", "sophos-spl-update.service"} );
        return std::make_tuple(process->exitCode(), process->output());
    }

    void SulDownloaderRunner::abortWaitingForReport()
    {
        m_listener.abort();
    }


}