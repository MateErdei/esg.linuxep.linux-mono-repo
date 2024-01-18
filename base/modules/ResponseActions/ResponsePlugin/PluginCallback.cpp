// Copyright 2023 Sophos Limited. All rights reserved.

#include "PluginCallback.h"

#include "Logger.h"
#include "Telemetry.h"
#include "TelemetryConsts.h"

#include "Common/TelemetryHelperImpl/TelemetryHelper.h"

#include <chrono>
#include <thread>

using namespace Common::Telemetry;
using namespace std::chrono_literals;

namespace ResponsePlugin
{
    PluginCallback::PluginCallback(std::shared_ptr<TaskQueue> task) : m_task(std::move(task))
    {
        LOGDEBUG("Plugin Callback Started");
    }

    void PluginCallback::applyNewPolicy(const std::string& /*policyXml*/)
    {
        LOGDEBUG("Unexpected policy received");
    }

    void PluginCallback::queueAction(const std::string& /* actionXml */)
    {
        LOGDEBUG("Throwing away unwanted action");
    }

    void PluginCallback::queueActionWithCorrelation(const std::string& actionJson, const std::string& correlationId)
    {
        m_task->push(Task{ Task::TaskType::ACTION, actionJson, "", correlationId });
    }

    void PluginCallback::onShutdown()
    {
        LOGDEBUG("Shutdown signal received");
        m_task->pushStop();

        auto deadline = std::chrono::steady_clock::now() + 30s;
        auto nextLog = std::chrono::steady_clock::now() + 1s;

        while (isRunning() && std::chrono::steady_clock::now() < deadline)
        {
            if (std::chrono::steady_clock::now() > nextLog)
            {
                nextLog = std::chrono::steady_clock::now() + 1s;
                LOGDEBUG("Shutdown waiting for all processes to complete");
            }
            std::this_thread::sleep_for(50ms);
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
        auto& telemetry = TelemetryHelper::getInstance();

        for (const auto& field : Telemetry::TELEMETRY_FIELDS)
        {
            telemetry.increment(field, 0UL);
        }

        std::optional<std::string> version = ResponsePlugin::Telemetry::getVersion();

        if (version)
        {
            telemetry.set(ResponsePlugin::Telemetry::version, version.value());
        }
        telemetry.set(Telemetry::pluginHealthStatus, static_cast<u_long>(0));
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

} // namespace ResponsePlugin