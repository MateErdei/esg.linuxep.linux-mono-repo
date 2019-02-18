/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Installer/ManifestDiff/CommandLineOptions.h>
#include <gtest/gtest.h>

using namespace Installer::ManifestDiff;

TEST(TestCommandLineOptions, TestOptionsGoCorrectly) // NOLINT
{
    Common::Datatypes::StringVector args{ "Exe",           "--old=FOO",        "--new=BAR", "--changed=CHANGED",
                                          "--added=ADDED", "--removed=REMOVED" };
    CommandLineOptions options(args);
    EXPECT_EQ(options.m_changed, "CHANGED");
    EXPECT_EQ(options.m_added, "ADDED");
    EXPECT_EQ(options.m_removed, "REMOVED");
    EXPECT_EQ(options.m_new, "BAR");
    EXPECT_EQ(options.m_old, "FOO");
}

TEST(TestCommandLineOptions, TestMissingOptions) // NOLINT
{
    Common::Datatypes::StringVector args{ "Exe" };
    CommandLineOptions options(args);
    EXPECT_EQ(options.m_changed, "");
    EXPECT_EQ(options.m_added, "");
    EXPECT_EQ(options.m_removed, "");
    EXPECT_EQ(options.m_new, "");
    EXPECT_EQ(options.m_old, "");
}
