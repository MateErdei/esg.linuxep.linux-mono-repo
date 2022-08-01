// Copyright 2022, Sophos Limited.  All rights reserved.

#include "common/SaferStrerror.h"

#include <gtest/gtest.h>

using namespace common;

TEST(SaferStrerror, standard)
{
    EXPECT_EQ(safer_strerror(ENOENT), "No such file or directory");
}

TEST(SaferStrerror, nonstandard)
{
    EXPECT_EQ(safer_strerror(513), "Unknown error 513");
}
