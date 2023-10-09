/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <string>
#include <vector>

namespace Common::Process
{
    using EnvironmentPair = std::pair<std::string, std::string>;
    using EnvPairVector = std::vector<EnvironmentPair>;
} // namespace Common::Process
