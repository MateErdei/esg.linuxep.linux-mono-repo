/******************************************************************************************************
Copyright 2021 Sophos Limited.  All rights reserved.
******************************************************************************************************/

#include "PluginAdapter.h"

#include "ApplicationPaths.h"
#include "DiskManager.h"
#include "Logger.h"

#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/UtilityImpl/TimeUtils.h>
#include <SubscriberLib/Subscriber.h>

namespace Plugin
{
    PluginAdapter::PluginAdapter(
        std::shared_ptr<QueueTask> queueTask,
        std::unique_ptr<Common::PluginApi::IBaseServiceApi> baseService,
        std::shared_ptr<PluginCallback> callback,
        std::unique_ptr<SubscriberLib::ISubscriber> subscriber
        ) :
        m_queueTask(std::move(queueTask)),
        m_baseService(std::move(baseService)),
        m_callback(std::move(callback)),
        m_subscriber(std::move(subscriber))
    {
    }

    void PluginAdapter::mainLoop()
    {
        manageJournalDataStoreSize();
        Common::UtilityImpl::FormattedTime time;
        uint64_t lastChecked = std::stoul(time.currentEpochTimeInSeconds());
        m_callback->setRunning(true);
        LOGINFO("Entering the main loop");
        m_subscriber->start();
        while (true)
        {
            Task task;
            if (!m_queueTask->pop(task, QUEUE_TIMEOUT))
            {
                if (!m_subscriber->getRunningStatus())
                {
                    LOGERROR("Subscriber not running, restarting it.");
                    m_subscriber->restart();
                }
                uint64_t current = std::stoul(time.currentEpochTimeInSeconds());
                if ((lastChecked+3600) < current)
                {
                    // an hour has passed check the journalFiles
                    manageJournalDataStoreSize();
                }
            }
            else
            {
                switch (task.taskType)
                {
                    case Task::TaskType::Stop:
                        m_subscriber->stop();
                        return;
                    case Task::TaskType::Policy:
                        processPolicy(task.Content);
                        break;
                }
            }
        }
    }

    void PluginAdapter::processPolicy(const std::string& policyXml) { LOGDEBUG("Process policy: " << policyXml); }
    void PluginAdapter::manageJournalDataStoreSize()
    {
        Plugin::DiskManager disk;
        std::string eventJournalPath = Common::ApplicationConfiguration::applicationPathManager().getEventJournalsPath();
        disk.compressClosedFiles(eventJournalPath);
        uint64_t size = disk.getDirectorySize(eventJournalPath);
        std::vector<std::string> subjects;
        try
        {
            subjects = Common::FileSystem::fileSystem()->listDirectories(eventJournalPath);
        }
        catch(Common::FileSystem::IFileSystemException&)
        {
            LOGDEBUG("No event journal subjects found");
            return;
        }
        size_t maxRetryCount = subjects.size();
        size_t counter = 0;

        while (size > dataLimit && counter < maxRetryCount)
        {
            uint64_t totalDeleted = disk.deleteOldJournalFiles(eventJournalPath, lowerLimit, size);
            if (totalDeleted == 0)
            {
                break;
            }
            size = disk.getDirectorySize(eventJournalPath);
            counter++;
        }
    }
} // namespace Plugin