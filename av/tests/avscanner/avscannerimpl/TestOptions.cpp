/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gtest/gtest.h>

#include "avscanner/avscannerimpl/Options.h"

using namespace avscanner::avscannerimpl;

TEST(Options, TestNoArgs) // NOLINT
{
    int argc = 0;
    char* argv[0];
    Options o(argc, argv);
    EXPECT_EQ(o.paths().size(), 0);
    EXPECT_EQ(o.config(), "");
}

TEST(Options, TestPaths) // NOLINT
{
    int argc = 2;
    const char* argv[2];
    argv[0] = "/usr/bin/avscanner";
    argv[1] = "/foo";
    Options o(argc, const_cast<char**>(argv));
    auto paths = o.paths();
    ASSERT_EQ(paths.size(), 1);
    EXPECT_EQ(paths.at(0), "/foo");
    EXPECT_EQ(o.config(), "");
}

TEST(Options, TestMulitplePaths) // NOLINT
{
    const int argc = 3;
    const char* argv[argc];
    argv[0] = "/usr/bin/avscanner";
    argv[1] = "/foo";
    argv[2] = "/bar";
    Options o(argc, const_cast<char**>(argv));
    auto paths = o.paths();
    ASSERT_EQ(paths.size(), 2);
    EXPECT_EQ(paths.at(0), "/foo");
    EXPECT_EQ(paths.at(1), "/bar");
    EXPECT_EQ(o.config(), "");
}


TEST(Options, TestConfig) // NOLINT
{
    const int argc = 3;
    const char* argv[argc];
    argv[0] = "/usr/bin/avscanner";
    argv[1] = "--config";
    argv[2] = "/bar";
    Options o(argc, const_cast<char**>(argv));
    auto paths = o.paths();
    ASSERT_EQ(paths.size(), 0);
    EXPECT_EQ(o.config(), "/bar");
}

