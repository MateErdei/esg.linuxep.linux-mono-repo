/******************************************************************************************************

Copyright 2019 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "PluginCallback.h"

#include <Common/PluginApi/ApiException.h>
#include <TelemetryScheduler/LoggerImpl/Logger.h>

#include <utility>

namespace TelemetrySchedulerImpl
{
    PluginCallback::PluginCallback(std::shared_ptr<TaskQueue> taskQueue) :
        m_shutdownReceived(false),
        m_taskQueue(std::move(taskQueue)),
        m_statusInfo()
    {
        LOGDEBUG("Plugin callback started");
    }

    void PluginCallback::applyNewPolicy(const std::string& /*policyXml*/)
    {
        LOGSUPPORT("Not applying unexpected new policy");
    }

    void PluginCallback::queueAction(const std::string& /*actionXml*/)
    {
        LOGSUPPORT("Received unexpected action");
    }

    void PluginCallback::onShutdown()
    {
        LOGSUPPORT("Shutdown signal received");
        m_shutdownReceived = true;
        m_taskQueue->pushPriority(Task { Task::Shutdown });
    }

    Common::PluginApi::StatusInfo PluginCallback::getStatus(const std::string& /*appId*/)
    {
        LOGSUPPORT("Received unexpected get status request");
        return m_statusInfo;
    }

    std::string PluginCallback::getTelemetry()
    {
        LOGSUPPORT("Received telemetry request");
        return std::string(); // TODO: LINUXEP-7988
    }

    bool PluginCallback::shutdownReceived() { return m_shutdownReceived; }
} // namespace TelemetrySchedulerImpl