/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "sync_versioned_files.h"

#include <gtest/gtest.h>

using namespace sync_versioned_files;

TEST(TestSyncVersionedFiles, testStartsWith_true) // NOLINT
{
    fs::path a("/a/b/c/d");
    fs::path s("/a/b/c");
    EXPECT_TRUE(startswith(a, s));
}


TEST(TestSyncVersionedFiles, testStartsWith_false) // NOLINT
{
    fs::path a("/a/b/c/d");
    fs::path s("/a/b/f");
    EXPECT_FALSE(startswith(a, s));
}
