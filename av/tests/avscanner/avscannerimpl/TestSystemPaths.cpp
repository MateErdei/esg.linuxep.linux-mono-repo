/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gtest/gtest.h>

#include "avscanner/avscannerimpl/SystemPaths.h"

using namespace avscanner::avscannerimpl;

TEST(SystemPaths, TestSystemPaths) // NOLINT
{
    SystemPaths systemPaths;
    EXPECT_EQ(systemPaths.mountInfoFilePath(), "/proc/mounts");
    EXPECT_EQ(systemPaths.cmdlineInfoFilePath(), "/proc/cmdline");
    EXPECT_EQ(systemPaths.findfsCmdPath(), "/sbin/findfs");
    EXPECT_EQ(systemPaths.mountCmdPath(), "/bin/mount");
}