/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/UtilityImpl/StringUtils.h>

#include "gtest/gtest.h"

using namespace Common::UtilityImpl;

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

TEST(TestStringUtils, startswith) //NOLINT
{
    EXPECT_TRUE(StringUtils::startswith("FOOBAR","FOO"));
    EXPECT_TRUE(StringUtils::startswith("FOOBAR",""));
    EXPECT_TRUE(StringUtils::startswith("FOOBAR","F"));
    EXPECT_TRUE(StringUtils::startswith("",""));
    EXPECT_FALSE(StringUtils::startswith("FOOBAR","BAR"));
}

TEST(TestStringUtils, replace) //NOLINT
{
    EXPECT_EQ(StringUtils::replaceAll("FOO","FOO","BAR"),"BAR");
    EXPECT_EQ(StringUtils::replaceAll("FOO","",""),"FOO");
    EXPECT_EQ(StringUtils::replaceAll("FOOBARFOO","BAR","FOO"),"FOOFOOFOO");
    EXPECT_EQ(StringUtils::replaceAll("FOOBARFOOBARBAS","BAR","FOO"),"FOOFOOFOOFOOBAS");

    EXPECT_EQ(
            StringUtils::replaceAll("ABCDEFGH","D",""),
            "ABCEFGH"
            );
}
