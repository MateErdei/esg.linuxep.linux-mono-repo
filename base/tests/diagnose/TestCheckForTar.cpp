/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <modules/Diagnose/diagnose/CheckForTar.h>

#include <gtest/gtest.h>

TEST(TestCheckForTar, realPath) // NOLINT
{
    // Assume tar is actually available everywhere we run unittests
    ASSERT_TRUE(diagnose::CheckForTar::isTarAvailable());
}

TEST(TestCheckForTar, emptyPath) // NOLINT
{
    ASSERT_FALSE(diagnose::CheckForTar::isTarAvailable(""));
}

TEST(TestCheckForTar, nonExistentPath) // NOLINT
{
    ASSERT_FALSE(diagnose::CheckForTar::isTarAvailable("/t"));
}

TEST(TestCheckForTar, manyNonExistentPaths) // NOLINT
{
    ASSERT_FALSE(diagnose::CheckForTar::isTarAvailable("/t:/a:/b:/c:/d:/e:/f"));
}

TEST(TestCheckForTar, manyNonExistentPathsFollowedByBin) // NOLINT
{
    ASSERT_TRUE(diagnose::CheckForTar::isTarAvailable("/t:/a:/b:/c:/d:/e:/f:/bin:/usr/bin"));
}

TEST(TestCheckForTar, simpleBin) // NOLINT
{
    ASSERT_TRUE(diagnose::CheckForTar::isTarAvailable("/bin"));
}
