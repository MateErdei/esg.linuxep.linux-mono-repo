// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "SchedulerPluginCallback.h"

#include "UpdateSchedulerProcessor.h"
#include "UpdateSchedulerTelemetryConsts.h"
#include "UpdateSchedulerUtils.h"

#include "Common/PluginApi/ApiException.h"
#include "Common/TelemetryHelperImpl/TelemetryHelper.h"
#include "common/Logger.h"
#include "stateMachinesModule/StateMachineProcessor.h"

#include <json.hpp>
#include <unistd.h>
#include <utility>

namespace UpdateSchedulerImpl
{
    using namespace UpdateScheduler;
    SchedulerPluginCallback::SchedulerPluginCallback(std::shared_ptr<SchedulerTaskQueue> task) :
        m_task(std::move(task)), m_statusInfo(), m_shutdownReceived(false)
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

    void SchedulerPluginCallback::applyNewPolicy(const std::string& /* policyXml */)
    {
        LOGERROR("Attempted to apply new policy without AppId: This method should never be called.");
    }

    void SchedulerPluginCallback::applyNewPolicyWithAppId(const std::string& appId, const std::string& policyXml)
    {
        LOGDEBUG("Applying new policy");
        m_task->push(SchedulerTask{ SchedulerTask::TaskType::Policy, policyXml, appId });
    }

    void SchedulerPluginCallback::queueAction(const std::string& actionXml)
    {
        LOGDEBUG("API received action");
        m_task->push(SchedulerTask{ SchedulerTask::TaskType::UpdateNow, actionXml });
    }

    void SchedulerPluginCallback::onShutdown()
    {
        LOGDEBUG("Shutdown signal received");
        std::unique_lock lock{ runningMutex_ };
        m_shutdownReceived = true;
        m_task->push(SchedulerTask{ SchedulerTask::TaskType::ShutdownReceived, "" });

        int timeoutCounter = 0;
        int shutdownTimeout = 30;
        while (isRunningLocked(lock) && timeoutCounter < shutdownTimeout)
        {
            LOGDEBUG("Shutdown waiting for all processes to complete");
            runningConditionVariable_.wait_for(lock, std::chrono::seconds{ 1 });
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
        nlohmann::json healthjson = nlohmann::json::parse(UpdateSchedulerUtils::calculateHealth(m_stateMachineData));
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
        Common::Telemetry::TelemetryHelper::getInstance().increment(Telemetry::failedUpdateCount, 0UL);
        Common::Telemetry::TelemetryHelper::getInstance().increment(Telemetry::failedDownloaderCount, 0UL);
        nlohmann::json health = nlohmann::json::parse(UpdateSchedulerUtils::calculateHealth(m_stateMachineData));
        long overallHealth = health["overall"];
        Common::Telemetry::TelemetryHelper::getInstance().set(Telemetry::pluginHealthStatus, overallHealth);
        long installState = health["installState"];
        Common::Telemetry::TelemetryHelper::getInstance().set(Telemetry::installState, installState);
        long downloadState = health["downloadState"];
        Common::Telemetry::TelemetryHelper::getInstance().set(Telemetry::downloadState, downloadState);
        return Common::Telemetry::TelemetryHelper::getInstance().serialiseAndReset();
    }

    bool SchedulerPluginCallback::shutdownReceived()
    {
        return m_shutdownReceived;
    }

    void SchedulerPluginCallback::setRunning(bool running)
    {
        std::lock_guard guard{ runningMutex_ };
        m_running = running;
        runningConditionVariable_.notify_all();
    }

    bool SchedulerPluginCallback::isRunning()
    {
        std::unique_lock lock{ runningMutex_ };
        return isRunningLocked(lock);
    }

    bool SchedulerPluginCallback::isRunningLocked(std::unique_lock<std::mutex>&)
    {
        return m_running;
    }
} // namespace UpdateSchedulerImpl