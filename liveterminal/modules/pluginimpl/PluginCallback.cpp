// Copyright 2023 Sophos Limited. All rights reserved.

#include "PluginCallback.h"
#include "Logger.h"
#include "Telemetry/Telemetry.h"
#include "common/TelemetryConsts.h"
#include "Common/TelemetryHelperImpl/TelemetryHelper.h"
#include <unistd.h>

namespace Plugin
{
    PluginCallback::PluginCallback(std::shared_ptr<QueueTask> task) : m_task(std::move(task))
    {
        LOGDEBUG("Plugin Callback Started");
    }

    void PluginCallback::applyNewPolicy(const std::string& policyXml)
    {
        LOGSUPPORT("Applying new policy");
        m_task->push(Task{ Task::TaskType::Policy, policyXml });
    }

    void PluginCallback::queueAction(const std::string& /* actionXml */)
    {
        LOGERROR("This method should never be called.");
    }

    void PluginCallback::queueActionWithCorrelation(const std::string& actionXml, const std::string& correlationId)
    {
        LOGSUPPORT("Receive new live response session request");
        m_task->push(Task { Task::TaskType::Response, actionXml, correlationId });
    }

    void PluginCallback::onShutdown()
    {
        LOGSUPPORT("Shutdown signal received");
        m_task->pushStop();
        int timeoutCounter = 0;
        int shutdownTimeout = 30;
        while(isRunning() && timeoutCounter < shutdownTimeout)
        {
            LOGSUPPORT("Shutdown waiting for all processes to complete");
            sleep(1);
            timeoutCounter++;
        }
    }

    Common::PluginApi::StatusInfo PluginCallback::getStatus(const std::string& /* appId */)
    {
        LOGSUPPORT("Received unexpected get status request");
        return Common::PluginApi::StatusInfo{};
    }

    std::string PluginCallback::getTelemetry()
    {
        LOGSUPPORT("Received get telemetry request");
        auto& telemetry = Common::Telemetry::TelemetryHelper::getInstance();

        std::optional<std::string> version = Plugin::Telemetry::getVersion();
        if (version)
        {
            telemetry.set(Plugin::Telemetry::versionTag, version.value());
        }
        telemetry.set("health",0UL);
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

    std::string PluginCallback::getHealth()
    {
        LOGDEBUG("Received health request");
        return "{}";
    }
} // namespace Plugin