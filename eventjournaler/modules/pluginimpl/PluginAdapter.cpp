/******************************************************************************************************
Copyright 2021 Sophos Limited.  All rights reserved.
******************************************************************************************************/

#include "PluginAdapter.h"
#include "Logger.h"
#include "ApplicationPaths.h"

#include <SubscriberLib/Subscriber.h>

namespace Plugin
{
    PluginAdapter::PluginAdapter(
        std::shared_ptr<QueueTask> queueTask,
        std::unique_ptr<Common::PluginApi::IBaseServiceApi> baseService,
        std::shared_ptr<PluginCallback> callback,
        std::unique_ptr<SubscriberLib::ISubscriber> subscriber,
        std::unique_ptr<EventWriterLib::IEventWriterWorker> eventWriter) :
        m_queueTask(std::move(queueTask)),
        m_baseService(std::move(baseService)),
        m_callback(std::move(callback)),
        m_subscriber(std::move(subscriber)),
        m_eventWriterWorker(std::move(eventWriter))
    {
    }

    void PluginAdapter::mainLoop()
    {
        m_callback->setRunning(true);
        LOGINFO("Entering the main loop");
        m_eventWriterWorker->start();
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
                if (!m_eventWriterWorker->getRunningStatus())
                {
                    LOGERROR("Event Writer not running, restarting it.");
                    m_eventWriterWorker->restart();
                }
            }
            else
            {
                switch (task.taskType)
                {
                    case Task::TaskType::Stop:
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
} // namespace Plugin