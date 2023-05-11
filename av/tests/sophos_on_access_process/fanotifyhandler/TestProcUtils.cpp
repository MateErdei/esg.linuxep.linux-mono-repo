// Copyright 2022-2023 Sophos Limited. All rights reserved.

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
