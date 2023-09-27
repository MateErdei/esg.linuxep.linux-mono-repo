/******************************************************************************************************

Copyright 2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "../ApplicationPaths.h"
#include "../Logger.h"

#include <Common/TelemetryHelperImpl/TelemetryHelper.h>
#include <pluginimpl/PluginUtils.h>
#include "TelemetryConsts.h"

namespace
{
    const std::string PRODUCT_VERSION_STR = "PRODUCT_VERSION";
} // namespace

namespace Plugin
{
    namespace Telemetry
    {
        std::optional<std::string> getVersion()
        {

            return Plugin::PluginUtils::readKeyValueFromIniFile(PRODUCT_VERSION_STR, Plugin::getVersionIniFilePath());
        }

        void initialiseTelemetry()
        {
            auto& telemetry = Common::Telemetry::TelemetryHelper::getInstance();
            telemetry.increment(Plugin::Telemetry::totalSessionsTag, 0L);
            telemetry.increment(Plugin::Telemetry::failedSessionsTag, 0L);
        }
    }
} // namespace plugin
