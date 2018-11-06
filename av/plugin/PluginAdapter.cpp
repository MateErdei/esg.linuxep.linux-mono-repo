/******************************************************************************************************

Copyright 2018 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "PluginAdapter.h"
#include "EventBuilder.h"
#include "Telemetry.h"
#include "Logger.h"

namespace Example
{
    PluginAdapter::PluginAdapter(std::shared_ptr<QueueTask> queueTask, std::unique_ptr<Common::PluginApi::IBaseServiceApi> baseService, std::shared_ptr<PluginCallback> callback)
    : m_queueTask(queueTask), m_baseService(std::move(baseService)), m_callback(callback), m_fileScanner(), m_scannerSettings()
    {

    }

    void PluginAdapter::mainLoop()
    {
        LOGINFO("Request SAV policy");
        try
        {
            m_baseService->requestPolicies("SAV");
        }
        catch(std::exception & ex)
        {
            LOGERROR("Request of Policies for SAV failed. Error: " << ex.what());
        }


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

                case Task::TaskType::ScanNow:
                    runScanner();
                    break;
            }
        }
    }

    void PluginAdapter::runScanner()
    {
        if (!m_scannerSettings.isEnabled())
        {
            LOGINFO("Ignoring ScanNow Action as scanner is disabled");
            return;
        }
        LOGINFO("Starting Scan");
        m_fileScanner.setExclusions(m_scannerSettings.getExclusions());
        ScanReport scanReport = m_fileScanner.scan();
        Telemetry::instance().updateWithReport(scanReport);
        std::vector<std::string> events = buildEvents(scanReport);
        LOGINFO("Sending " << events.size() << " events");
        for (auto & event : events)
        {
            m_baseService->sendEvent("SAV", event);
        }
    }

    void PluginAdapter::processPolicy(const std::string & policyXml)
    {
        LOGDEBUG("Process policy: " << policyXml);
        m_scannerSettings.ProcessPolicy(policyXml);
        auto statusInfo = m_scannerSettings.getStatus();
        m_callback->setStatus(statusInfo);
        m_baseService->sendStatus(statusInfo.appId, statusInfo.statusXml, statusInfo.statusWithoutTimestampsXml);
    }
}