#include <utility>

/******************************************************************************************************

Copyright 2018 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SchedulerPluginCallback.h"

#include <Common/PluginApi/ApiException.h>
#include <TelemetryScheduler/LoggerImpl/Logger.h>

namespace TelemetrySchedulerImpl
{
    // TODO: add task queue use - see UpdateScheduler for example

    SchedulerPluginCallback::SchedulerPluginCallback() :
        m_statusInfo(),
        m_shutdownReceived(false)
    {
        LOGDEBUG("Plugin callback started");
    }

    void SchedulerPluginCallback::applyNewPolicy(const std::string& /*policyXml*/)
    {
        LOGSUPPORT("Not applying new policy");
    }

    void SchedulerPluginCallback::queueAction(const std::string& /*actionXml*/) { LOGSUPPORT("Received action"); }

    void SchedulerPluginCallback::onShutdown()
    {
        LOGSUPPORT("Shutdown signal received");
        m_shutdownReceived = true;
    }

    Common::PluginApi::StatusInfo SchedulerPluginCallback::getStatus(const std::string& /*appId*/)
    {
        LOGSUPPORT("Received get status request");

        if (m_statusInfo.statusXml.empty())
        {
            LOGWARN("Status has not been configured yet.");
            throw Common::PluginApi::ApiException("Status not yet set");
        }

        return m_statusInfo;
    }

    void SchedulerPluginCallback::setStatus(Common::PluginApi::StatusInfo statusInfo)
    {
        LOGSUPPORT("Setting status");
        m_statusInfo = std::move(statusInfo);
    }

    std::string SchedulerPluginCallback::getTelemetry()
    {
        LOGSUPPORT("Received telemetry request");
        return std::string(); // TODO: LINUXEP-7988
    }

    bool SchedulerPluginCallback::shutdownReceived() { return m_shutdownReceived; }
} // namespace TelemetrySchedulerImpl