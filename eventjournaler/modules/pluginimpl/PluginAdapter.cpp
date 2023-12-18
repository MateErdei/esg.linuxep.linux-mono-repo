// Copyright 2021-2023 Sophos Limited. All rights reserved.

#include "PluginAdapter.h"

#include "ApplicationPaths.h"
#include "DiskManager.h"
#include "Logger.h"

#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "Common/UtilityImpl/TimeUtils.h"

#include <utility>

namespace Plugin
{
    PluginAdapter::PluginAdapter(
            std::shared_ptr<TaskQueue> queueTask,
            std::unique_ptr<Common::PluginApi::IBaseServiceApi> baseService,
            std::shared_ptr<PluginCallback> callback,
            std::unique_ptr<SubscriberLib::ISubscriber> subscriber,
            std::shared_ptr<EventWriterLib::IEventWriterWorker> eventWriter,
            std::shared_ptr<Heartbeat::HeartbeatPinger> heartbeatPinger) :
        m_taskQueue(std::move(queueTask)),
        m_baseService(std::move(baseService)),
        m_callback(std::move(callback)),
        m_subscriber(std::move(subscriber)),
        m_eventWriterWorker(std::move(eventWriter)),
        m_heartbeatPinger(std::move(heartbeatPinger))
    {
    }

    void PluginAdapter::mainLoop()
    {
        manageJournalDataStoreSize();
        Common::UtilityImpl::FormattedTime time;
        uint64_t lastChecked = std::stoul(time.currentEpochTimeInSeconds());
        m_callback->setRunning(true);
        LOGINFO("Completed initialization of Event Journaler");
        m_eventWriterWorker->start();
        m_subscriber->start();
        while (true)
        {
            m_heartbeatPinger->ping();
            Task task;
            if (!m_taskQueue->pop(task, QUEUE_TIMEOUT))
            {
                if (!m_subscriber->getRunningStatus())
                {
                    LOGERROR("Subscriber not running, restarting it.");
                    m_subscriber->restart();
                }
                if (!m_eventWriterWorker->getRunningStatus())
                {
                    LOGERROR("Event Writer not running, restarting it.");
                    m_eventWriterWorker->restart();
                }
                uint64_t current = std::stoul(time.currentEpochTimeInSeconds());
                if ((lastChecked+3600) < current)
                {
                    // an hour has passed check the journalFiles
                    manageJournalDataStoreSize();
                    lastChecked = current;
                }
            }
            else
            {
                switch (task.taskType)
                {
                    case Task::TaskType::Stop:
                        m_eventWriterWorker->beginStop();
                        m_subscriber->stop();
                        m_eventWriterWorker->stop();
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
        disk.compressClosedFiles(eventJournalPath, m_eventWriterWorker);
        uint64_t size = disk.getDirectorySize(eventJournalPath);


        disk.deleteOldJournalFiles(eventJournalPath, lowerLimit, size);
    }
} // namespace Plugin