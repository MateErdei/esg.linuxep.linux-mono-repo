// Copyright 2024 Sophos Limited. All rights reserved.

#include "sophos_install/SophosInstall.h"
#include "datatypes/Print.h"

#include <gtest/gtest.h>

TEST(TestSophosInstall, basic)
{
    char installPath[PATH_MAX];
    getSophosInstall(installPath, PATH_MAX);

    // Can't test much more than this
    EXPECT_NE(installPath[0], 0);
    PRINT(installPath);
}

TEST(TestSophosInstall, tooSmall)
{
    char installPath[3];
    auto result = getSophosInstall(installPath, 3);
    EXPECT_EQ(result, -1);
    EXPECT_EQ(errno, ENAMETOOLONG);
}