/******************************************************************************************************

Copyright 2018 Sophos Limited.  All rights reserved.

******************************************************************************************************/
//#include <Subscriber/Subscriber.h>
#include "PluginAdapter.h"

#include "Logger.h"
#include "Telemetry.h"
#include "ApplicationPaths.h"
#include <Common/PluginApi/NoPolicyAvailableException.h>
//#include <modules/SubscriberLib/Subscriber.h>
#include <SubscriberLib/Subscriber.h>

#include <unistd.h>

namespace Plugin
{
    PluginAdapter::PluginAdapter(
        std::shared_ptr<QueueTask> queueTask,
        std::unique_ptr<Common::PluginApi::IBaseServiceApi> baseService,
        std::shared_ptr<PluginCallback> callback) :
        m_queueTask(std::move(queueTask)),
        m_baseService(std::move(baseService)),
        m_callback(std::move(callback))
    {
    }

    void PluginAdapter::mainLoop()
    {
        m_callback->setRunning(true);
        LOGINFO("Entering the main loop");
        SubscriberLib::Subscriber subscriber(Plugin::getSubscriberSocketPath());
        subscriber.start();

        while (true)
        {
            Task task;
            if (!m_queueTask->pop(task, QUEUE_TIMEOUT))
            {
                if (!subscriber.getRunningStatus())
                {
                    subscriber.reset();
                }
            }
            else
            {
                switch (task.taskType)
                {
                    case Task::TaskType::Stop:
                        subscriber.stop();
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