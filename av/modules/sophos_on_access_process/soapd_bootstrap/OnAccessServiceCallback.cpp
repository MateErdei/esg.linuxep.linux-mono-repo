// Copyright 2022-2023 Sophos Limited. All rights reserved.

#include "OnAccessServiceCallback.h"

#include "Logger.h"

#include "sophos_on_access_process/OnAccessTelemetryFields/OnAccessTelemetryFields.h"

#include <utility>

using namespace sophos_on_access_process::service_callback;
using namespace sophos_on_access_process::onaccessimpl::onaccesstelemetry;

OnAccessServiceCallback::OnAccessServiceCallback(IOnAccessTelemetryUtilitySharedPtr telemetryUtility) :
m_telemetryUtility(std::move(telemetryUtility))
{

}

std::string OnAccessServiceCallback::getTelemetry()
{
    LOGDEBUG("Received get telemetry request");
    auto onAccessScanData = m_telemetryUtility->getTelemetry();
    Common::Telemetry::TelemetryHelper::getInstance().set(RATIO_DROPPED_EVENTS, onAccessScanData.m_percentageEventsDropped);
    Common::Telemetry::TelemetryHelper::getInstance().set(RATIO_SCAN_ERRORS, onAccessScanData.m_percentageScanErrors);
    return Common::Telemetry::TelemetryHelper::getInstance().serialiseAndReset();
}


//The below are not used by OnAccess
void OnAccessServiceCallback::applyNewPolicy(const std::string& policyXml)
{
    LOGWARN("NotSupported: Received apply new policy: " << policyXml);
}

std::string OnAccessServiceCallback::getHealth()
{
    LOGDEBUG("NotSupported: Received health request");
    return "{}";
}

Common::PluginApi::StatusInfo OnAccessServiceCallback::getStatus(const std::string& appId)
{
    LOGWARN("NotSupported: Received getStatus");
    return { "", "", appId };
}

void OnAccessServiceCallback::queueAction(const std::string& action)
{
    LOGWARN("NotSupported: Queue action: " << action);
}
