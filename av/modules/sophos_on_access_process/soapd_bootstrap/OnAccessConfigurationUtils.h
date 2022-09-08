/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <string>
#include <vector>

namespace sophos_on_access_process::OnAccessConfig
{
    struct OnAccessConfiguration
    {
        bool enabled;
        bool excludeRemoteFiles;
        std::vector<std::string> exclusions;
    };

    bool inline operator==(const OnAccessConfiguration& lhs, const OnAccessConfiguration& rhs)
    {
        return lhs.enabled == rhs.enabled
               && lhs.excludeRemoteFiles == rhs.excludeRemoteFiles
               && lhs.exclusions == rhs.exclusions;
    }

    bool inline operator!=(const OnAccessConfiguration& lhs, const OnAccessConfiguration& rhs)
    {
        return !(lhs == rhs);
    }

    std::string readConfigFile();
    OnAccessConfiguration parseOnAccessSettingsFromJson(const std::string& jsonString);
    bool isSettingTrue(const std::string& settingString);
}