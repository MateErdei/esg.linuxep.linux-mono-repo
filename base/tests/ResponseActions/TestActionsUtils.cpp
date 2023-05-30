// Copyright 2023 Sophos Limited. All rights reserved.

#include "modules/Common/UtilityImpl/TimeUtils.h"
#include "modules/ResponseActions/ResponseActionsImpl/ActionsUtils.h"
#include "modules/ResponseActions/ResponseActionsImpl/InvalidCommandFormat.h"
#include "tests/Common/Helpers/FileSystemReplaceAndRestore.h"
#include "tests/Common/Helpers/MemoryAppender.h"
#include "tests/Common/Helpers/MockFilePermissions.h"
#include "tests/Common/Helpers/MockFileSystem.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <json.hpp>

class ActionsUtilsTests : public MemoryAppenderUsingTests
{
public:
    ActionsUtilsTests()
        : MemoryAppenderUsingTests("ResponseActionsImpl")
    {}
    nlohmann::json getDefaultUploadObject(ResponseActionsImpl::ActionType type)
    {
        nlohmann::json action;

        std::string targetKey;
        switch (type)
        {
            case ResponseActionsImpl::ActionType::UPLOADFILE:
                targetKey = "targetFile";
                break;
            case ResponseActionsImpl::ActionType::UPLOADFOLDER:
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
    nlohmann::json action = getDefaultUploadObject(ResponseActionsImpl::ActionType::UPLOADFILE);
    ResponseActionsImpl::UploadInfo info = ResponseActionsImpl::ActionsUtils::readUploadAction(action.dump(),ResponseActionsImpl::ActionType::UPLOADFILE);
    EXPECT_EQ(info.targetPath,"path");
    EXPECT_EQ(info.maxSize,10000000);
    EXPECT_EQ(info.compress,false);
    EXPECT_EQ(info.password,"");
    EXPECT_EQ(info.expiration,1444444);
    EXPECT_EQ(info.timeout,10);
}

TEST_F(ActionsUtilsTests, testSucessfulParseUploadFolder)
{
    nlohmann::json action = getDefaultUploadObject(ResponseActionsImpl::ActionType::UPLOADFOLDER);
    ResponseActionsImpl::UploadInfo info = ResponseActionsImpl::ActionsUtils::readUploadAction(action.dump(),ResponseActionsImpl::ActionType::UPLOADFOLDER);
    EXPECT_EQ(info.targetPath,"path");
    EXPECT_EQ(info.maxSize,10000000);
    EXPECT_EQ(info.compress,false);
    EXPECT_EQ(info.password,"");
    EXPECT_EQ(info.expiration,1444444);
    EXPECT_EQ(info.timeout,10);
}

TEST_F(ActionsUtilsTests, testParseFailsWhenActionIsInvalidJson)
{
    EXPECT_THROW(
        std::ignore = ResponseActionsImpl::ActionsUtils::readUploadAction("", ResponseActionsImpl::ActionType::UPLOADFILE),
        ResponseActionsImpl::InvalidCommandFormat);
}

TEST_F(ActionsUtilsTests, testParseFailsWhenActionISUploadFileWhenExpectingFolder)
{
    nlohmann::json action = getDefaultUploadObject(ResponseActionsImpl::ActionType::UPLOADFILE);
    EXPECT_THROW(
        std::ignore = ResponseActionsImpl::ActionsUtils::readUploadAction(action.dump(), ResponseActionsImpl::ActionType::UPLOADFOLDER),
        ResponseActionsImpl::InvalidCommandFormat);
}

TEST_F(ActionsUtilsTests, testParseFailsWhenActionISUploadFolderWhenExpectingFile)
{
    nlohmann::json action = getDefaultUploadObject(ResponseActionsImpl::ActionType::UPLOADFOLDER);
    EXPECT_THROW(
        std::ignore = ResponseActionsImpl::ActionsUtils::readUploadAction(action.dump(), ResponseActionsImpl::ActionType::UPLOADFILE),
        ResponseActionsImpl::InvalidCommandFormat);
}

TEST_F(ActionsUtilsTests, testSucessfulParseCompressionEnabled)
{
    nlohmann::json action = getDefaultUploadObject(ResponseActionsImpl::ActionType::UPLOADFILE);
    action["compress"] = true;
    action["password"] = "password";
    ResponseActionsImpl::UploadInfo info =
        ResponseActionsImpl::ActionsUtils::readUploadAction(action.dump(), ResponseActionsImpl::ActionType::UPLOADFILE);

    EXPECT_EQ(info.compress, true);
    EXPECT_EQ(info.password, "password");
}

TEST_F(ActionsUtilsTests, testFailedParseInvalidValue)
{
    nlohmann::json action = getDefaultUploadObject(ResponseActionsImpl::ActionType::UPLOADFILE);
    action["url"] = 1000;
    EXPECT_THROW(std::ignore = ResponseActionsImpl::ActionsUtils::readUploadAction(action.dump(),ResponseActionsImpl::ActionType::UPLOADFILE),ResponseActionsImpl::InvalidCommandFormat);
}

TEST_F(ActionsUtilsTests, testFailedParseMissingUrl)
{
    nlohmann::json action = getDefaultUploadObject(ResponseActionsImpl::ActionType::UPLOADFILE);
    action.erase("url");
    EXPECT_THROW(std::ignore = ResponseActionsImpl::ActionsUtils::readUploadAction(action.dump(),ResponseActionsImpl::ActionType::UPLOADFILE),ResponseActionsImpl::InvalidCommandFormat);
}

TEST_F(ActionsUtilsTests, testFailedParseMissingExpiration)
{
    nlohmann::json action = getDefaultUploadObject(ResponseActionsImpl::ActionType::UPLOADFILE);
    action.erase("expiration");
    EXPECT_THROW(std::ignore = ResponseActionsImpl::ActionsUtils::readUploadAction(action.dump(),ResponseActionsImpl::ActionType::UPLOADFILE),ResponseActionsImpl::InvalidCommandFormat);
}

TEST_F(ActionsUtilsTests, testFailedParseMissingTimeout)
{
    nlohmann::json action = getDefaultUploadObject(ResponseActionsImpl::ActionType::UPLOADFILE);
    action.erase("timeout");
    EXPECT_THROW(std::ignore = ResponseActionsImpl::ActionsUtils::readUploadAction(action.dump(),ResponseActionsImpl::ActionType::UPLOADFILE),ResponseActionsImpl::InvalidCommandFormat);
}

TEST_F(ActionsUtilsTests, testFailedParseMissingMaxUploadSizeBytes)
{
    nlohmann::json action = getDefaultUploadObject(ResponseActionsImpl::ActionType::UPLOADFILE);
    action.erase("maxUploadSizeBytes");
    EXPECT_THROW(std::ignore = ResponseActionsImpl::ActionsUtils::readUploadAction(action.dump(),ResponseActionsImpl::ActionType::UPLOADFILE),ResponseActionsImpl::InvalidCommandFormat);
}

TEST_F(ActionsUtilsTests, testFailedParseMissingTargetFile)
{
    nlohmann::json action = getDefaultUploadObject(ResponseActionsImpl::ActionType::UPLOADFILE);
    action.erase("targetFile");
    EXPECT_THROW(std::ignore = ResponseActionsImpl::ActionsUtils::readUploadAction(action.dump(),ResponseActionsImpl::ActionType::UPLOADFILE),ResponseActionsImpl::InvalidCommandFormat);
}

TEST_F(ActionsUtilsTests, testSendResponse)
{
    auto mockFileSystem = new NaggyMock<MockFileSystem>();
    auto mockFilePermissions = new NaggyMock<MockFilePermissions>();
    EXPECT_CALL(*mockFileSystem, writeFile(_, "content"));
    EXPECT_CALL(*mockFilePermissions, chown(_, "sophos-spl-user", "sophos-spl-group"));
    EXPECT_CALL(*mockFilePermissions, chmod(_, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP));
    EXPECT_CALL(*mockFileSystem, moveFile(_, "/opt/sophos-spl/base/mcs/response/CORE_correlation_id_response.json"));
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>{ mockFileSystem });
    Tests::replaceFilePermissions(std::unique_ptr<Common::FileSystem::IFilePermissions>{mockFilePermissions});

    ResponseActions::RACommon::sendResponse("correlation_id", "content");
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
        EXPECT_STREQ(except.what(), "Invalid command format. No 'url' in Download action JSON");
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
        EXPECT_STREQ(except.what(), "Invalid command format. No 'targetPath' in Download action JSON");
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
        EXPECT_STREQ(except.what(), "Invalid command format. No 'sha256' in Download action JSON");
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
        EXPECT_STREQ(except.what(), "Invalid command format. No 'timeout' in Download action JSON");
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
        EXPECT_STREQ(except.what(), "Invalid command format. No 'sizeBytes' in Download action JSON");
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
        EXPECT_STREQ(except.what(), "Invalid command format. No 'expiration' in Download action JSON");
    }
}

