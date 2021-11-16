/******************************************************************************************************

Copyright 2018-2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Logger.h"
#include "UpdateSchedulerProcessor.h"
#include "SchedulerPluginCallback.h"
#include "UpdateSchedulerUtils.h"
#include <UpdateSchedulerImpl/stateMachinesModule/StateMachineProcessor.h>

#include <Common/PluginApi/ApiException.h>
#include <Common/TelemetryHelperImpl/TelemetryHelper.h>
#include <json.hpp>
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

        auto currentTime = Common::UtilityImpl::TimeUtils::fromTime(Common::UtilityImpl::TimeUtils::getCurrTime());
        stateMachinesModule::StateMachineProcessor stateMachineProcessor(currentTime);
        m_stateMachineData = stateMachineProcessor.getStateMachineData();
        LOGDEBUG("Plugin Callback Started");
    }

    void SchedulerPluginCallback::applyNewPolicy(const std::string& policyXml)
    {
        LOGDEBUG("Applying new policy");
        m_task->push(SchedulerTask{ SchedulerTask::TaskType::Policy, policyXml });
    }

    void SchedulerPluginCallback::queueAction(const std::string& actionXml)
    {
        LOGDEBUG("API received action");
        m_task->push(SchedulerTask{ SchedulerTask::TaskType::UpdateNow, actionXml });
    }

    void SchedulerPluginCallback::onShutdown()
    {
        LOGDEBUG("Shutdown signal received");
        m_shutdownReceived = true;
        m_task->push(SchedulerTask{ SchedulerTask::TaskType::ShutdownReceived, "" });

        int timeoutCounter = 0;
        int shutdownTimeout = 30;
        while(isRunning() && timeoutCounter < shutdownTimeout)
        {
            LOGDEBUG("Shutdown waiting for all processes to complete");
            sleep(1);
            timeoutCounter++;
        }

    }

    Common::PluginApi::StatusInfo SchedulerPluginCallback::getStatus(const std::string& /*appId*/)
    {
        LOGDEBUG("Received get status request");
        if (m_statusInfo.statusXml.empty())
        {
            LOGWARN("Status has not been configured yet.");
            throw Common::PluginApi::ApiException("Status not set yet. ");
        }
        return m_statusInfo;
    }

    std::string SchedulerPluginCallback::getHealth()
    {
        nlohmann::json healthjson =  nlohmann::json::parse(UpdateSchedulerUtils::calculateHealth(m_stateMachineData));
        nlohmann::json healthResponseMessage;
        healthResponseMessage["Health"] = healthjson["overall"];
        return healthResponseMessage.dump();
    }

    void SchedulerPluginCallback::setStatus(Common::PluginApi::StatusInfo statusInfo)
    {
        LOGDEBUG("Setting status");
        m_statusInfo = std::move(statusInfo);
    }
    void SchedulerPluginCallback::setStateMachine(StateData::StateMachineData stateMachineData)
    {
        m_stateMachineData = stateMachineData;
    }

    std::string SchedulerPluginCallback::getTelemetry()
    {
        LOGDEBUG("Received get telemetry request");

        // Ensure counts are always reported:
        Common::Telemetry::TelemetryHelper::getInstance().increment("failed-update-count", 0UL);
        Common::Telemetry::TelemetryHelper::getInstance().increment("failed-downloader-count", 0UL);
        nlohmann::json health = nlohmann::json::parse(UpdateSchedulerUtils::calculateHealth(m_stateMachineData));
        long overallHealth = health["overall"];
        Common::Telemetry::TelemetryHelper::getInstance().set("health",overallHealth);
        long installState = health["installState"];
        Common::Telemetry::TelemetryHelper::getInstance().set("install-state",installState);
        long downloadState = health["downloadState"];
        Common::Telemetry::TelemetryHelper::getInstance().set("download-state",downloadState);
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