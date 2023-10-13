/******************************************************************************************************

Copyright 2018-2021 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "PluginCallback.h"
#include "Logger.h"
#include "Telemetry.h"
#include "TelemetryConsts.h"
#include <Common/TelemetryHelperImpl/TelemetryHelper.h>
#include <modules/pluginimpl/ApplicationPaths.h>
#include <unistd.h>
#include <utility>

namespace Plugin
{
    PluginCallback::PluginCallback(std::shared_ptr<QueueTask> task, std::shared_ptr<Heartbeat::IHeartbeat> heartbeat) :
    m_task(std::move(task)),
    m_heartbeat(std::move(heartbeat))
    {
        std::string noPolicySetStatus{
            R"sophos(<?xml version="1.0" encoding="utf-8" ?>
                    <status xmlns="http://www.sophos.com/EE/EESavStatus">
                        <CompRes xmlns="com.sophos\msys\csc" Res="NoRef" RevID="" policyType="2" />
                    </status>)sophos"
        };
        Common::PluginApi::StatusInfo noPolicyStatusInfo = { noPolicySetStatus, noPolicySetStatus, "SAV" };
        m_statusInfo = noPolicyStatusInfo;
        LOGDEBUG("Plugin Callback Started");
    }

    void PluginCallback::applyNewPolicy(const std::string& policyXml)
    {
        LOGSUPPORT("Applying new policy");
        m_task->push(Task{ Task::TaskType::POLICY, policyXml});
    }

    void PluginCallback::queueAction(const std::string& /* actionXml */) { LOGSUPPORT("Queueing action"); }

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
        LOGSUPPORT("Received get telemetry request");
        auto& telemetry = Common::Telemetry::TelemetryHelper::getInstance();

        std::optional<std::string> version = Plugin::getVersion();
        if (version)
        {
            telemetry.set(Plugin::Telemetry::version, version.value());
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

    std::string PluginCallback::getHealth()
    {
        LOGDEBUG("Received health request");
        // 0 = good
        // 1 = bad

        int health = 1;

        // Check if any pingers have not been pinged by the owning thread/worker etc.
        auto missedHeartbeats = m_heartbeat->getMissedHeartbeats();
        if (missedHeartbeats.empty())
        {
            health = 0;
        }
        else
        {
            health = 1;
            for (const auto& missed: missedHeartbeats)
            {
                LOGDEBUG("A heartbeat ping was missed by: " << missed);
            }
        }
        m_heartbeat->resetAll();
        return "{'Health': " + std::to_string(health) + "}";
    }

} // namespace Plugin