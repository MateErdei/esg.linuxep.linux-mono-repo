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
        m_taskQueue(std::move(taskQueue))
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
        m_taskQueue->pushPriority(Task { Task::Shutdown });
    }

    Common::PluginApi::StatusInfo PluginCallback::getStatus(const std::string& /*appId*/)
    {
        LOGSUPPORT("Received unexpected get status request");
        return Common::PluginApi::StatusInfo {};
    }

    std::string PluginCallback::getTelemetry()
    {
        LOGSUPPORT("Received telemetry request");
        return std::string(); // TODO: LINUXEP-7988
    }
} // namespace TelemetrySchedulerImpl