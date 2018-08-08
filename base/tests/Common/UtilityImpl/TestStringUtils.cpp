/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/UtilityImpl/StringUtils.h>

#include "gtest/gtest.h"

using namespace Common::Datatypes;

TEST(TestStringUtils, endswith) // NOLINT
{
    EXPECT_TRUE(StringUtils::endswith("FOOBAR","BAR"));
    EXPECT_TRUE(StringUtils::endswith("BAR","BAR"));
    EXPECT_FALSE(StringUtils::endswith("FOOBAS","BAR"));
    EXPECT_FALSE(StringUtils::endswith("","BAR"));
    EXPECT_FALSE(StringUtils::endswith("F","BAR"));
    EXPECT_TRUE(StringUtils::endswith("FOOBAR",""));
    EXPECT_TRUE(StringUtils::endswith("BARFOOBAR","BAR"));
    EXPECT_TRUE(StringUtils::endswith("stuff.json",".json"));
    EXPECT_TRUE(StringUtils::endswith("stuff.json.json",".json"));
    EXPECT_FALSE(StringUtils::endswith("stuff.json.other",".json"));
}
