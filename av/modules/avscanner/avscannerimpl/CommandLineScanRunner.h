/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <string>
#include <vector>

namespace avscanner::avscannerimpl
{
    class CommandLineScanRunner
    {
    public:
        int run(const std::vector<std::string>& paths);
    };
}



