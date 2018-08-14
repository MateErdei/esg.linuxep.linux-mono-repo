/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SulDownloaderRunner.h"

namespace UpdateScheduler
{
    SulDownloaderRunner::SulDownloaderRunner(std::shared_ptr<SchedulerTaskQueue> schedulerTaskQueue, std::string directoryToWatch)
            : m_listener(directoryToWatch), m_schedulerTaskQueue(schedulerTaskQueue)
    {
        m_directoryWatcher = std::unique_ptr<Common::DirectoryWatcher::IDirectoryWatcher>(new Common::DirectoryWatcherImpl::DirectoryWatcher());
        m_directoryWatcher->addListener(m_listener);
    }

    void SulDownloaderRunner::run()
    {
        m_directoryWatcher->startWatch();
        SchedulerTask schedulerTask;

        // Run SUL Downloader service, report error and return if it fails to start.
        if (startService() != 0)
        {
            schedulerTask.taskType = SchedulerTask::TaskType::SulDownloaderFailedToStart;
            m_schedulerTaskQueue->push(schedulerTask);
            return;
        }

        // Wait for the SUL Downloader results file to be created.
        std::string fileLocation = m_listener.waitForFile(60);

        // If the file failed to be created after waiting for the timeout period then report a timeout error and return.
        if (fileLocation.empty())
        {
            schedulerTask.taskType = SchedulerTask::TaskType::SulDownloaderTimedOut;
            m_schedulerTaskQueue->push(schedulerTask);
            return;
        }

        // If update result was successfully generated within timeout period then report file location and return.
        schedulerTask.taskType = SchedulerTask::TaskType::SulDownloaderFinished;
        schedulerTask.content = fileLocation;

        m_schedulerTaskQueue->push(schedulerTask);
    }

    int SulDownloaderRunner::startService()
    {
        return system("systemctl start sophos-spl-update.service");
    }
}