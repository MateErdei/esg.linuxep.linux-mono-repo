/******************************************************************************************************

Copyright 2018-2019 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "PluginAdapter.h"

#include "Logger.h"
#include "Telemetry.h"
#include <livequery/ResponseDispatcher.h>

namespace Plugin
{
    PluginAdapter::PluginAdapter(
        std::shared_ptr<QueueTask> queueTask,
        std::unique_ptr<Common::PluginApi::IBaseServiceApi> baseService,
        std::shared_ptr<PluginCallback> callback,
        std::unique_ptr<livequery::IQueryProcessor> queryProcessor,
        std::unique_ptr<livequery::IResponseDispatcher> responseDispatcher) :
        m_queueTask(std::move(queueTask)),
        m_baseService(std::move(baseService)),
        m_callback(std::move(callback)),
        m_queryProcessor{std::move(queryProcessor)},
        m_responseDispatcher{std::move(responseDispatcher)}
    {
    }

    void PluginAdapter::mainLoop()
    {
        LOGINFO("Entering the main loop");

        while (true)
        {
            Task task = m_queueTask->pop();
            switch (task.m_taskType)
            {
                case Task::TaskType::Stop:
                    return;

                case Task::TaskType::Policy:
                    processPolicy(task.m_content);
                    break;
                case Task::TaskType::Query:
                    processQuery(task.m_content, task.m_correlationId);
            }
        }
    }

    void PluginAdapter::processPolicy(const std::string& policyXml) { LOGDEBUG("Process policy: " << policyXml); }

    void PluginAdapter::processQuery(const std::string &queryJson, const std::string &correlationId)
    {
        livequery::processQuery(*m_queryProcessor, *m_responseDispatcher, queryJson, correlationId);
    }


} // namespace Plugin