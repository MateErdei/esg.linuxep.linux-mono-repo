// Copyright 2023 Sophos Limited. All rights reserved.

#include "PluginCallback.h"

#include "Logger.h"
#include "Telemetry.h"
#include "TelemetryConsts.h"

#include <Common/TelemetryHelperImpl/TelemetryHelper.h>

#include <unistd.h>

namespace Plugin
{
    PluginCallback::PluginCallback(std::shared_ptr<QueueTask> task) : m_task(std::move(task))
    {
        LOGDEBUG("Plugin Callback Started");
    }

    void PluginCallback::applyNewPolicy(const std::string& policyXml)
    {
        LOGDEBUG("Unexpected policy recieved");
    }

    void PluginCallback::queueAction(const std::string&  actionXml )
    {
        m_task->push(Task{ Task::TaskType::ACTION, actionXml });
    }

    void PluginCallback::onShutdown()
    {
        LOGDEBUG("Shutdown signal received");
        m_task->pushStop();
        int timeoutCounter = 0;
        int shutdownTimeout = 30;
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

        std::optional<std::string> version = Plugin::getVersion();
        if (version)
        {
            telemetry.set(Plugin::Telemetry::version, version.value());
        }
        Common::Telemetry::TelemetryHelper::getInstance().set(Telemetry::pluginHealthStatus, static_cast<u_long>(1));
        std::string telemetryJson = telemetry.serialiseAndReset();
        LOGDEBUG("Got telemetry JSON data: " << telemetryJson);

        return telemetryJson;
    }

    std::string PluginCallback::getHealth()
    {
        LOGDEBUG("Received health request");
        return "{}";
    }

    void PluginCallback::setRunning(bool running)
    {
        m_running = running;
    }

    bool PluginCallback::isRunning()
    {
        return m_running;
    }

} // namespace Plugin