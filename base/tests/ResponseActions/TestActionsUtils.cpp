// Copyright 2023 Sophos Limited. All rights reserved.

#include "modules/ResponseActions/ResponseActionsImpl/ActionsUtils.h"
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
                throw std::runtime_error("invalid type");
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
    ResponseActionsImpl::ActionsUtils actionsUtils;
    EXPECT_TRUE(actionsUtils.isExpired(1000));
    EXPECT_FALSE(actionsUtils.isExpired(1991495566));
    Common::UtilityImpl::FormattedTime time;
    u_int64_t currentTime = time.currentEpochTimeInSecondsAsInteger();
    EXPECT_TRUE(actionsUtils.isExpired(currentTime-10));
    EXPECT_FALSE(actionsUtils.isExpired(currentTime+10));
}

TEST_F(ActionsUtilsTests, testSucessfulParseUploadFile)
{
    ResponseActionsImpl::ActionsUtils actionsUtils;
    nlohmann::json action = getDefaultUploadObject(ResponseActionsImpl::UploadType::FILE);
    ResponseActionsImpl::UploadInfo info = actionsUtils.readUploadAction(action.dump(),ResponseActionsImpl::UploadType::FILE);
    EXPECT_EQ(info.targetPath,"path");
    EXPECT_EQ(info.maxSize,10000000);
    EXPECT_EQ(info.compress,false);
    EXPECT_EQ(info.password,"");
    EXPECT_EQ(info.expiration,1444444);
    EXPECT_EQ(info.timeout,10);
}

TEST_F(ActionsUtilsTests, testSucessfulParseUploadFolder)
{
    ResponseActionsImpl::ActionsUtils actionsUtils;
    nlohmann::json action = getDefaultUploadObject(ResponseActionsImpl::UploadType::FOLDER);
    ResponseActionsImpl::UploadInfo info = actionsUtils.readUploadAction(action.dump(),ResponseActionsImpl::UploadType::FOLDER);
    EXPECT_EQ(info.targetPath,"path");
    EXPECT_EQ(info.maxSize,10000000);
    EXPECT_EQ(info.compress,false);
    EXPECT_EQ(info.password,"");
    EXPECT_EQ(info.expiration,1444444);
    EXPECT_EQ(info.timeout,10);
}

TEST_F(ActionsUtilsTests, testParseFailsWhenActionISUploadFileWhenExpectingFolder)
{
    ResponseActionsImpl::ActionsUtils actionsUtils;
    nlohmann::json action = getDefaultUploadObject(ResponseActionsImpl::UploadType::FILE);
    EXPECT_THROW(actionsUtils.readUploadAction(action.dump(),ResponseActionsImpl::UploadType::FOLDER),std::runtime_error);
}

TEST_F(ActionsUtilsTests, testSucessfulParseCompressionEnabled)
{
    ResponseActionsImpl::ActionsUtils actionsUtils;
    nlohmann::json action = getDefaultUploadObject(ResponseActionsImpl::UploadType::FILE);
    action["compress"] = true;
    action["password"] = "password";
    ResponseActionsImpl::UploadInfo info = actionsUtils.readUploadAction(action.dump(),ResponseActionsImpl::UploadType::FILE);

    EXPECT_EQ(info.compress,true);
    EXPECT_EQ(info.password,"password");
}

TEST_F(ActionsUtilsTests, testFailedParseInvalidValue)
{
    ResponseActionsImpl::ActionsUtils actionsUtils;
    nlohmann::json action = getDefaultUploadObject(ResponseActionsImpl::UploadType::FILE);
    action["url"] = 1000;
    EXPECT_THROW(actionsUtils.readUploadAction(action.dump(),ResponseActionsImpl::UploadType::FOLDER),std::runtime_error);
}

TEST_F(ActionsUtilsTests, testFailedParseMissingUrl)
{
    ResponseActionsImpl::ActionsUtils actionsUtils;
    nlohmann::json action = getDefaultUploadObject(ResponseActionsImpl::UploadType::FILE);
    action.erase("url");
    EXPECT_THROW(actionsUtils.readUploadAction(action.dump(),ResponseActionsImpl::UploadType::FOLDER),std::runtime_error);
}

TEST_F(ActionsUtilsTests, testFailedParseMissingExpiration)
{
    ResponseActionsImpl::ActionsUtils actionsUtils;
    nlohmann::json action = getDefaultUploadObject(ResponseActionsImpl::UploadType::FILE);
    action.erase("expiration");
    EXPECT_THROW(actionsUtils.readUploadAction(action.dump(),ResponseActionsImpl::UploadType::FOLDER),std::runtime_error);
}

TEST_F(ActionsUtilsTests, testFailedParseMissingTimeout)
{
    ResponseActionsImpl::ActionsUtils actionsUtils;
    nlohmann::json action = getDefaultUploadObject(ResponseActionsImpl::UploadType::FILE);
    action.erase("timeout");
    EXPECT_THROW(actionsUtils.readUploadAction(action.dump(),ResponseActionsImpl::UploadType::FOLDER),std::runtime_error);
}

TEST_F(ActionsUtilsTests, testFailedParseMissingMaxUploadSizeBytes)
{
    ResponseActionsImpl::ActionsUtils actionsUtils;
    nlohmann::json action = getDefaultUploadObject(ResponseActionsImpl::UploadType::FILE);
    action.erase("maxUploadSizeBytes");
    EXPECT_THROW(actionsUtils.readUploadAction(action.dump(),ResponseActionsImpl::UploadType::FOLDER),std::runtime_error);
}

TEST_F(ActionsUtilsTests, testFailedParseMissingTargetFile)
{
    ResponseActionsImpl::ActionsUtils actionsUtils;
    nlohmann::json action = getDefaultUploadObject(ResponseActionsImpl::UploadType::FILE);
    action.erase("targetFile");
    EXPECT_THROW(actionsUtils.readUploadAction(action.dump(),ResponseActionsImpl::UploadType::FOLDER),std::runtime_error);
}
