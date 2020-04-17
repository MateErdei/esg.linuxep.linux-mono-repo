/******************************************************************************************************

Copyright 2019 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "PluginCallback.h"

#include <Common/PluginApi/ApiException.h>
#include <TelemetryScheduler/LoggerImpl/Logger.h>

#include <utility>
#include <Common/TelemetryHelperImpl/TelemetryHelper.h>

namespace TelemetrySchedulerImpl
{
    const char* g_pluginName = "tscheduler";
    PluginCallback::PluginCallback(std::shared_ptr<ITaskQueue> taskQueue) : m_taskQueue(std::move(taskQueue))
    {
        LOGDEBUG("Plugin callback started");
    }

    void PluginCallback::applyNewPolicy(const std::string& /*policyXml*/)
    {
        LOGSUPPORT("Not applying unexpected new policy");
    }

    void PluginCallback::queueAction(const std::string& /*actionXml*/) { LOGSUPPORT("Received unexpected action"); }

    void PluginCallback::onShutdown()
    {
        LOGSUPPORT("Shutdown signal received");
        m_taskQueue->pushPriority(SchedulerTask::Shutdown);
    }

    Common::PluginApi::StatusInfo PluginCallback::getStatus(const std::string& /*appId*/)
    {
        LOGSUPPORT("Received unexpected get status request");
        return Common::PluginApi::StatusInfo{};
    }

    std::string PluginCallback::getTelemetry()
    {
        LOGSUPPORT("Received telemetry request");
        return "{}"; // TODO: LINUXEP-7988
    }

    void PluginCallback::saveTelemetry()
    {
        LOGSUPPORT("Received save telemetry request");
        try
        {
            Common::Telemetry::TelemetryHelper::getInstance().save(g_pluginName);
        }
        catch(std::exception& ex)
        {
            LOGDEBUG("Saving telemetry was unsuccessful reason: "<< ex.what());
        }
    }

    void initialiseTelemetry()
    {
        try
        {
            LOGSUPPORT("Restoring telemetry from disk");
            Common::Telemetry::TelemetryHelper::getInstance().restore(g_pluginName);
        }
        catch(std::exception& ex)
        {
            LOGDEBUG("Restore telemetry was unsuccessful reason: "<< ex.what());
        }
    }
} // namespace TelemetrySchedulerImpl