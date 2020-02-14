/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gtest/gtest.h>

#include "avscanner/avscannerimpl/Mounts.h"

using namespace avscanner::avscannerimpl;

TEST(Mounts, TestMountPoints) // NOLINT
{
    // Constructing the "Mounts" object here will parse /proc/mounts for the build machine
    std::shared_ptr<IMountInfo> mountInfo = std::make_shared<Mounts>();
    std::vector<std::shared_ptr<IMountPoint>> allMountpoints = mountInfo->mountPoints();
    EXPECT_GT(allMountpoints.size(), 0);
}