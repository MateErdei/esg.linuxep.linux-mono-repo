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
        : MemoryAppenderUsingTests("ResponseActionsImpl")
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

    nlohmann::json getDefaultDownloadAction()
    {
        nlohmann::json action;
        action["url"] = "https://s3.com/download.zip";
        action["targetPath"] = "/targetpath";
        action["sha256"] = "shastring";
        action["sizeBytes"] = 1024;
        action["expiration"] = 144444000000004;
        action["timeout"] = 10;
        //optional
        action["decompress"] = false;
        action["password"] = "";
        return action;
    }
};

TEST_F(ActionsUtilsTests, testExpiry)
{
    EXPECT_TRUE(ResponseActionsImpl::ActionsUtils::isExpired(1000));
    // time is set here to Tue 8 Feb 17:12:46 GMT 2033
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

//**********************DOWNLOAD ACTION***************************
TEST_F(ActionsUtilsTests, testMissingUrl)
{
    nlohmann::json action = getDefaultDownloadAction();
    action.erase("url");
    try
    {
        auto output = ResponseActionsImpl::ActionsUtils::readDownloadAction(action.dump());
        FAIL() << "Didnt throw due to missing essential action";
    }
    catch (const ResponseActionsImpl::InvalidCommandFormat& except)
    {
        EXPECT_STREQ(except.what(), "Invalid command format. Download command from Central missing required parameter: url");
    }
}

TEST_F(ActionsUtilsTests, testMissingtargetPath)
{
    nlohmann::json action = getDefaultDownloadAction();
    action.erase("targetPath");
    try
    {
        auto output = ResponseActionsImpl::ActionsUtils::readDownloadAction(action.dump());
        FAIL() << "Didnt throw due to missing essential action";
    }
    catch (const ResponseActionsImpl::InvalidCommandFormat& except)
    {
        EXPECT_STREQ(except.what(), "Invalid command format. Download command from Central missing required parameter: targetPath");
    }
}

TEST_F(ActionsUtilsTests, testMissingsha256)
{
    nlohmann::json action = getDefaultDownloadAction();
    action.erase("sha256");
    try
    {
        auto output = ResponseActionsImpl::ActionsUtils::readDownloadAction(action.dump());
        FAIL() << "Didnt throw due to missing essential action";
    }
    catch (const ResponseActionsImpl::InvalidCommandFormat& except)
    {
        EXPECT_STREQ(except.what(), "Invalid command format. Download command from Central missing required parameter: sha256");
    }
}

TEST_F(ActionsUtilsTests, testMissingtimeout)
{
    nlohmann::json action = getDefaultDownloadAction();
    action.erase("timeout");
    try
    {
        auto output = ResponseActionsImpl::ActionsUtils::readDownloadAction(action.dump());
        FAIL() << "Didnt throw due to missing essential action";
    }
    catch (const ResponseActionsImpl::InvalidCommandFormat& except)
    {
        EXPECT_STREQ(except.what(), "Invalid command format. Download command from Central missing required parameter: timeout");
    }
}

TEST_F(ActionsUtilsTests, testMissingsizeBytes)
{
    nlohmann::json action = getDefaultDownloadAction();
    action.erase("sizeBytes");
    try
    {
        auto output = ResponseActionsImpl::ActionsUtils::readDownloadAction(action.dump());
        FAIL() << "Didnt throw due to missing essential action";
    }
    catch (const ResponseActionsImpl::InvalidCommandFormat& except)
    {
        EXPECT_STREQ(except.what(), "Invalid command format. Download command from Central missing required parameter: sizeBytes");
    }
}

TEST_F(ActionsUtilsTests, testMissingexpiration)
{
    nlohmann::json action = getDefaultDownloadAction();
    action.erase("expiration");
    try
    {
        auto output = ResponseActionsImpl::ActionsUtils::readDownloadAction(action.dump());
        FAIL() << "Didnt throw due to missing essential action";
    }
    catch (const ResponseActionsImpl::InvalidCommandFormat& except)
    {
        EXPECT_STREQ(except.what(), "Invalid command format. Download command from Central missing required parameter: expiration");
    }
}

TEST_F(ActionsUtilsTests, testMissingpassword)
{
    nlohmann::json action = getDefaultDownloadAction();
    action.erase("password");
    EXPECT_NO_THROW(ResponseActionsImpl::ActionsUtils::readDownloadAction(action.dump()));
}

TEST_F(ActionsUtilsTests, testMissingdecompress)
{
    nlohmann::json action = getDefaultDownloadAction();
    action.erase("decompress");
    EXPECT_NO_THROW(ResponseActionsImpl::ActionsUtils::readDownloadAction(action.dump()));
}

TEST_F(ActionsUtilsTests, testSuccessfulParsing)
{
    nlohmann::json action = getDefaultDownloadAction();
    auto output = ResponseActionsImpl::ActionsUtils::readDownloadAction(action.dump());

    EXPECT_EQ(output.decompress, action.at("decompress"));
    EXPECT_EQ(output.sizeBytes, action.at("sizeBytes"));
    EXPECT_EQ(output.expiration, action.at("expiration"));
    EXPECT_EQ(output.timeout, action.at("timeout"));
    EXPECT_EQ(output.sha256, action.at("sha256"));
    EXPECT_EQ(output.url, action.at("url"));
    EXPECT_EQ(output.targetPath, action.at("targetPath"));
    EXPECT_EQ(output.password, action.at("password"));

}
/*

action["url"] = "https://s3.com/download.zip";
action["targetPath"] = "/targetpath";
action["sha256"] = "shastring";
action["sizeBytes"] = 1024;
action["expiration"] = 144444000000004;
action["timeout"] = 10;
//optional
action["decompress"] = false;
action["password"] = "";*/
