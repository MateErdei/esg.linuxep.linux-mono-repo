/******************************************************************************************************

Copyright 2018 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "PluginCallback.h"
#include "Logger.h"
#include "Telemetry.h"

namespace UpdateScheduler
{

    PluginCallback::PluginCallback(std::shared_ptr<QueueTask> task) :
        m_task(task), m_statusInfo()
    {
        LOGDEBUG("Plugin Callback Started");
    }

    void PluginCallback::applyNewPolicy(const std::string &policyXml)
    {
        LOGSUPPORT("Applying new policy");
    }

    void PluginCallback::queueAction(const std::string &actionXml)
    {
        LOGSUPPORT("Queueing action");

    }

    void PluginCallback::onShutdown()
    {
        LOGSUPPORT("Shutdown signal received");

    }

    Common::PluginApi::StatusInfo PluginCallback::getStatus(const std::string &appId)
    {
        LOGSUPPORT("Received get status request");
        return m_statusInfo;
    }

    void PluginCallback::setStatus(Common::PluginApi::StatusInfo statusInfo)
    {
        LOGSUPPORT("Setting status");

    }

    std::string PluginCallback::getTelemetry()
    {
        LOGSUPPORT("Received get telemetry request");
        return std::string();
    }
}