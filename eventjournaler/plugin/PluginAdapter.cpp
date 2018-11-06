/******************************************************************************************************

Copyright 2018 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "PluginAdapter.h"
#include "Telemetry.h"
#include "Logger.h"

namespace Example
{
    PluginAdapter::PluginAdapter(std::shared_ptr<QueueTask> queueTask, std::unique_ptr<Common::PluginApi::IBaseServiceApi> baseService, std::shared_ptr<PluginCallback> callback)
    : m_queueTask(queueTask), m_baseService(std::move(baseService)), m_callback(callback)
    {

    }

    void PluginAdapter::mainLoop()
    {

        LOGINFO("Entering the main loop");

        while(true)
        {
            Task task = m_queueTask->pop();
            switch(task.taskType)
            {
                case Task::TaskType::Stop:
                    return;

                case Task::TaskType::Policy:
                    processPolicy(task.Content);
                    break;

            }
        }
    }

    void PluginAdapter::processPolicy(const std::string & policyXml)
    {
        LOGDEBUG("Process policy: " << policyXml);
    }
}