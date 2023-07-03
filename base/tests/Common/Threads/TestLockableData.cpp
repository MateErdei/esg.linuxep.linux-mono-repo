// Copyright 2023 Sophos All rights reserved.

#include "modules/Common/Threads/LockableData.h"

#include <gtest/gtest.h>

namespace
{

    class TestBoolean
    {
    public:
        TestBoolean() = default;
        explicit TestBoolean(bool initialValue)
            : boolean_(initialValue)
        {
        }
        Common::Threads::LockableData<bool> boolean_;
    };
}

TEST(TestLockableData, boolean)
{
    TestBoolean test;
    {
        auto locked = test.boolean_.lock();
        bool& value = *locked;
        value = true;
    }

    {
        auto locked = test.boolean_.lock();
        bool& value = *locked;
        EXPECT_TRUE(value);
        EXPECT_TRUE(*locked);
    }
}

TEST(TestLockableData, boolean_locked)
{
    TestBoolean test(true);
    {
        const auto locked = test.boolean_.lock();
        const bool& value = *locked;
        EXPECT_TRUE(value);
        EXPECT_TRUE(*locked);
    }
}

TEST(TestLockableData, boolean_construct_true)
{
    TestBoolean test(true);
    {
        auto locked = test.boolean_.lock();
        bool& value = *locked;
        EXPECT_TRUE(value);
        EXPECT_TRUE(*locked);
    }
}

TEST(TestLockableData, boolean_construct_false)
{
    TestBoolean test(false);
    {
        auto locked = test.boolean_.lock();
        bool& value = *locked;
        EXPECT_FALSE(value);
        EXPECT_FALSE(*locked);
    }
}



