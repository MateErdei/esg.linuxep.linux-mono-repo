/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ThreatHealthReceiverImpl.h"

#include "ThreatHealthTask.h"

using ManagementAgent::ThreatHealthReceiverImpl::ThreatHealthReceiverImpl;

ManagementAgent::ThreatHealthReceiverImpl::ThreatHealthReceiverImpl::ThreatHealthReceiverImpl(
    Common::TaskQueue::ITaskQueueSharedPtr taskQueue) :
    m_taskQueue(std::move(taskQueue))
{
}

void ThreatHealthReceiverImpl::receivedThreatHealth(const std::string& pluginName, const std::string& threatHealth, std::shared_ptr<ManagementAgent::HealthStatusImpl::HealthStatus> healthStatusSharedObj)
{
    //  TODO maybe ? extract plugin name and hleaht in from message string if needed
    (void) threatHealth;
    Common::TaskQueue::ITaskPtr task(new ThreatHealthTask(pluginName, 2, healthStatusSharedObj));
    m_taskQueue->queueTask(std::move(task));
}
