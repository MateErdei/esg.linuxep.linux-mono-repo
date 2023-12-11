// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "Logger.h"
#include "PluginCallback.h"
#include "PluginUtils.h"
#include "Telemetry.h"

#include "EdrCommon/TelemetryConsts.h"

#include "Common/TelemetryHelperImpl/TelemetryHelper.h"

#include <thread>

#include <nlohmann/json.hpp>
#include <unistd.h>

namespace Plugin
{
    PluginCallback::PluginCallback(std::shared_ptr<QueueTask> task) : m_task(std::move(task))
    {
        std::string noPolicySetStatus {
            R"sophos(<?xml version="1.0" encoding="utf-8" ?>
                    <status xmlns="http://www.sophos.com/EE/EESavStatus">
                        <CompRes xmlns="com.sophos\msys\csc" Res="NoRef" RevID="" policyType="2" />
                    </status>)sophos"
        };
        Common::PluginApi::StatusInfo noPolicyStatusInfo = { noPolicySetStatus, noPolicySetStatus, "SAV" };
        m_statusInfo = noPolicyStatusInfo;
        m_osqueryShouldBeRunning = false;
        m_osqueryRunning = false;
        LOGDEBUG("Plugin Callback Started");
    }

    void PluginCallback::applyNewPolicy(const std::string& /* policyXml */)
    {
        LOGERROR("This method should never be called.");
    }

    void PluginCallback::applyNewPolicyWithAppId(const std::string& appId, const std::string& policyXml)
    {
        LOGSUPPORT("Applying new policy with APPID: " << appId);
        m_task->push(Task { Task::TaskType::POLICY, policyXml, "", appId });
    }

    void PluginCallback::queueAction(const std::string& /* actionXml */)
    {
        LOGERROR("This method should never be called.");
    }

    void PluginCallback::queueActionWithCorrelation(const std::string& queryJson, const std::string& correlationId)
    {
        if (m_running && m_osqueryShouldBeRunning)
        {
            LOGSUPPORT("Receive new query");
            m_task->push(Task { Task::TaskType::QUERY, queryJson, correlationId });
        }
    }

    void PluginCallback::onShutdown()
    {
        LOGSUPPORT("Shutdown signal received");
        m_task->pushStop();

        using namespace std::chrono_literals;
        using clock_t = std::chrono::steady_clock;

        constexpr auto shutdownTimeout = 20s;
        auto deadline = clock_t::now() + shutdownTimeout;
        auto nextLog = clock_t::now() + 1s;

        while(isRunning() && clock_t::now() < deadline)
        {
            if ( clock_t::now() > nextLog )
            {
                nextLog = clock_t::now() + 1s;
                LOGSUPPORT("Shutdown waiting for all processes to complete");
            }
            std::this_thread::sleep_for(50ms);
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

    void PluginCallback::setOsqueryRunning(bool running)
    {
        m_osqueryRunning = running;
    }
    void PluginCallback::setOsqueryShouldBeRunning(bool shouldBeRunning)
    {
        m_osqueryShouldBeRunning = shouldBeRunning;
    }

    std::string PluginCallback::getTelemetry()
    {
        LOGSUPPORT("Received get telemetry request");
        auto& telemetry = Common::Telemetry::TelemetryHelper::getInstance();

        std::optional<std::string> version = plugin::getVersion();
        if (version)
        {
            telemetry.set(plugin::version, version.value());
        }
        bool isXDR;
        try
        {
            isXDR = Plugin::PluginUtils::retrieveGivenFlagFromSettingsFile(PluginUtils::MODE_IDENTIFIER);
        }
        catch (const std::runtime_error& ex)
        {
            // not set in plugin.conf default to false
            isXDR = false;
        }
        telemetry.set(plugin::telemetryIsXdrEnabled, isXDR);

        std::optional<unsigned long> osqueryDatabaseSize = plugin::getOsqueryDatabaseSize();
        if (osqueryDatabaseSize)
        {
            telemetry.set(plugin::telemetryOSQueryDatabaseSize, osqueryDatabaseSize.value());
        }
        plugin::readOsqueryInfoFiles();

        long health = 0;
        if (m_osqueryShouldBeRunning && !m_osqueryRunning )
        {
            health = 1;
        }
        telemetry.set(plugin::health,health);
        std::string storedTelemetry = telemetry.serialise();

        if (storedTelemetry.find(plugin::telemetryUploadLimitHitQueries) == std::string::npos)
        {
            std::string key = std::string(plugin::telemetryScheduledQueries)+ "." + plugin::telemetryUploadLimitHitQueries;
            telemetry.set(key,false);
        }
        std::string telemetryJson = telemetry.serialiseAndReset();
        LOGDEBUG("Got telemetry JSON data: " << telemetryJson);

        initialiseTelemetry();
        return telemetryJson;
    }

    void PluginCallback::initialiseTelemetry()
    {
        auto& telemetry = Common::Telemetry::TelemetryHelper::getInstance();
        telemetry.increment(plugin::telemetryOsqueryRestarts, 0L);
        telemetry.increment(plugin::telemetryOSQueryRestartsCPU, 0L);
        telemetry.increment(plugin::telemetryOSQueryRestartsMemory, 0L);
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