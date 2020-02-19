/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gtest/gtest.h>

#include "avscanner/avscannerimpl/CommandLineScanRunner.h"
#include "datatypes/sophos_filesystem.h"

#include <fstream>

namespace fs = sophos_filesystem;


TEST(CommandLineScanRunner, construction) // NOLINT
{
    std::vector<std::string> paths;
    avscanner::avscannerimpl::CommandLineScanRunner runner(paths);
}

TEST(CommandLineScanRunner, scanRelativeDirectory) // NOLINT
{
    fs::create_directories("sandbox/a/b/d/e");
    std::ofstream("sandbox/a/b/file1.txt");

    fs::remove_all("sandbox");

//    ASSERT_EQ(callbacks.m_paths.size(), 1);
//    EXPECT_EQ(callbacks.m_paths.at(0), "sandbox/a/b/file1.txt");
}