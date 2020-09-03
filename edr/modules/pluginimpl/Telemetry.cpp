/******************************************************************************************************

Copyright 2018-2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ApplicationPaths.h"
#include "Telemetry.h"
#include "Logger.h"
#include "StringReplace.h"

#include <Common/UtilityImpl/StringUtils.h>

namespace
{
    const std::string PRODUCT_VERSION_STR = "PRODUCT_VERSION";
} // namespace

namespace Plugin
{
    std::optional<std::string> getVersion()
    {
        try
        {
            std::string versionIniFilepath = Plugin::getVersionIniFilePath();
            return Common::UtilityImpl::StringUtils::extractValueFromIniFile(versionIniFilepath,PRODUCT_VERSION_STR);
        }
        catch (std::exception& ex)
        {
            LOGERROR("Telemetry cannot find the plugin version");
            return std::nullopt;
        }
    }
} // namespace Plugin