/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <string>
#include <vector>

namespace sophos_on_access_process::ConfigMonitorThread
{
    struct OnAccessConfiguration
    {
        bool enabled;
        bool excludeRemoteFiles;
        std::vector<std::string> exclusions;
    };

    bool operator ==(const OnAccessConfiguration& lhs, const OnAccessConfiguration& rhs)
    {
        return lhs.enabled == rhs.enabled
               && lhs.excludeRemoteFiles == rhs.excludeRemoteFiles
               && lhs.exclusions == rhs.exclusions;
    }

    bool operator !=(const OnAccessConfiguration& lhs, const OnAccessConfiguration& rhs)
    {
        return !(lhs == rhs);
    }
}