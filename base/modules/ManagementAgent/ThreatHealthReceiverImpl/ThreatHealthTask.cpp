/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ThreatHealthTask.h"

ManagementAgent::ThreatHealthReceiverImpl::ThreatHealthTask::ThreatHealthTask(
    std::string pluginName,
    unsigned int threatHealth,
    std::shared_ptr<HealthStatusImpl::HealthStatus> healthStatus) :
    m_pluginName(std::move(pluginName)),
    m_threatHealth(threatHealth),
    m_healthStatus(std::move(healthStatus))
{

}

void ManagementAgent::ThreatHealthReceiverImpl::ThreatHealthTask::run()
{
    PluginCommunication::PluginHealthStatus threatHealthStatus;
    threatHealthStatus.healthType = PluginCommunication::HealthType::THREAT_DETECTION;
    threatHealthStatus.healthValue = m_threatHealth;
    m_healthStatus->addPluginHealth(m_pluginName, threatHealthStatus);
}
