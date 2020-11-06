/******************************************************************************************************

Copyright 2018-2019 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "PluginCallback.h"

#include "Logger.h"
#include "Telemetry.h"
#include "TelemetryConsts.h"
#include "PluginUtils.h"

#include <Common/TelemetryHelperImpl/TelemetryHelper.h>

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
        LOGDEBUG("Plugin Callback Started");
    }

    void PluginCallback::applyNewPolicy(const std::string& /* policyXml */)
    {
        LOGERROR("This method should never be called.");
    }

    void PluginCallback::applyNewPolicyWithAppId(const std::string& appId, const std::string& policyXml)
    {
        LOGSUPPORT("Applying new policy with APPID: " << appId);
        m_task->push(Task { Task::TaskType::Policy, policyXml, "", appId });
    }

    void PluginCallback::queueAction(const std::string& /* actionXml */)
    {
        LOGERROR("This method should never be called.");
    }

    void PluginCallback::queueActionWithCorrelation(const std::string& queryJson, const std::string& correlationId)
    {
        LOGSUPPORT("Receive new query");
        m_task->push(Task { Task::TaskType::Query, queryJson, correlationId });
    }

    void PluginCallback::onShutdown()
    {
        LOGSUPPORT("Shutdown signal received");
        m_task->pushStop();
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

        telemetry.updateTelemetryWithStats();
        telemetry.getStatStdDeviation(plugin::telemetryRecordSize);

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
        telemetry.increment(plugin::telemetryOSQueryDatabasePurges, 0L);
    }
} // namespace Plugin