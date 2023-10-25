// Copyright 2023 Sophos All rights reserved.

#include "Common/Threads/LockableBool.h"

#include <gtest/gtest.h>

TEST(TestLockableBool, create)
{
    Common::Threads::LockableBool v{true};
}

TEST(TestLockableBool, access)
{
    Common::Threads::LockableBool v{true};
    EXPECT_TRUE(v.load());
}

TEST(TestLockableBool, setTrue)
{
    Common::Threads::LockableBool v{false};
    v.store(true);
    EXPECT_TRUE(v.load());
}

TEST(TestLockableBool, setFalse)
{
    Common::Threads::LockableBool v{true};
    v.store(false);
    EXPECT_FALSE(v.load());
}