TEST_F(ActionsUtilsTests, testMissingpassword)
{
    nlohmann::json action = getDefaultDownloadAction();
    action.erase("password");
    EXPECT_NO_THROW(std::ignore = ResponseActionsImpl::ActionsUtils::readDownloadAction(action.dump()));
}

TEST_F(ActionsUtilsTests, testMissingdecompress)
{
    nlohmann::json action = getDefaultDownloadAction();
    action.erase("decompress");
    EXPECT_NO_THROW(std::ignore = ResponseActionsImpl::ActionsUtils::readDownloadAction(action.dump()));
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

TEST_F(ActionsUtilsTests, testWrongTypeDecompress)
{
    nlohmann::json action = getDefaultDownloadAction();
    action["decompress"] = "sickness";
    try
    {
        auto output = ResponseActionsImpl::ActionsUtils::readDownloadAction(action.dump());
        FAIL() << "Didnt throw due to missing essential action";
    }
    catch (const ResponseActionsImpl::InvalidCommandFormat& except)
    {
        EXPECT_STREQ(except.what(), "Invalid command format. Failed to parse download command json, json value in unexpected type: [json.exception.type_error.302] type must be boolean, but is string");
    }
}

TEST_F(ActionsUtilsTests, testWrongTypeSizeBytes)
{
    nlohmann::json action = getDefaultDownloadAction();
    action["sizeBytes"] = "sizebytes";
    try
    {
        auto output = ResponseActionsImpl::ActionsUtils::readDownloadAction(action.dump());
        FAIL() << "Didnt throw due to missing essential action";
    }
    catch (const ResponseActionsImpl::InvalidCommandFormat& except)
    {
        EXPECT_STREQ(except.what(), "Invalid command format. Failed to parse download command json, json value in unexpected type: [json.exception.type_error.302] type must be number, but is string");
    }
}

TEST_F(ActionsUtilsTests, testWrongTypeExpiration)
{
    nlohmann::json action = getDefaultDownloadAction();
    action["expiration"] = "expiration";
    try
    {
        auto output = ResponseActionsImpl::ActionsUtils::readDownloadAction(action.dump());
        FAIL() << "Didnt throw due to missing essential action";
    }
    catch (const ResponseActionsImpl::InvalidCommandFormat& except)
    {
        EXPECT_STREQ(except.what(), "Invalid command format. Failed to parse download command json, json value in unexpected type: [json.exception.type_error.302] type must be number, but is string");
    }
}

TEST_F(ActionsUtilsTests, testWrongTypeTimeout)
{
    nlohmann::json action = getDefaultDownloadAction();
    action["timeout"] = "timeout";
    try
    {
        auto output = ResponseActionsImpl::ActionsUtils::readDownloadAction(action.dump());
        FAIL() << "Didnt throw due to missing essential action";
    }
    catch (const ResponseActionsImpl::InvalidCommandFormat& except)
    {
        EXPECT_STREQ(except.what(), "Invalid command format. Failed to parse download command json, json value in unexpected type: [json.exception.type_error.302] type must be number, but is string");
    }
}

TEST_F(ActionsUtilsTests, testWrongTypeSHA256)
{
    nlohmann::json action = getDefaultDownloadAction();
    action["sha256"] = 256;
    try
    {
        auto output = ResponseActionsImpl::ActionsUtils::readDownloadAction(action.dump());
        FAIL() << "Didnt throw due to missing essential action";
    }
    catch (const ResponseActionsImpl::InvalidCommandFormat& except)
    {
        EXPECT_STREQ(except.what(), "Invalid command format. Failed to parse download command json, json value in unexpected type: [json.exception.type_error.302] type must be string, but is number");
    }
}

TEST_F(ActionsUtilsTests, testWrongTypeURL)
{
    nlohmann::json action = getDefaultDownloadAction();
    action["url"] = 123;
    try
    {
        auto output = ResponseActionsImpl::ActionsUtils::readDownloadAction(action.dump());
        FAIL() << "Didnt throw due to missing essential action";
    }
    catch (const ResponseActionsImpl::InvalidCommandFormat& except)
    {
        EXPECT_STREQ(except.what(), "Invalid command format. Failed to parse download command json, json value in unexpected type: [json.exception.type_error.302] type must be string, but is number");
    }
}

TEST_F(ActionsUtilsTests, testWrongTypeTargetPath)
{
    nlohmann::json action = getDefaultDownloadAction();
    action["targetPath"] = 930752758;
    try
    {
        auto output = ResponseActionsImpl::ActionsUtils::readDownloadAction(action.dump());
        FAIL() << "Didnt throw due to missing essential action";
    }
    catch (const ResponseActionsImpl::InvalidCommandFormat& except)
    {
        EXPECT_STREQ(except.what(), "Invalid command format. Failed to parse download command json, json value in unexpected type: [json.exception.type_error.302] type must be string, but is number");
    }
}

TEST_F(ActionsUtilsTests, testEmptyTargetPath)
{
    nlohmann::json action = getDefaultDownloadAction();
    action["targetPath"] = "";
    try
    {
        auto output = ResponseActionsImpl::ActionsUtils::readDownloadAction(action.dump());
        FAIL() << "Didnt throw due to empty target path field";
    }
    catch (const ResponseActionsImpl::InvalidCommandFormat& except)
    {
        EXPECT_STREQ(except.what(), "Invalid command format. Target Path field is empty");
    }
}

TEST_F(ActionsUtilsTests, testEmptysha256)
{
    nlohmann::json action = getDefaultDownloadAction();
    action["sha256"] = "";
    try
    {
        auto output = ResponseActionsImpl::ActionsUtils::readDownloadAction(action.dump());
        FAIL() << "Didnt throw due to empty sha256 field";
    }
    catch (const ResponseActionsImpl::InvalidCommandFormat& except)
    {
        EXPECT_STREQ(except.what(), "Invalid command format. sha256 field is empty");
    }
}

TEST_F(ActionsUtilsTests, testEmptyurl)
{
    nlohmann::json action = getDefaultDownloadAction();
    action["url"] = "";
    try
    {
        auto output = ResponseActionsImpl::ActionsUtils::readDownloadAction(action.dump());
        FAIL() << "Didnt throw due to empty url field";
    }
    catch (const ResponseActionsImpl::InvalidCommandFormat& except)
    {
        EXPECT_STREQ(except.what(), "Invalid command format. url field is empty");
    }
}

TEST_F(ActionsUtilsTests, testWrongTypePassword)
{
    nlohmann::json action = getDefaultDownloadAction();
    action["password"] = 999;
    try
    {
        auto output = ResponseActionsImpl::ActionsUtils::readDownloadAction(action.dump());
        FAIL() << "Didnt throw due to missing essential action";
    }
    catch (const ResponseActionsImpl::InvalidCommandFormat& except)
    {
        EXPECT_STREQ(except.what(), "Invalid command format. Failed to parse download command json, json value in unexpected type: [json.exception.type_error.302] type must be string, but is number");
    }
}