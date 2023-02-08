// Copyright 2023 Sophos Limited. All rights reserved.

#include "modules/ResponseActions/ResponsePlugin/PluginUtils.h"

#include "tests/Common/Helpers/MemoryAppender.h"


#include <gtest/gtest.h>
#include <gmock/gmock.h>

class PluginUtilsTests : public MemoryAppenderUsingTests
{
public:
    PluginUtilsTests()
        : MemoryAppenderUsingTests("responseactions")
    {}
};

TEST_F(PluginUtilsTests, invalidActionReturnsEnumNone)
{
    UsingMemoryAppender recorder(*this);
    ResponsePlugin::ActionType type = ResponsePlugin::PluginUtils::getType("");
    EXPECT_EQ(ResponsePlugin::ActionType::NONE,type);
    EXPECT_TRUE(appenderContains("Cannot parse action with error"));
}

TEST_F(PluginUtilsTests, ValidUploadFileActionReturnCorrectEnum)
{
    UsingMemoryAppender recorder(*this);
    ResponsePlugin::ActionType type = ResponsePlugin::PluginUtils::getType("{\"type\":\"sophos.mgt.action.UploadFile\"}");
    EXPECT_EQ(ResponsePlugin::ActionType::UPLOAD_FILE,type);
    EXPECT_FALSE(appenderContains("Cannot parse action with error"));
}

TEST_F(PluginUtilsTests, invalidTypeBoolReturnsEnumThrow)
{
    UsingMemoryAppender recorder(*this);
    ResponsePlugin::ActionType type = ResponsePlugin::PluginUtils::getType("{\"type\":false}");
    EXPECT_EQ(ResponsePlugin::ActionType::NONE,type);
    EXPECT_TRUE(appenderContains("Type value: false is not a string"));
}

TEST_F(PluginUtilsTests, invalidTypeIntReturnsEnumThrow)
{
    UsingMemoryAppender recorder(*this);
    ResponsePlugin::ActionType type = ResponsePlugin::PluginUtils::getType("{\"type\":1000}");
    EXPECT_EQ(ResponsePlugin::ActionType::NONE,type);
    EXPECT_TRUE(appenderContains("Type value: 1000 is not a string"));
}