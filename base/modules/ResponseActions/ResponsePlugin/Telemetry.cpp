// Copyright 2023 Sophos Limited. All rights reserved.

#include "Telemetry.h"

#include "ApplicationPaths.h"
#include "Logger.h"
#include "json.hpp"

#include <Common/UtilityImpl/StringUtils.h>

namespace
{
    const std::string PRODUCT_VERSION_STR = "PRODUCT_VERSION";
} // namespace

namespace ResponsePlugin
{
    TelemetryUtils::actionTelemetry runCommands;

    std::optional<std::string> getVersion()
    {
        try
        {
            std::string versionIniFilepath = ResponsePlugin::getVersionIniFilePath();
            return Common::UtilityImpl::StringUtils::extractValueFromIniFile(versionIniFilepath,PRODUCT_VERSION_STR);
        }
        catch (std::exception& ex)
        {
            LOGERROR("Telemetry cannot find the plugin version");
            return std::nullopt;
        }
    }

    TelemetryUtils::actionTelemetry TelemetryUtils::getRunCommandTelemetry()
    {
//        std::map<std::string, int> rcMap = { { "total-actions", runCommands.total },
//                                             { "total-failures", runCommands.totalFailures },
//                                             { "timeout-failures", runCommands.timeoutFailures },
//                                             { "expiration-failures", runCommands.expiryFailures } };

        return runCommands;
    }

    void TelemetryUtils::incrementTotalActions(const std::string& type)
    {
        if (type == "sophos.mgt.action.RunCommands")
        {
            runCommands.total += 1;
        }
    }

    void TelemetryUtils::incrementFailedActions(const std::string& type)
    {
        if (type == "sophos.mgt.action.RunCommands")
        {
            runCommands.totalFailures += 1;
        }
    }

    void TelemetryUtils::incrementTimedOutActions(const std::string& type)
    {
        if (type == "sophos.mgt.action.RunCommands")
        {
            runCommands.timeoutFailures += 1;
        }
    }

    void TelemetryUtils::incrementExpiredActions(const std::string& type)
    {
        if (type == "sophos.mgt.action.RunCommands")
        {
            runCommands.expiryFailures += 1;
        }
    }

} // namespace ResponsePlugin