/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "MessageRelay.h"

#include <map>
#include <string>
#include <vector>

namespace MCS
{
    struct ConfigOptions
    {
        std::vector<MCS::MessageRelay> messageRelays;
        std::map<std::string, std::string> config;

        void writeToDisk(const std::string& fullPathToOutFile) const;
    };

} // namespace MCS