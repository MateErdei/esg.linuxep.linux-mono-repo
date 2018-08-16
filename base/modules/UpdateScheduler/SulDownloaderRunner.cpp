/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/Process/IProcess.h>
#include "SulDownloaderRunner.h"

namespace UpdateScheduler
{

    SulDownloaderRunner::SulDownloaderRunner(std::shared_ptr<SchedulerTaskQueue> schedulerTaskQueue, std::string directoryToWatch, std::chrono::seconds timeout)
        :
            m_listener(directoryToWatch),
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
        if (startUpdateService() != 0)
        {
            schedulerTask.taskType = SchedulerTask::TaskType::SulDownloaderFailedToStart;
            m_schedulerTaskQueue->push(schedulerTask);
            return;
        }

        // Wait for the SUL Downloader results file to be created or a timeout or abort is called.
        std::string reportFileLocation = m_listener.waitForFile(m_timeout);

        // If the file failed to be created it means either an abort was called or the wait timed out and no file was found.
        if (reportFileLocation.empty())
        {
            if (m_listener.wasAborted())
            {
                schedulerTask.taskType = SchedulerTask::TaskType::SulDownloaderWasAborted;
            }
            else
            {
                schedulerTask.taskType = SchedulerTask::TaskType::SulDownloaderTimedOut;
            }

            m_schedulerTaskQueue->push(schedulerTask);
            return;
        }

        // If update result was successfully generated within timeout period then report file location and return.
        schedulerTask.taskType = SchedulerTask::TaskType::SulDownloaderFinished;
        schedulerTask.content = reportFileLocation;

        m_schedulerTaskQueue->push(schedulerTask);
    }

    int SulDownloaderRunner::startUpdateService()
    {
        auto process = Common::Process::createProcess();
        process->exec( "systemctl", {"start", "sophos-spl-update.service"} );
        std::string output = process->output();
        return process->exitCode();
    }

    void SulDownloaderRunner::abort()
    {
        m_listener.abort();
    }

}