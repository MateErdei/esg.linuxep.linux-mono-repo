/******************************************************************************************************

Copyright 2018 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SchedulerPluginCallback.h"
#include "Logger.h"

namespace UpdateScheduler
{

    SchedulerPluginCallback::SchedulerPluginCallback(std::shared_ptr<SchedulerTaskQueue> task) :
        m_task(task), m_statusInfo()
    {
        LOGDEBUG("Plugin Callback Started");
    }

    void SchedulerPluginCallback::applyNewPolicy(const std::string &policyXml)
    {
        LOGSUPPORT("Applying new policy");
    }

    void SchedulerPluginCallback::queueAction(const std::string &actionXml)
    {
        LOGSUPPORT("Queueing action");

    }

    void SchedulerPluginCallback::onShutdown()
    {
        LOGSUPPORT("Shutdown signal received");

    }

    Common::PluginApi::StatusInfo SchedulerPluginCallback::getStatus(const std::string &appId)
    {
        LOGSUPPORT("Received get status request");
        return m_statusInfo;
    }

    void SchedulerPluginCallback::setStatus(Common::PluginApi::StatusInfo statusInfo)
    {
        LOGSUPPORT("Setting status");

    }

    std::string SchedulerPluginCallback::getTelemetry()
    {
        LOGSUPPORT("Received get telemetry request");
        return std::string();
    }
}