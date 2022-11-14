// Copyright 2022, Sophos Limited.  All rights reserved.

#include "OnAccessServiceCallback.h"

#include "common/ApplicationPaths.h"

#include "sophos_on_access_process/soapd_bootstrap/Logger.h"

using namespace sophos_on_access_process::service_callback;

std::string OnAccessServiceCallback::getTelemetry()
{
    LOGDEBUG("Received get telemetry request");

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

Common::PluginApi::StatusInfo OnAccessServiceCallback::getStatus(const std::string& appid)
{
    LOGWARN("NotSupported: Received getStatus");
    return Common::PluginApi::StatusInfo{ "", "", appid };
}

void OnAccessServiceCallback::queueAction(const std::string& action)
{
    LOGWARN("NotSupported: Queue action: " << action);
}
