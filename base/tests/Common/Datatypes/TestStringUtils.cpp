/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/Datatypes/StringUtils.h>

#include "gtest/gtest.h"

TEST(TestStringUtils, endswith) // NOLINT
{
    EXPECT_TRUE(Common::Datatypes::StringUtils::endswith("FOOBAR","BAR"));
    EXPECT_TRUE(Common::Datatypes::StringUtils::endswith("BAR","BAR"));
    EXPECT_FALSE(Common::Datatypes::StringUtils::endswith("FOOBAS","BAR"));
    EXPECT_FALSE(Common::Datatypes::StringUtils::endswith("","BAR"));
    EXPECT_FALSE(Common::Datatypes::StringUtils::endswith("F","BAR"));
    EXPECT_TRUE(Common::Datatypes::StringUtils::endswith("FOOBAR",""));
    EXPECT_TRUE(Common::Datatypes::StringUtils::endswith("BARFOOBAR","BAR"));
    EXPECT_TRUE(Common::Datatypes::StringUtils::endswith("stuff.json",".json"));
    EXPECT_TRUE(Common::Datatypes::StringUtils::endswith("stuff.json.json",".json"));
    EXPECT_FALSE(Common::Datatypes::StringUtils::endswith("stuff.json.other",".json"));
}
