/******************************************************************************************************

Copyright 2018 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/PluginApi/ApiException.h>
#include "SchedulerPluginCallback.h"
#include "Logger.h"
#include "UpdateSchedulerProcessor.h"

namespace UpdateSchedulerImpl
{
    using namespace UpdateScheduler;
    SchedulerPluginCallback::SchedulerPluginCallback(std::shared_ptr<SchedulerTaskQueue> task) :
        m_task(task), m_statusInfo()
    {
        std::string noPolicySetStatus{
            R"sophos(<?xml version="1.0" encoding="utf-8" ?>
                    <status xmlns="com.sophos\mansys\status" type="sau">
                        <CompRes xmlns="com.sophos\msys\csc" Res="NoRef" RevID="" policyType="1" />
                    </status>)sophos"};

        m_statusInfo = Common::PluginApi::StatusInfo{
            noPolicySetStatus, noPolicySetStatus, UpdateSchedulerProcessor::getAppId()};

        LOGDEBUG("Plugin Callback Started");
    }

    void SchedulerPluginCallback::applyNewPolicy(const std::string &policyXml)
    {
        LOGSUPPORT("Applying new policy");
        m_task->push(SchedulerTask{SchedulerTask::TaskType::Policy, policyXml});
    }

    void SchedulerPluginCallback::queueAction(const std::string &actionXml)
    {
        LOGSUPPORT("API received action");
        m_task->push(SchedulerTask{SchedulerTask::TaskType::UpdateNow, actionXml});
    }

    void SchedulerPluginCallback::onShutdown()
    {
        LOGSUPPORT("Shutdown signal received");
        m_task->push(SchedulerTask{SchedulerTask::TaskType::ShutdownReceived, ""});

    }

    Common::PluginApi::StatusInfo SchedulerPluginCallback::getStatus(const std::string &appId)
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
        m_statusInfo = statusInfo;

    }

    std::string SchedulerPluginCallback::getTelemetry()
    {
        LOGSUPPORT("Received get telemetry request");
        return std::string();
    }
}