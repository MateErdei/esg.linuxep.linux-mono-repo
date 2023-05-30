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

TEST_F(RAPluginUtilsTests, EmptyActionReturns_EmptyTypeAndInvalidTimeout)
{
    UsingMemoryAppender recorder(*this);
    auto [ actiontype, timeout ] = ResponsePlugin::PluginUtils::getType("");
    EXPECT_EQ("", actiontype);
    EXPECT_EQ(-1, timeout);
    EXPECT_TRUE(appenderContains("Cannot parse action with error"));
}

TEST_F(RAPluginUtilsTests, ValidActionTypeAndTimeoutReturns_CorrectValues)
{
    UsingMemoryAppender recorder(*this);
    auto [ actiontype, timeout ]  =
        ResponsePlugin::PluginUtils::getType(R"({"type":"sophos.mgt.action.UploadFile","timeout":1000})");
    EXPECT_EQ("sophos.mgt.action.UploadFile", actiontype);
    EXPECT_EQ(1000, timeout);
    EXPECT_FALSE(appenderContains("Cannot parse action with error"));
}

TEST_F(RAPluginUtilsTests, ValidActionTypeAndMissingTimeoutReturns_InvalidTypeAndTimeout)
{
    UsingMemoryAppender recorder(*this);
    auto [ actiontype, timeout ]  =
        ResponsePlugin::PluginUtils::getType(R"({"type":"sophos.mgt.action.UploadFile")");
    EXPECT_EQ("", actiontype);
    EXPECT_EQ(-1, timeout);
    EXPECT_FALSE(appenderContains("Missing either type or timeout field"));
}

TEST_F(RAPluginUtilsTests, MissingActionTypeAndValidTimeoutReturns_InvalidTypeAndTimeout)
{
    UsingMemoryAppender recorder(*this);
    auto [ actiontype, timeout ]  =
        ResponsePlugin::PluginUtils::getType(R"({"timeout":1000")");
    EXPECT_EQ("", actiontype);
    EXPECT_EQ(-1, timeout);
    EXPECT_FALSE(appenderContains("Missing either type or timeout field"));
}

TEST_F(RAPluginUtilsTests, ActionTypeBoolReturns_InvalidTypeAndTimeout)
{
    UsingMemoryAppender recorder(*this);
    auto [ actiontype1, timeout1 ]  = ResponsePlugin::PluginUtils::getType(R"({"type":false,"timeout":1000})");
    EXPECT_EQ("", actiontype1);
    EXPECT_EQ(-1, timeout1);
    EXPECT_TRUE(appenderContains("Action Type: false is not a string"));

    auto [ actiontype2, timeout2 ]  = ResponsePlugin::PluginUtils::getType(R"({"type":true,"timeout":1000})");
    EXPECT_EQ("", actiontype2);
    EXPECT_EQ(-1, timeout2);
    EXPECT_TRUE(appenderContains("Action Type: true is not a string"));
}

TEST_F(RAPluginUtilsTests, ActionTypeIntReturns_InvalidTypeAndTimeout)
{
    UsingMemoryAppender recorder(*this);
    auto [ actiontype, timeout ]  = ResponsePlugin::PluginUtils::getType(R"({"type":1000,"timeout":1000})");
    EXPECT_EQ("", actiontype);
    EXPECT_EQ(-1, timeout);
    EXPECT_TRUE(appenderContains("Action Type: 1000 is not a string"));
}

TEST_F(RAPluginUtilsTests, LargeTimeoutReturns_InvalidTypeAndTimeout)
{
    UsingMemoryAppender recorder(*this);

    auto [ actiontype, timeout ] = ResponsePlugin::PluginUtils::getType(R"({"type":"sophos.mgt.action.UploadFile","timeout":1234567891011121314151617})");
    EXPECT_EQ("", actiontype);
    EXPECT_EQ(-1, timeout);
    EXPECT_TRUE(appenderContains("Timeout value: 1.2345678910111213e+24 is larger than maximum allowed value: 2147483647"));
}

TEST_F(RAPluginUtilsTests, LargeNegativeTimeoutReturns_InvalidTypeAndTimeout)
{
    UsingMemoryAppender recorder(*this);

    auto [ actiontype, timeout ] = ResponsePlugin::PluginUtils::getType(R"({"type":"sophos.mgt.action.UploadFile","timeout":-1234567891011121314151617})");
    EXPECT_EQ("", actiontype);
    EXPECT_EQ(-1, timeout);
    EXPECT_TRUE(appenderContains("Timeout value: -1.2345678910111213e+24 is negative and not allowed"));
}

TEST_F(RAPluginUtilsTests, NegativeTimeoutReturns_InvalidTypeAndTimeout)
{
    UsingMemoryAppender recorder(*this);

    auto [ actiontype, timeout ] = ResponsePlugin::PluginUtils::getType(R"({"type":"sophos.mgt.action.UploadFile","timeout":-5})");
    EXPECT_EQ("", actiontype);
    EXPECT_EQ(-1, timeout);
    EXPECT_TRUE(appenderContains("Timeout value: -5 is negative and not allowed"));
}

TEST_F(RAPluginUtilsTests, TimeoutFloatReturns_ValidTypeAndTruncatedTimeout)
{
    UsingMemoryAppender recorder(*this);
    auto [ actiontype, timeout ] = ResponsePlugin::PluginUtils::getType(R"({"type":"sophos.mgt.action.UploadFile","timeout":100.22})");
    EXPECT_EQ("sophos.mgt.action.UploadFile", actiontype);
    EXPECT_EQ(100, timeout);
    EXPECT_TRUE(appenderContains("Timeout value: 100.22 is type float, truncated to 100"));
}

TEST_F(RAPluginUtilsTests, TimeoutBoolReturns_InValidTypeAndTimeout)
{
    UsingMemoryAppender recorder(*this);
    auto [ actiontype1, timeout1 ]  = ResponsePlugin::PluginUtils::getType(R"({"type":"sophos.mgt.action.UploadFile","timeout":true})");
    EXPECT_EQ("", actiontype1);
    EXPECT_EQ(-1, timeout1);
    EXPECT_TRUE(appenderContains("Timeout value: true, is not a number"));

    auto [ actiontype2, timeout2 ]  = ResponsePlugin::PluginUtils::getType(R"({"type":"sophos.mgt.action.UploadFile","timeout":false})");
    EXPECT_EQ("", actiontype2);
    EXPECT_EQ(-1, timeout2);
    EXPECT_TRUE(appenderContains("Timeout value: false, is not a number"));
}

TEST_F(RAPluginUtilsTests, TimeoutStringReturns_InvalidTypeAndTimeout)
{
    UsingMemoryAppender recorder(*this);
    auto [ actiontype, timeout ] = ResponsePlugin::PluginUtils::getType(R"({"type":"sophos.mgt.action.UploadFile","timeout":"1000"})");
    EXPECT_EQ("", actiontype);
    EXPECT_EQ(-1, timeout);
    EXPECT_TRUE(appenderContains(R"(Timeout value: "1000", is not a number)"));
}