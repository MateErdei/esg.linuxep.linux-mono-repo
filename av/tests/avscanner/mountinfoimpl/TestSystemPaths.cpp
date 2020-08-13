/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "avscanner/mountinfoimpl/SystemPaths.h"

#include <gtest/gtest.h>

using namespace avscanner::mountinfoimpl;

TEST(SystemPaths, TestSystemPaths) // NOLINT
{
    SystemPaths systemPaths;
    EXPECT_EQ(systemPaths.mountInfoFilePath(), "/proc/mounts");
    EXPECT_EQ(systemPaths.cmdlineInfoFilePath(), "/proc/cmdline");
    EXPECT_EQ(systemPaths.findfsCmdPath(), "/sbin/findfs");
    EXPECT_EQ(systemPaths.mountCmdPath(), "/bin/mount");
}