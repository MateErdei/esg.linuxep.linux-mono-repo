/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "mount_monitor/mountinfo/ISystemPaths.h"

namespace mount_monitor::mountinfoimpl
{
    class SystemPaths : public mountinfo::ISystemPaths
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
