// Copyright 2022, Sophos Limited.  All rights reserved.

#include "SafeStoreServiceCallback.h"

#include "common/ApplicationPaths.h"

#include "safestore/Logger.h"

namespace safestore
{
    void SafeStoreServiceCallback::applyNewPolicy(const std::string& policyXml)
    {
        LOGWARN("NotSupported: Received apply new policy: " << policyXml);
    }

    std::string SafeStoreServiceCallback::getHealth()
    {
        LOGDEBUG("Received health request");
        return "{}";
    }

    Common::PluginApi::StatusInfo SafeStoreServiceCallback::getStatus(const std::string& appid)
    {
        LOGWARN("NotSupported: Received getStatus");
        return Common::PluginApi::StatusInfo{ "", "", appid };
    }

    std::string SafeStoreServiceCallback::getTelemetry()
    {
        LOGDEBUG("Received get telemetry request");

        auto fileSystem = Common::FileSystem::fileSystem();
        bool dormant = fileSystem->isFile(Plugin::getSafeStoreDormantFlagPath());
        unsigned long healthValue = dormant ? 1UL : 0UL;

        Common::Telemetry::TelemetryHelper::getInstance().set("dormant-mode", dormant);
        Common::Telemetry::TelemetryHelper::getInstance().set("health", healthValue);


        return Common::Telemetry::TelemetryHelper::getInstance().serialiseAndReset();
    }

    void SafeStoreServiceCallback::queueAction(const std::string& action)
    {
        LOGWARN("NotSupported: Queue action: " << action);
    }
} // namespace safestore