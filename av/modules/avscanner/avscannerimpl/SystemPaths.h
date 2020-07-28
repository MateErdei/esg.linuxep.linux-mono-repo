/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "ISystemPaths.h"

namespace avscanner::avscannerimpl
{
    class SystemPaths : public ISystemPaths
    {
    public:
        std::string mountInfoFilePath() const
        {
            return "/proc/mounts";
        }

        std::string cmdlineInfoFilePath() const
        {
            return "/proc/cmdline";
        }

        std::string findfsCmdPath() const
        {
            return "/sbin/findfs";
        }

        std::string mountCmdPath() const
        {
            return "/bin/mount";
        }
    };
}
