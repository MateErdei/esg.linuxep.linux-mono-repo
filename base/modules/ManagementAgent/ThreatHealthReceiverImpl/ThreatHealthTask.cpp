/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ThreatHealthTask.h"

#include <ManagementAgent/LoggerImpl/Logger.h>

ManagementAgent::ThreatHealthReceiverImpl::ThreatHealthTask::ThreatHealthTask(
    std::string pluginName,
    unsigned int threatHealth,
    std::shared_ptr<HealthStatusImpl::HealthStatus> healthStatus) :
    m_pluginName(std::move(pluginName)), m_threatHealth(threatHealth), m_healthStatus(std::move(healthStatus))
{
}

void ManagementAgent::ThreatHealthReceiverImpl::ThreatHealthTask::run()
{
    LOGDEBUG("Running threat health task");
    if (m_healthStatus != nullptr)
    {
        PluginCommunication::PluginHealthStatus threatHealthStatus;
        threatHealthStatus.healthType = PluginCommunication::HealthType::THREAT_DETECTION;
        threatHealthStatus.healthValue = m_threatHealth;
        m_healthStatus->addPluginHealth(m_pluginName, threatHealthStatus);
    }
    else
    {
        LOGERROR("Failed to receive Threat Health because Health Status shared object has not been initialised");
    }
}
