/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SchedulerPluginCallback.h"

#include "Logger.h"
#include "UpdateSchedulerProcessor.h"

#include <Common/PluginApi/ApiException.h>
#include <Common/TelemetryHelperImpl/TelemetryHelper.h>

#include <utility>
#include <unistd.h>

namespace UpdateSchedulerImpl
{
    using namespace UpdateScheduler;
    SchedulerPluginCallback::SchedulerPluginCallback(std::shared_ptr<SchedulerTaskQueue> task) :
        m_task(std::move(task)),
        m_statusInfo(),
        m_shutdownReceived(false)
    {
        std::string noPolicySetStatus{
            R"sophos(<?xml version="1.0" encoding="utf-8" ?>
                    <status xmlns="com.sophos\mansys\status" type="sau">
                        <CompRes xmlns="com.sophos\msys\csc" Res="NoRef" RevID="" policyType="1" />
                    </status>)sophos"
        };

        m_statusInfo =
            Common::PluginApi::StatusInfo{ noPolicySetStatus, noPolicySetStatus, UpdateSchedulerProcessor::getAppId() };

        LOGDEBUG("Plugin Callback Started");
    }

    void SchedulerPluginCallback::applyNewPolicy(const std::string& policyXml)
    {
        LOGSUPPORT("Applying new policy");
        m_task->push(SchedulerTask{ SchedulerTask::TaskType::Policy, policyXml });
    }

    void SchedulerPluginCallback::queueAction(const std::string& actionXml)
    {
        LOGSUPPORT("API received action");
        m_task->push(SchedulerTask{ SchedulerTask::TaskType::UpdateNow, actionXml });
    }

    void SchedulerPluginCallback::onShutdown()
    {
        LOGSUPPORT("Shutdown signal received");
        m_shutdownReceived = true;
        m_task->push(SchedulerTask{ SchedulerTask::TaskType::ShutdownReceived, "" });

        int timeoutCounter = 0;
        int shutdownTimeout = 30;
        while(isRunning() && timeoutCounter < shutdownTimeout)
        {
            LOGSUPPORT("Shutdown waiting for all processes to complete");
            sleep(1);
            timeoutCounter++;
        }

    }

    Common::PluginApi::StatusInfo SchedulerPluginCallback::getStatus(const std::string& /*appId*/)
    {
        LOGSUPPORT("Received get status request");
        if (m_statusInfo.statusXml.empty())
        {
            LOGWARN("Status has not been configured yet.");
            throw Common::PluginApi::ApiException("Status not set yet. ");
        }
        return m_statusInfo;
    }

    void SchedulerPluginCallback::setStatus(Common::PluginApi::StatusInfo statusInfo)
    {
        LOGSUPPORT("Setting status");
        m_statusInfo = std::move(statusInfo);
    }

    std::string SchedulerPluginCallback::getTelemetry()
    {
        LOGSUPPORT("Received get telemetry request");

        // Ensure counts are always reported:
        Common::Telemetry::TelemetryHelper::getInstance().increment("failed-update-count", 0UL);
        Common::Telemetry::TelemetryHelper::getInstance().increment("failed-downloader-count", 0UL);

        return Common::Telemetry::TelemetryHelper::getInstance().serialiseAndReset();
    }

    std::string SchedulerPluginCallback::getHealth()
    {
        LOGDEBUG("Received health request");
        return "{}";
    }

    bool SchedulerPluginCallback::shutdownReceived() { return m_shutdownReceived; }

    void SchedulerPluginCallback::setRunning(bool running)
    {
        m_running = running;
    }

    bool SchedulerPluginCallback::isRunning()
    {
        return m_running;
    }
} // namespace UpdateSchedulerImpl