// Copyright 2023 Sophos Limited. All rights reserved.

#include "modules/ResponseActions/ResponseActionsImpl/ActionsUtils.h"
#include "modules/ResponseActions/ResponseActionsImpl/InvalidCommandFormat.h"
#include "modules/Common/UtilityImpl/TimeUtils.h"

#include "tests/Common/Helpers/MemoryAppender.h"

#include <json.hpp>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

class ActionsUtilsTests : public MemoryAppenderUsingTests
{
public:
    ActionsUtilsTests()
        : MemoryAppenderUsingTests("responseactions")
    {}
    nlohmann::json getDefaultUploadObject(ResponseActionsImpl::UploadType type)
    {
        nlohmann::json action;

        std::string targetKey;
        switch (type)
        {
            case ResponseActionsImpl::UploadType::FILE:
                targetKey = "targetFile";
                break;
            case ResponseActionsImpl::UploadType::FOLDER:
                targetKey = "targetFolder";
                break;
            default:
                throw ResponseActionsImpl::InvalidCommandFormat("invalid type");
        }
        action[targetKey] = "path";
        action["url"] = "https://s3.com/somewhere";
        action["compress"] = false;
        action["password"] = "";
        action["expiration"] = 1444444;
        action["timeout"] = 10;
        action["maxUploadSizeBytes"] = 10000000;
        return action;
    }
};

TEST_F(ActionsUtilsTests, testExpiry)
{
    EXPECT_TRUE(ResponseActionsImpl::ActionsUtils::isExpired(1000));
    EXPECT_FALSE(ResponseActionsImpl::ActionsUtils::isExpired(1991495566));
    Common::UtilityImpl::FormattedTime time;
    u_int64_t currentTime = time.currentEpochTimeInSecondsAsInteger();
    EXPECT_TRUE(ResponseActionsImpl::ActionsUtils::isExpired(currentTime-10));
    EXPECT_FALSE(ResponseActionsImpl::ActionsUtils::isExpired(currentTime+10));
}

TEST_F(ActionsUtilsTests, testSucessfulParseUploadFile)
{
    nlohmann::json action = getDefaultUploadObject(ResponseActionsImpl::UploadType::FILE);
    ResponseActionsImpl::UploadInfo info = ResponseActionsImpl::ActionsUtils::readUploadAction(action.dump(),ResponseActionsImpl::UploadType::FILE);
    EXPECT_EQ(info.targetPath,"path");
    EXPECT_EQ(info.maxSize,10000000);
    EXPECT_EQ(info.compress,false);
    EXPECT_EQ(info.password,"");
    EXPECT_EQ(info.expiration,1444444);
    EXPECT_EQ(info.timeout,10);
}

TEST_F(ActionsUtilsTests, testSucessfulParseUploadFolder)
{
    nlohmann::json action = getDefaultUploadObject(ResponseActionsImpl::UploadType::FOLDER);
    ResponseActionsImpl::UploadInfo info = ResponseActionsImpl::ActionsUtils::readUploadAction(action.dump(),ResponseActionsImpl::UploadType::FOLDER);
    EXPECT_EQ(info.targetPath,"path");
    EXPECT_EQ(info.maxSize,10000000);
    EXPECT_EQ(info.compress,false);
    EXPECT_EQ(info.password,"");
    EXPECT_EQ(info.expiration,1444444);
    EXPECT_EQ(info.timeout,10);
}

TEST_F(ActionsUtilsTests, testParseFailsWhenActionIsInvalidJson)
{
    nlohmann::json action = getDefaultUploadObject(ResponseActionsImpl::UploadType::FILE);
    EXPECT_THROW(ResponseActionsImpl::ActionsUtils::readUploadAction("",ResponseActionsImpl::UploadType::FILE),ResponseActionsImpl::InvalidCommandFormat);
}

TEST_F(ActionsUtilsTests, testParseFailsWhenActionISUploadFileWhenExpectingFolder)
{
    nlohmann::json action = getDefaultUploadObject(ResponseActionsImpl::UploadType::FILE);
    EXPECT_THROW(ResponseActionsImpl::ActionsUtils::readUploadAction(action.dump(),ResponseActionsImpl::UploadType::FOLDER),ResponseActionsImpl::InvalidCommandFormat);
}

TEST_F(ActionsUtilsTests, testSucessfulParseCompressionEnabled)
{
    nlohmann::json action = getDefaultUploadObject(ResponseActionsImpl::UploadType::FILE);
    action["compress"] = true;
    action["password"] = "password";
    ResponseActionsImpl::UploadInfo info = ResponseActionsImpl::ActionsUtils::readUploadAction(action.dump(),ResponseActionsImpl::UploadType::FILE);

    EXPECT_EQ(info.compress,true);
    EXPECT_EQ(info.password,"password");
}

TEST_F(ActionsUtilsTests, testFailedParseInvalidValue)
{
    nlohmann::json action = getDefaultUploadObject(ResponseActionsImpl::UploadType::FILE);
    action["url"] = 1000;
    EXPECT_THROW(ResponseActionsImpl::ActionsUtils::readUploadAction(action.dump(),ResponseActionsImpl::UploadType::FILE),ResponseActionsImpl::InvalidCommandFormat);
}

TEST_F(ActionsUtilsTests, testFailedParseMissingUrl)
{
    nlohmann::json action = getDefaultUploadObject(ResponseActionsImpl::UploadType::FILE);
    action.erase("url");
    EXPECT_THROW(ResponseActionsImpl::ActionsUtils::readUploadAction(action.dump(),ResponseActionsImpl::UploadType::FILE),ResponseActionsImpl::InvalidCommandFormat);
}

TEST_F(ActionsUtilsTests, testFailedParseMissingExpiration)
{
    nlohmann::json action = getDefaultUploadObject(ResponseActionsImpl::UploadType::FILE);
    action.erase("expiration");
    EXPECT_THROW(ResponseActionsImpl::ActionsUtils::readUploadAction(action.dump(),ResponseActionsImpl::UploadType::FILE),ResponseActionsImpl::InvalidCommandFormat);
}

TEST_F(ActionsUtilsTests, testFailedParseMissingTimeout)
{
    nlohmann::json action = getDefaultUploadObject(ResponseActionsImpl::UploadType::FILE);
    action.erase("timeout");
    EXPECT_THROW(ResponseActionsImpl::ActionsUtils::readUploadAction(action.dump(),ResponseActionsImpl::UploadType::FILE),ResponseActionsImpl::InvalidCommandFormat);
}

TEST_F(ActionsUtilsTests, testFailedParseMissingMaxUploadSizeBytes)
{
    nlohmann::json action = getDefaultUploadObject(ResponseActionsImpl::UploadType::FILE);
    action.erase("maxUploadSizeBytes");
    EXPECT_THROW(ResponseActionsImpl::ActionsUtils::readUploadAction(action.dump(),ResponseActionsImpl::UploadType::FILE),ResponseActionsImpl::InvalidCommandFormat);
}

TEST_F(ActionsUtilsTests, testFailedParseMissingTargetFile)
{
    nlohmann::json action = getDefaultUploadObject(ResponseActionsImpl::UploadType::FILE);
    action.erase("targetFile");
    EXPECT_THROW(ResponseActionsImpl::ActionsUtils::readUploadAction(action.dump(),ResponseActionsImpl::UploadType::FILE),ResponseActionsImpl::InvalidCommandFormat);
}
