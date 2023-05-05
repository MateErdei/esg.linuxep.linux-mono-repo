// Copyright 2023 Sophos Limited. All rights reserved.

#include "avscanner/avscannerimpl/PuaExclusions.h"

#include "ScanRunnerMemoryAppenderUsingTests.h"

#include <gtest/gtest.h>

using namespace avscanner::avscannerimpl;

namespace
{
    class PuaExclusions : public ScanRunnerMemoryAppenderUsingTests
    {
    };
}

TEST_F(PuaExclusions, allow_empty)
{
    pua_exclusion_t exclusions;
    EXPECT_FALSE(badPuaExclusion(exclusions));
}

TEST_F(PuaExclusions, allow_lowercase)
{
    pua_exclusion_t exclusions{"abc"};
    EXPECT_FALSE(badPuaExclusion(exclusions));
}

TEST_F(PuaExclusions, allow_uppercase)
{
    pua_exclusion_t exclusions{"ABC"};
    EXPECT_FALSE(badPuaExclusion(exclusions));
}

TEST_F(PuaExclusions, allow_digit)
{
    pua_exclusion_t exclusions{"123"};
    EXPECT_FALSE(badPuaExclusion(exclusions));
}

TEST_F(PuaExclusions, allow_slash)
{
    pua_exclusion_t exclusions{"/"};
    EXPECT_FALSE(badPuaExclusion(exclusions));
}

TEST_F(PuaExclusions, allow_multiple)
{
    pua_exclusion_t exclusions{"abc", "123"};
    EXPECT_FALSE(badPuaExclusion(exclusions));
}

TEST_F(PuaExclusions, deny_space)
{
    pua_exclusion_t exclusions{" "};
    EXPECT_TRUE(badPuaExclusion(exclusions));
}

TEST_F(PuaExclusions, deny_dollar)
{
    pua_exclusion_t exclusions{"$"};
    EXPECT_TRUE(badPuaExclusion(exclusions));
}

TEST_F(PuaExclusions, deny_percent)
{
    pua_exclusion_t exclusions{"%"};
    EXPECT_TRUE(badPuaExclusion(exclusions));
}

TEST_F(PuaExclusions, bad_overrides_good)
{
    pua_exclusion_t exclusions{"abc", "$"};
    EXPECT_TRUE(badPuaExclusion(exclusions));
}
