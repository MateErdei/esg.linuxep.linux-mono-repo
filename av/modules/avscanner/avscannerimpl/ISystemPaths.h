/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <string>

namespace avscanner::avscannerimpl
{
    class ISystemPaths
    {
    public:
        virtual std::string mountInfoFilePath() const = 0;
        virtual std::string cmdlineInfoFilePath() const = 0;
        virtual std::string findfsCmdPath() const = 0;
        virtual std::string mountCmdPath() const = 0;
    };
}
