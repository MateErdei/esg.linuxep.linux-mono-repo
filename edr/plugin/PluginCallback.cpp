/******************************************************************************************************

Copyright 2018 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "PluginCallback.h"
#include "Logger.h"
#include "Telemetry.h"

namespace
{
    bool isScanNow(const std::string & actionXml)
    {
        return actionXml.find("type=\"ScanNow\"") != std::string::npos;
    }

    bool isValidSAVAction(const std::string & actionXml)
    {
        if (actionXml.find("sophos.mgt.action") != std::string::npos)
        {
            return true;
        }
        return isScanNow(actionXml);
    }
}

namespace Example
{

    PluginCallback::PluginCallback(std::shared_ptr<QueueTask> task) :
        m_task(task), m_statusInfo()
    {
        std::string noPolicySetStatus{
                R"sophos(<?xml version="1.0" encoding="utf-8" ?>
                    <status xmlns="http://www.sophos.com/EE/EESavStatus">
                        <CompRes xmlns="com.sophos\msys\csc" Res="NoRef" RevID="" policyType="2" />
                    </status>)sophos"};
        Common::PluginApi::StatusInfo noPolicyStatusInfo = {noPolicySetStatus, noPolicySetStatus, "SAV"};
        m_statusInfo = noPolicyStatusInfo;
        LOGDEBUG("Plugin Callback Started");
    }

    void PluginCallback::applyNewPolicy(const std::string &policyXml)
    {
        LOGSUPPORT("Applying new policy");
        m_task->push(Task{Task::TaskType::Policy, policyXml});
    }

    void PluginCallback::queueAction(const std::string &actionXml)
    {
        LOGSUPPORT("Queueing action");
        if ( !isValidSAVAction(actionXml))
        {
            throw std::logic_error("Received invalid action for SAV app");
        }
        if( isScanNow(actionXml))
        {
            m_task->push( Task{Task::TaskType::ScanNow} );
        }
    }

    void PluginCallback::onShutdown()
    {
        LOGSUPPORT("Shutdown signal received");
        m_task->pushStop();
    }

    Common::PluginApi::StatusInfo PluginCallback::getStatus(const std::string &appId)
    {
        LOGSUPPORT("Received get status request");
        return m_statusInfo;
    }

    void PluginCallback::setStatus(Common::PluginApi::StatusInfo statusInfo)
    {
        LOGSUPPORT("Setting status");
        m_statusInfo = statusInfo;
    }

    std::string PluginCallback::getTelemetry()
    {
        LOGSUPPORT("Received get telemetry request");
        auto & telemetry = Telemetry::instance();
        std::string telemetryJson = telemetry.getJson();
        telemetry.clear();
        return telemetryJson;
    }
}