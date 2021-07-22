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
        if (size > dataLimit)
        {
            disk.deleteOldJournalFiles(eventJournalPath, lowerLimit);
        }
    }
} // namespace Plugin