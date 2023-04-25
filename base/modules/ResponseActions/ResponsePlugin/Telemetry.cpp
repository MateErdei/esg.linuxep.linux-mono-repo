// Copyright 2023 Sophos Limited. All rights reserved.

#include "Telemetry.h"

#include "ApplicationPaths.h"
#include "Logger.h"

#include "Common/TelemetryHelperImpl/TelemetryHelper.h"

#include <Common/UtilityImpl/StringUtils.h>

namespace
{
    const std::string PRODUCT_VERSION_STR = "PRODUCT_VERSION";
} // namespace

namespace ResponsePlugin
{
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

    void incrementTotalActions(const std::string& type)
    {
        if (type == "sophos.mgt.action.RunCommands")
        {
            Common::Telemetry::TelemetryHelper::getInstance().increment("run-command-actions", 1UL);
        }
    }

    void incrementFailedActions(const std::string& type)
    {
        if (type == "sophos.mgt.action.RunCommands")
        {
            Common::Telemetry::TelemetryHelper::getInstance().increment("run-command-failed", 1UL);
        }
    }

    void incrementTimedOutActions(const std::string& type)
    {
        if (type == "sophos.mgt.action.RunCommands")
        {
            Common::Telemetry::TelemetryHelper::getInstance().increment("run-command-timeout-actions", 1UL);
        }
    }

    void incrementExpiredActions(const std::string& type)
    {
        if (type == "sophos.mgt.action.RunCommands")
        {
            Common::Telemetry::TelemetryHelper::getInstance().increment("run-command-expired-actions", 1UL);
        }
    }

} // namespace ResponsePlugin