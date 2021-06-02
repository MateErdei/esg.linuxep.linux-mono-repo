/******************************************************************************************************

Copyright 2019 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "PluginCallback.h"

#include <Common/PluginApi/ApiException.h>
#include <TelemetryScheduler/LoggerImpl/Logger.h>

#include <utility>

namespace RemoteDiagnoseImpl
{
    PluginCallback::PluginCallback(std::shared_ptr<ITaskQueue> taskQueue) : m_taskQueue(std::move(taskQueue))
    {
        LOGDEBUG("Plugin callback started");
    }

    void PluginCallback::applyNewPolicy(const std::string& policyXml)
    {
        LOGSUPPORT("Not applying unexpected new policy: " << policyXml);
    }

    void PluginCallback::queueAction(const std::string& actionXml) {
        m_taskQueue->push(Task{ Task::TaskType::ACTION, actionXml });}

    void PluginCallback::onShutdown()
    {
        LOGSUPPORT("Shutdown signal received");

        m_taskQueue->pushStop();
    }

    Common::PluginApi::StatusInfo PluginCallback::getStatus(const std
    ::string& /* appId */)
    {
        LOGSUPPORT("Received get status request");
        return m_statusInfo;
    }

    void PluginCallback::setStatus(Common::PluginApi::StatusInfo statusInfo)
    {
        LOGSUPPORT("Setting status");
        m_statusInfo = std::move(statusInfo);
    }
    std::string PluginCallback::getTelemetry()
    {
        LOGSUPPORT("Received telemetry request");
        return "{}";
    }
} // namespace RemoteDiagnoseImpl