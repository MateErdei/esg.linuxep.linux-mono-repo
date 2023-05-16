// Copyright 2023 Sophos Limited. All rights reserved.

#include "modules/ResponseActions/ResponsePlugin/PluginUtils.h"

#include "tests/Common/Helpers/MemoryAppender.h"


#include <gtest/gtest.h>
#include <gmock/gmock.h>

class RAPluginUtilsTests : public MemoryAppenderUsingTests
{
public:
    RAPluginUtilsTests()
        : MemoryAppenderUsingTests("responseactions")
    {}
};

TEST_F(RAPluginUtilsTests, invalidActionReturnsEnumNone)
{
    UsingMemoryAppender recorder(*this);
    const auto [type, timeout] = ResponsePlugin::PluginUtils::getType("");
    EXPECT_EQ("", type);
    EXPECT_TRUE(appenderContains("Cannot parse action with error"));
}

TEST_F(RAPluginUtilsTests, ValidUploadFileActionReturnCorrectValues)
{
    UsingMemoryAppender recorder(*this);
    const auto [type, timeout] = ResponsePlugin::PluginUtils::getType(R"({"type":"sophos.mgt.action.UploadFile","timeout":1000})");
    EXPECT_EQ("sophos.mgt.action.UploadFile", type);
    EXPECT_EQ(1000, timeout);
    EXPECT_FALSE(appenderContains("Cannot parse action with error"));
}

TEST_F(RAPluginUtilsTests, EmptyTypeFieldResponseAction)
{
    UsingMemoryAppender recorder(*this);
    const auto [type, timeout] = ResponsePlugin::PluginUtils::getType(R"({"type":"","timeout":1000})");
    EXPECT_EQ("", type);
    EXPECT_EQ(1000, timeout);
    EXPECT_FALSE(appenderContains("Cannot parse action with error"));
}

TEST_F(RAPluginUtilsTests, LongTypeFieldResponseAction)
{
    UsingMemoryAppender recorder(*this);
    std::string longString(500, 'a');
    const auto [type, timeout] = ResponsePlugin::PluginUtils::getType(R"({"type":")" + longString + R"(","timeout":1000})");
    EXPECT_EQ(longString, type);
    EXPECT_EQ(1000, timeout);
    EXPECT_FALSE(appenderContains("Cannot parse action with error"));
}

TEST_F(RAPluginUtilsTests, invalidTypeBoolReturnsEnumThrow)
{
    UsingMemoryAppender recorder(*this);
    const auto [type, timeout] = ResponsePlugin::PluginUtils::getType(R"({"type":false,"timeout":1000})");
    EXPECT_EQ("", type);
    EXPECT_EQ(-1, timeout);
    EXPECT_TRUE(appenderContains("Type value: false is not a string"));
}

TEST_F(RAPluginUtilsTests, invalidTypeIntReturnsEnumThrow)
{
    UsingMemoryAppender recorder(*this);
    const auto [type, timeout] = ResponsePlugin::PluginUtils::getType(R"({"type":1000,"timeout":1000})");
    EXPECT_EQ("", type);
    EXPECT_EQ(-1, timeout);
    EXPECT_TRUE(appenderContains("Type value: 1000 is not a string"));
}

TEST_F(RAPluginUtilsTests, MissingTypeFieldResponseAction)
{
    UsingMemoryAppender recorder(*this);
    const auto [type, timeout] = ResponsePlugin::PluginUtils::getType(R"({"timeout":1000})");
    EXPECT_EQ("", type);
    EXPECT_EQ(-1, timeout);
    EXPECT_FALSE(appenderContains("Cannot parse action with error"));
}

TEST_F(RAPluginUtilsTests, MissingTimeoutFieldResponseAction)
{
    UsingMemoryAppender recorder(*this);
    const auto [type, timeout] = ResponsePlugin::PluginUtils::getType(R"({"type":1000})");
    EXPECT_EQ("", type);
    EXPECT_EQ(-1, timeout);
    EXPECT_FALSE(appenderContains("Cannot parse action with error"));
}
