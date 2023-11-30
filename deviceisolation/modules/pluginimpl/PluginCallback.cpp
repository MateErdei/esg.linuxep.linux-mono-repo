// Copyright 2023 Sophos Limited. All rights reserved.

#include "PluginCallback.h"

#include "ApplicationPaths.h"
#include "Logger.h"
#include "Telemetry.h"

#include "Common/TelemetryHelperImpl/TelemetryHelper.h"

#include <utility>

#include <unistd.h>

namespace Plugin
{
    PluginCallback::PluginCallback(std::shared_ptr<TaskQueue> task)
    : m_task(std::move(task))
    {
        LOGDEBUG("Plugin Callback Started");
    }

    void PluginCallback::applyNewPolicyWithAppId(const std::string& appId, const std::string& policyXml)
    {
        LOGDEBUG("Applying new policy with APPID: " << appId);
        m_task->push(Task { Task::TaskType::Policy, policyXml, "", appId });
    }

    void PluginCallback::applyNewPolicy(const std::string& /*policyXml */)
    {
        LOGERROR("Received policy without appId!");
    }

    void PluginCallback::queueActionWithCorrelation(const std::string& actionXml, const std::string& correlationId)
    {
        m_task->push(Task { Task::TaskType::Action, actionXml, correlationId });
    }

    void PluginCallback::queueAction(const std::string& actionXml)
    {
        LOGWARN("Internal error: queueAction without correlationId");
        queueActionWithCorrelation(actionXml, "");
    }

    void PluginCallback::onShutdown()
    {
        LOGDEBUG("Shutdown signal received");
        m_task->pushStop();
        int timeoutCounter = 0;
        int shutdownTimeout = 5;
        while(isRunning() && timeoutCounter < shutdownTimeout)
        {
            LOGDEBUG("Shutdown waiting for all processes to complete");
            sleep(1);
            timeoutCounter++;
        }
    }

    Common::PluginApi::StatusInfo PluginCallback::getStatus(const std::string& /* appId */)
    {
        LOGDEBUG("Received unexpected get status request");
        return Common::PluginApi::StatusInfo{};
    }

    std::string PluginCallback::getTelemetry()
    {
        LOGDEBUG("Received get telemetry request");
        auto& telemetry = Common::Telemetry::TelemetryHelper::getInstance();

        getHealthInner();
        
        std::optional<std::string> version = Plugin::getVersion();
        if (version)
        {
            telemetry.set("version", version.value());
        }
        std::string telemetryJson = telemetry.serialiseAndReset();
        LOGDEBUG("Got telemetry JSON data: " << telemetryJson);

        return telemetryJson;
    }

    void PluginCallback::setRunning(bool running)
    {
        m_running = running;
    }

    bool PluginCallback::isRunning()
    {
        return m_running;
    }

    uint PluginCallback::getHealthInner()
    {
        // TODO: Set health
        uint health = 0;
        Common::Telemetry::TelemetryHelper::getInstance().set("health", static_cast<u_long>(health));
        return health;
    }

    std::string PluginCallback::getHealth()
    {
        LOGDEBUG("Received health request");
        return "{'Health': " + std::to_string(getHealthInner()) + "}";
    }
} // namespace Plugin