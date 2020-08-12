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
        [[nodiscard]] std::string mountInfoFilePath() const override
        {
            return "/proc/mounts";
        }

        [[nodiscard]] std::string cmdlineInfoFilePath() const override
        {
            return "/proc/cmdline";
        }

        [[nodiscard]] std::string findfsCmdPath() const override
        {
            return "/sbin/findfs";
        }

        [[nodiscard]] std::string mountCmdPath() const override
        {
            return "/bin/mount";
        }
    };
}
