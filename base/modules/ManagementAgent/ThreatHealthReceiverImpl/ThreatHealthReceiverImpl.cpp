/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ThreatHealthReceiverImpl.h"

#include "ThreatHealthTask.h"

#include "../../../thirdparty/nlohmann-json/json.hpp"
#include <ManagementAgent/LoggerImpl/Logger.h>

using ManagementAgent::ThreatHealthReceiverImpl::ThreatHealthReceiverImpl;

ManagementAgent::ThreatHealthReceiverImpl::ThreatHealthReceiverImpl::ThreatHealthReceiverImpl(
    Common::TaskQueue::ITaskQueueSharedPtr taskQueue) :
    m_taskQueue(std::move(taskQueue))
{
}

void ThreatHealthReceiverImpl::receivedThreatHealth(const std::string& pluginName, const std::string& threatHealth, std::shared_ptr<ManagementAgent::HealthStatusImpl::HealthStatus> healthStatusSharedObj)
{
    LOGDEBUG("ThreatHealthReceiverImpl::receivedThreatHealth" << pluginName << ":" << threatHealth);

    //  TODO try catch this.

    if (healthStatusSharedObj != nullptr)
    {
        nlohmann::json j = nlohmann::json::parse(threatHealth);
        unsigned int threatHealthValue = j["threatHealth"];

        Common::TaskQueue::ITaskPtr task(new ThreatHealthTask(pluginName, threatHealthValue, healthStatusSharedObj));
        m_taskQueue->queueTask(std::move(task));
    }
    else
    {
        LOGERROR("HealthStatus shared obj has not been initialised");
    }
}
