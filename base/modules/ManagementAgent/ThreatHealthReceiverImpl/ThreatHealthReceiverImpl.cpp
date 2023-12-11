// Copyright 2021-2023 Sophos Limited. All rights reserved.

#include "ThreatHealthReceiverImpl.h"

#include "ThreatHealthTask.h"

#include "ManagementAgent/LoggerImpl/Logger.h"

#include <nlohmann/json.hpp>

using ManagementAgent::ThreatHealthReceiverImpl::ThreatHealthReceiverImpl;

ManagementAgent::ThreatHealthReceiverImpl::ThreatHealthReceiverImpl::ThreatHealthReceiverImpl(
    Common::TaskQueue::ITaskQueueSharedPtr taskQueue) :
    m_taskQueue(std::move(taskQueue))
{
}

bool ThreatHealthReceiverImpl::receivedThreatHealth(
    const std::string& pluginName,
    const std::string& threatHealth,
    std::shared_ptr<ManagementAgent::HealthStatusImpl::HealthStatus> healthStatusSharedObj)
{
    LOGDEBUG("ThreatHealthReceiverImpl::receivedThreatHealth" << pluginName << ":" << threatHealth);

    if (healthStatusSharedObj != nullptr)
    {
        try
        {
            nlohmann::json j = nlohmann::json::parse(threatHealth);
            unsigned int threatHealthValue = j["ThreatHealth"];

            Common::TaskQueue::ITaskPtr task(
                new ThreatHealthTask(pluginName, threatHealthValue, healthStatusSharedObj));
            m_taskQueue->queueTask(std::move(task));
            return true;
        }
        catch (nlohmann::json::parse_error& exception)
        {
            LOGERROR(
                "Failed to parse JSON sent from plugin: " << pluginName << ", error: " << exception.what()
                                                          << ", exception id: " << exception.id
                                                          << ", content: " << threatHealth);
        }
    }
    else
    {
        LOGERROR("HealthStatus shared obj has not been initialised");
    }
    return false;
}
