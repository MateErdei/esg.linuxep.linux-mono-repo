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
    std::pair<std::string, int> actionInfo = ResponsePlugin::PluginUtils::getType("");
    EXPECT_EQ("", actionInfo.first);
    EXPECT_TRUE(appenderContains("Cannot parse action with error"));
}

TEST_F(RAPluginUtilsTests, ValidUploadFileActionReturnCorrectValues)
{
    UsingMemoryAppender recorder(*this);
    std::pair<std::string, int> actionInfo =
        ResponsePlugin::PluginUtils::getType(R"({"type":"sophos.mgt.action.UploadFile","timeout":1000})");
    EXPECT_EQ("sophos.mgt.action.UploadFile", actionInfo.first);
    EXPECT_EQ(1000, actionInfo.second);
    EXPECT_FALSE(appenderContains("Cannot parse action with error"));
}

TEST_F(RAPluginUtilsTests, invalidTypeBoolReturnsEnumThrow)
{
    UsingMemoryAppender recorder(*this);
    std::pair<std::string, int> actionInfo = ResponsePlugin::PluginUtils::getType(R"({"type":false,"timeout":1000})");
    EXPECT_EQ("", actionInfo.first);
    EXPECT_TRUE(appenderContains("Type value: false is not a string"));
}

TEST_F(RAPluginUtilsTests, invalidTypeIntReturnsEnumThrow)
{
    UsingMemoryAppender recorder(*this);
    std::pair<std::string, int> actionInfo = ResponsePlugin::PluginUtils::getType(R"({"type":1000,"timeout":1000})");
    EXPECT_EQ("", actionInfo.first);
    EXPECT_TRUE(appenderContains("Type value: 1000 is not a string"));
}