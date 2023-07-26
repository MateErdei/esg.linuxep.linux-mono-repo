// Copyright 2019-2023 Sophos Limited. All rights reserved.

#include "PluginCallback.h"

#include <Common/PluginApi/ApiException.h>
#include <Common/TelemetryHelperImpl/TelemetryHelper.h>
#include <TelemetryScheduler/LoggerImpl/Logger.h>

#include <utility>

namespace TelemetrySchedulerImpl
{
    PluginCallback::PluginCallback(std::shared_ptr<ITaskQueue> taskQueue) : m_taskQueue(std::move(taskQueue))
    {
        LOGDEBUG("Plugin callback started");
    }

    void PluginCallback::applyNewPolicy(const std::string& /*policyXml*/)
    {
        LOGERROR("Attempted to apply new policy without AppId: This method should never be called.");
    }

    void PluginCallback::applyNewPolicyWithAppId(const std::string& appId, const std::string& policyXml)
    {
        LOGDEBUG("Applying new policy");
        m_taskQueue->push(SchedulerTask{ SchedulerTask::TaskType::Policy, policyXml, appId });
    }

    void PluginCallback::queueAction(const std::string& /*actionXml*/)
    {
        LOGSUPPORT("Received unexpected action");
    }

    void PluginCallback::onShutdown()
    {
        LOGSUPPORT("Shutdown signal received");
        m_taskQueue->pushPriority({ SchedulerTask::TaskType::Shutdown });
    }

    Common::PluginApi::StatusInfo PluginCallback::getStatus(const std::string& /*appId*/)
    {
        LOGSUPPORT("Received unexpected get status request");
        return Common::PluginApi::StatusInfo{};
    }

    std::string PluginCallback::getTelemetry()
    {
        LOGSUPPORT("Received telemetry request");
        Common::Telemetry::TelemetryHelper::getInstance().set("health", 0UL);
        return Common::Telemetry::TelemetryHelper::getInstance().serialiseAndReset();
    }

    std::string PluginCallback::getHealth()
    {
        LOGDEBUG("Received health request");
        return "{}";
    }
} // namespace TelemetrySchedulerImpl