// Copyright 2022, Sophos Limited.  All rights reserved.

#include "FanotifyHandlerMemoryAppenderUsingTests.h"

#include "sophos_on_access_process/fanotifyhandler/ProcUtils.h"

#include <gtest/gtest.h>


class TestProcUtils : public FanotifyHandlerMemoryAppenderUsingTests
{
};

using namespace sophos_on_access_process::fanotifyhandler;

TEST_F(TestProcUtils, startswith_empty)
{
    EXPECT_TRUE(startswith("", ""));
    EXPECT_TRUE(startswith("abc", ""));
}

TEST_F(TestProcUtils, startswith_match)
{
    EXPECT_TRUE(startswith("abc", "abc"));
    EXPECT_TRUE(startswith("abc", "a"));
    EXPECT_TRUE(startswith("a/b/c", "a/b/"));
}

TEST_F(TestProcUtils, startswith_no_match)
{
    EXPECT_FALSE(startswith("abc", "bc"));
    EXPECT_FALSE(startswith("abc", "b"));
    EXPECT_FALSE(startswith("a/b/c", "b/c"));
}

TEST_F(TestProcUtils, get_executable_path_from_pid_init)
{
    if (geteuid() != 0)
    {
        GTEST_SKIP();
    }
    auto initExe = get_executable_path_from_pid(1);
    EXPECT_NE(initExe, "");
}

TEST_F(TestProcUtils, get_executable_path_from_pid_self)
{
    auto exe = get_executable_path_from_pid(getpid());
    EXPECT_NE(exe, "");
}

TEST_F(TestProcUtils, get_executable_path_from_pid_empty_from_bad_pid)
{
    auto exe = get_executable_path_from_pid(0);
    EXPECT_EQ(exe, "");
}
