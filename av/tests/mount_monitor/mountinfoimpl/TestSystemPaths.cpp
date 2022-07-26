/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "mount_monitor/mountinfoimpl/SystemPaths.h"

#include "tests/common/LogInitializedTests.h"

#include <gtest/gtest.h>

using namespace mount_monitor::mountinfoimpl;

class TestSystemPaths : public LogInitializedTests
{
};

TEST_F(TestSystemPaths, test_paths_are_correct) // NOLINT
{
    SystemPaths systemPaths;
    EXPECT_EQ(systemPaths.mountInfoFilePath(), "/proc/mounts");
    EXPECT_EQ(systemPaths.cmdlineInfoFilePath(), "/proc/cmdline");
    EXPECT_EQ(systemPaths.findfsCmdPath(), "/sbin/findfs");
    EXPECT_EQ(systemPaths.mountCmdPath(), "/bin/mount");
}
