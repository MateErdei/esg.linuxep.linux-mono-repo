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

using namespace ResponseActionsImpl;

class ActionsUtilsTests : public MemoryAppenderUsingTests
{
public:
    ActionsUtilsTests()
        : MemoryAppenderUsingTests("ResponseActionsImpl")
    {}
    nlohmann::json getDefaultUploadObject(ActionType type)
    {
        nlohmann::json action;

        std::string targetKey;
        switch (type)
        {
            case ActionType::UPLOADFILE:
                targetKey = "targetFile";
                break;
            case ActionType::UPLOADFOLDER:
                targetKey = "targetFolder";
                break;
            default:
                throw InvalidCommandFormat("invalid type");
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

//**********************GENERIC***************************
TEST_F(ActionsUtilsTests, expiry)
{
    EXPECT_TRUE(ActionsUtils::isExpired(1000));
    // time is set here to Tue 8 Feb 17:12:46 GMT 2033
    EXPECT_FALSE(ActionsUtils::isExpired(1991495566));
    Common::UtilityImpl::FormattedTime time;
    u_int64_t currentTime = time.currentEpochTimeInSecondsAsInteger();
    EXPECT_TRUE(ActionsUtils::isExpired(currentTime-10));
    EXPECT_FALSE(ActionsUtils::isExpired(currentTime+10));
}

TEST_F(ActionsUtilsTests, setResultErrorTypeAndErrorMessage)
{
    nlohmann::json json;
    const std::string errorType = "ErrorType";
    const std::string errorMessage = "ErrorMessage";
    const int result = static_cast<int>(ResponseActions::RACommon::ResponseResult::ERROR);

    ActionsUtils::setErrorInfo(json, result, errorMessage, errorType);

    EXPECT_EQ(json.at("result"), result);
    EXPECT_EQ(json.at("errorMessage"), errorMessage);
    EXPECT_EQ(json.at("errorType"), errorType);
}

TEST_F(ActionsUtilsTests, setResultAndErrorType)
{
    nlohmann::json json;
    const std::string errorType = "ErrorType";
    const std::string errorMessage = "";
    const int result = static_cast<int>(ResponseActions::RACommon::ResponseResult::ERROR);

    ActionsUtils::setErrorInfo(json, result, errorMessage, errorType);

    EXPECT_EQ(json.at("result"), result);
    EXPECT_EQ(json.at("errorType"), errorType);
    EXPECT_FALSE(json.contains("errorMessage"));
}

TEST_F(ActionsUtilsTests, setResultAndErrorMessage)
{
    nlohmann::json json;
    const std::string errorType = "";
    const std::string errorMessage = "ErrorMessage";
    const int result = static_cast<int>(ResponseActions::RACommon::ResponseResult::ERROR);

    ActionsUtils::setErrorInfo(json, result, errorMessage, errorType);

    EXPECT_EQ(json.at("result"), result);
    EXPECT_EQ(json.at("errorMessage"), errorMessage);
    EXPECT_FALSE(json.contains("errorType"));
}

TEST_F(ActionsUtilsTests, resetJsonError)
{
    nlohmann::json json;
    const int duration = 123;
    const std::string type = ResponseActions::RACommon::UPLOAD_FILE_RESPONSE_TYPE;

    json["result"] = static_cast<int>(ResponseActions::RACommon::ResponseResult::ERROR;
    json["errorMessage"] = "ErrorMessage";
    json["errorType"] = "ErrorType";
    json["duration"] = duration;
    json["type"] = type;

    ActionsUtils::resetErrorInfo(json);

    EXPECT_FALSE(json.contains("errorType"));
    EXPECT_FALSE(json.contains("errorMessage"));
    EXPECT_FALSE(json.contains("result"));

    EXPECT_EQ(json.at("duration"), duration);
    EXPECT_EQ(json.at("type"), type);
}

//**********************UPLOAD ACTION***************************
TEST_F(ActionsUtilsTests, successfulParseUploadFile)
{
    nlohmann::json action = getDefaultUploadObject(ActionType::UPLOADFILE);
    UploadInfo info = ActionsUtils::readUploadAction(action.dump(),ActionType::UPLOADFILE);
    EXPECT_EQ(info.targetPath,"path");
    EXPECT_EQ(info.maxSize,10000000);
    EXPECT_EQ(info.compress,false);
    EXPECT_EQ(info.password,"");
    EXPECT_EQ(info.expiration,1444444);
    EXPECT_EQ(info.timeout,10);
}

TEST_F(ActionsUtilsTests, successfulParseUploadFolder)
{
    nlohmann::json action = getDefaultUploadObject(ActionType::UPLOADFOLDER);
    UploadInfo info = ActionsUtils::readUploadAction(action.dump(),ActionType::UPLOADFOLDER);
    EXPECT_EQ(info.targetPath,"path");
    EXPECT_EQ(info.maxSize,10000000);
    EXPECT_EQ(info.compress,false);
    EXPECT_EQ(info.password,"");
    EXPECT_EQ(info.expiration,1444444);
    EXPECT_EQ(info.timeout,10);
}

TEST_F(ActionsUtilsTests, readFailsWhenActionIsInvalidJson)
{
    EXPECT_THROW(
        std::ignore = ActionsUtils::readUploadAction("", ActionType::UPLOADFILE),
        InvalidCommandFormat);
}

TEST_F(ActionsUtilsTests, readFailsWhenActionISUploadFileWhenExpectingFolder)
{
    nlohmann::json action = getDefaultUploadObject(ActionType::UPLOADFILE);
    EXPECT_THROW(
        std::ignore = ActionsUtils::readUploadAction(action.dump(), ActionType::UPLOADFOLDER),
        InvalidCommandFormat);
}

TEST_F(ActionsUtilsTests, readFailsWhenActionISUploadFolderWhenExpectingFile)
{
    nlohmann::json action = getDefaultUploadObject(ActionType::UPLOADFOLDER);
    EXPECT_THROW(
        std::ignore = ActionsUtils::readUploadAction(action.dump(), ActionType::UPLOADFILE),
        InvalidCommandFormat);
}

TEST_F(ActionsUtilsTests, successfulParseCompressionEnabled)
{
    nlohmann::json action = getDefaultUploadObject(ActionType::UPLOADFILE);
    action["compress"] = true;
    action["password"] = "password";
    UploadInfo info =
        ActionsUtils::readUploadAction(action.dump(), ActionType::UPLOADFILE);

    EXPECT_EQ(info.compress, true);
    EXPECT_EQ(info.password, "password");
}

TEST_F(ActionsUtilsTests, failedParseInvalidValue)
{
    nlohmann::json action = getDefaultUploadObject(ActionType::UPLOADFILE);
    action["url"] = 1000;
    EXPECT_THROW(std::ignore = ActionsUtils::readUploadAction(action.dump(),ActionType::UPLOADFILE),InvalidCommandFormat);
}

TEST_F(ActionsUtilsTests, failedParseMissingUrl)
{
    nlohmann::json action = getDefaultUploadObject(ActionType::UPLOADFILE);
    action.erase("url");
    EXPECT_THROW(std::ignore = ActionsUtils::readUploadAction(action.dump(),ActionType::UPLOADFILE),InvalidCommandFormat);
}

TEST_F(ActionsUtilsTests, failedParseMissingExpiration)
{
    nlohmann::json action = getDefaultUploadObject(ActionType::UPLOADFILE);
    action.erase("expiration");
    EXPECT_THROW(std::ignore = ActionsUtils::readUploadAction(action.dump(),ActionType::UPLOADFILE),InvalidCommandFormat);
}

TEST_F(ActionsUtilsTests, failedParseMissingTimeout)
{
    nlohmann::json action = getDefaultUploadObject(ActionType::UPLOADFILE);
    action.erase("timeout");
    EXPECT_THROW(std::ignore = ActionsUtils::readUploadAction(action.dump(),ActionType::UPLOADFILE),InvalidCommandFormat);
}

TEST_F(ActionsUtilsTests, failedParseMissingMaxUploadSizeBytes)
{
    nlohmann::json action = getDefaultUploadObject(ActionType::UPLOADFILE);
    action.erase("maxUploadSizeBytes");
    EXPECT_THROW(std::ignore = ActionsUtils::readUploadAction(action.dump(),ActionType::UPLOADFILE),InvalidCommandFormat);
}

TEST_F(ActionsUtilsTests, failedParseMissingTargetFile)
{
    nlohmann::json action = getDefaultUploadObject(ActionType::UPLOADFILE);
    action.erase("targetFile");
    EXPECT_THROW(std::ignore = ActionsUtils::readUploadAction(action.dump(),ActionType::UPLOADFILE),InvalidCommandFormat);
}

TEST_F(ActionsUtilsTests, sendResponse)
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
TEST_F(ActionsUtilsTests, downloadMissingUrl)
{
    nlohmann::json action = getDefaultDownloadAction();
    action.erase("url");
    try
    {
        auto output = ActionsUtils::readDownloadAction(action.dump());
        FAIL() << "Didnt throw due to missing essential action";
    }
    catch (const InvalidCommandFormat& except)
    {
        EXPECT_STREQ(except.what(), "Invalid command format. No 'url' in Download action JSON");
    }
}

TEST_F(ActionsUtilsTests, downloadMissingtargetPath)
{
    nlohmann::json action = getDefaultDownloadAction();
    action.erase("targetPath");
    try
    {
        auto output = ActionsUtils::readDownloadAction(action.dump());
        FAIL() << "Didnt throw due to missing essential action";
    }
    catch (const InvalidCommandFormat& except)
    {
        EXPECT_STREQ(except.what(), "Invalid command format. No 'targetPath' in Download action JSON");
    }
}

TEST_F(ActionsUtilsTests, downloadMissingsha256)
{
    nlohmann::json action = getDefaultDownloadAction();
    action.erase("sha256");
    try
    {
        auto output = ActionsUtils::readDownloadAction(action.dump());
        FAIL() << "Didnt throw due to missing essential action";
    }
    catch (const InvalidCommandFormat& except)
    {
        EXPECT_STREQ(except.what(), "Invalid command format. No 'sha256' in Download action JSON");
    }
}

TEST_F(ActionsUtilsTests, downloadMissingtimeout)
{
    nlohmann::json action = getDefaultDownloadAction();
    action.erase("timeout");
    try
    {
        auto output = ActionsUtils::readDownloadAction(action.dump());
        FAIL() << "Didnt throw due to missing essential action";
    }
    catch (const InvalidCommandFormat& except)
    {
        EXPECT_STREQ(except.what(), "Invalid command format. No 'timeout' in Download action JSON");
    }
}

TEST_F(ActionsUtilsTests, downloadMissingsizeBytes)
{
    nlohmann::json action = getDefaultDownloadAction();
    action.erase("sizeBytes");
    try
    {
        auto output = ActionsUtils::readDownloadAction(action.dump());
        FAIL() << "Didnt throw due to missing essential action";
    }
    catch (const InvalidCommandFormat& except)
    {
        EXPECT_STREQ(except.what(), "Invalid command format. No 'sizeBytes' in Download action JSON");
    }
}

TEST_F(ActionsUtilsTests, downloadMissingExpiration)
{
    nlohmann::json action = getDefaultDownloadAction();
    action.erase("expiration");
    try
    {
        auto output = ActionsUtils::readDownloadAction(action.dump());
        FAIL() << "Didnt throw due to missing essential action";
    }
    catch (const InvalidCommandFormat& except)
    {
        EXPECT_STREQ(except.what(), "Invalid command format. No 'expiration' in Download action JSON");
    }
}

TEST_F(ActionsUtilsTests, downloadNegativeExpiration)
{
    nlohmann::json action = getDefaultDownloadAction();
    action.at("expiration") = -123456487;
    try
    {
        auto output = ActionsUtils::readDownloadAction(action.dump());
        FAIL() << "Didnt throw due to negative value";
    }
    catch (const InvalidCommandFormat& except)
    {
        EXPECT_STREQ(except.what(), "Invalid command format. Failed to process DownloadInfo from action JSON: expiration is a negative value: -123456487");
    }
}

TEST_F(ActionsUtilsTests, downloadNegativeExpirationFloat)
{
    nlohmann::json action = getDefaultDownloadAction();
    action.at("expiration") = -123456.487;
    try
    {
        auto output = ActionsUtils::readDownloadAction(action.dump());
        FAIL() << "Didnt throw due to negative value";
    }
    catch (const InvalidCommandFormat& except)
    {
        EXPECT_STREQ(except.what(), "Invalid command format. Failed to process DownloadInfo from action JSON: expiration is a negative value: -123456.487");
    }
}

TEST_F(ActionsUtilsTests, downloadNegativeSizeBytes)
{
    nlohmann::json action = getDefaultDownloadAction();
    action.at("sizeBytes") = -123456487;
    try
    {
        auto output = ActionsUtils::readDownloadAction(action.dump());
        FAIL() << "Didnt throw due to negative value";
    }
    catch (const InvalidCommandFormat& except)
    {
        EXPECT_STREQ(except.what(), "Invalid command format. Failed to process DownloadInfo from action JSON: sizeBytes is a negative value: -123456487");
    }
}

TEST_F(ActionsUtilsTests, downloadNegativeSizeBytesFloat)
{
    nlohmann::json action = getDefaultDownloadAction();
    action.at("sizeBytes") = -123456.487;
    try
    {
        auto output = ActionsUtils::readDownloadAction(action.dump());
        FAIL() << "Didnt throw due to negative value";
    }
    catch (const InvalidCommandFormat& except)
    {
        EXPECT_STREQ(except.what(), "Invalid command format. Failed to process DownloadInfo from action JSON: sizeBytes is a negative value: -123456.487");
    }
}

TEST_F(ActionsUtilsTests, downloadLargeSizeBytes)
{
    std::string action (
        R"({"type": "sophos.mgt.action.DownloadFile"
        ,"timeout": 1000
        ,"sizeBytes": 18446744073709551616
        ,"url": "https://s3.com/download.zip"
        ,"targetPath": "path"
        ,"sha256": "sha"
        ,"expiration": 1000})");

    try
    {
        auto output = ActionsUtils::readDownloadAction(action);
        FAIL() << "Didnt throw due to negative value";
    }
    catch (const InvalidCommandFormat& except)
    {
        EXPECT_STREQ(except.what(), "Invalid command format. sizeBytes field has been evaluated to 0. Very large values can also cause this.");
    }
}

TEST_F(ActionsUtilsTests, downloadLargeExpiration)
{
    std::string action (
        R"({"type": "sophos.mgt.action.DownloadFile"
        ,"timeout": 1000
        ,"sizeBytes": 1000
        ,"url": "https://s3.com/download.zip"
        ,"targetPath": "path"
        ,"sha256": "sha"
        ,"expiration": 18446744073709551616})");
    auto res = ActionsUtils::readDownloadAction(action);
    res.expiration = 0;
}

TEST_F(ActionsUtilsTests, downloadMissingPassword)
{
    nlohmann::json action = getDefaultDownloadAction();
    action.erase("password");
    EXPECT_NO_THROW(std::ignore = ActionsUtils::readDownloadAction(action.dump()));
}

TEST_F(ActionsUtilsTests, downloadMissingdecompress)
{
    nlohmann::json action = getDefaultDownloadAction();
    action.erase("decompress");
    EXPECT_NO_THROW(std::ignore = ActionsUtils::readDownloadAction(action.dump()));
}

TEST_F(ActionsUtilsTests, downloadSuccessfulParsing)
{
    nlohmann::json action = getDefaultDownloadAction();
    auto output = ActionsUtils::readDownloadAction(action.dump());

    EXPECT_EQ(output.decompress, action.at("decompress"));
    EXPECT_EQ(output.sizeBytes, action.at("sizeBytes"));
    EXPECT_EQ(output.expiration, action.at("expiration"));
    EXPECT_EQ(output.timeout, action.at("timeout"));
    EXPECT_EQ(output.sha256, action.at("sha256"));
    EXPECT_EQ(output.url, action.at("url"));
    EXPECT_EQ(output.targetPath, action.at("targetPath"));
    EXPECT_EQ(output.password, action.at("password"));
}

TEST_F(ActionsUtilsTests, downloadHugeurl)
{
    nlohmann::json action = getDefaultDownloadAction();
    const std::string largeStr(30000, 'a');
    const std::string largeURL("https://s3.com/download" + largeStr + ".zip");
    action["url"] = largeURL;

    auto output = ActionsUtils::readDownloadAction(action.dump());

    EXPECT_EQ(output.decompress, action.at("decompress"));
    EXPECT_EQ(output.sizeBytes, action.at("sizeBytes"));
    EXPECT_EQ(output.expiration, action.at("expiration"));
    EXPECT_EQ(output.timeout, action.at("timeout"));
    EXPECT_EQ(output.sha256, action.at("sha256"));
    EXPECT_EQ(output.url, action.at("url"));
    EXPECT_EQ(output.targetPath, action.at("targetPath"));
    EXPECT_EQ(output.password, action.at("password"));
}

TEST_F(ActionsUtilsTests, downloadFloatExpiration)
{
    nlohmann::json action = getDefaultDownloadAction();
    action["expiration"] = 12345.789;

    auto output = ActionsUtils::readDownloadAction(action.dump());

    EXPECT_EQ(output.decompress, action.at("decompress"));
    EXPECT_EQ(output.sizeBytes, action.at("sizeBytes"));
    EXPECT_EQ(output.expiration, 12345);
    EXPECT_EQ(output.timeout, action.at("timeout"));
    EXPECT_EQ(output.sha256, action.at("sha256"));
    EXPECT_EQ(output.url, action.at("url"));
    EXPECT_EQ(output.targetPath, action.at("targetPath"));
    EXPECT_EQ(output.password, action.at("password"));
}

TEST_F(ActionsUtilsTests, downloadFloatSizeBytes)
{
    nlohmann::json action = getDefaultDownloadAction();
    action["sizeBytes"] = 12345.789;

    auto output = ActionsUtils::readDownloadAction(action.dump());

    EXPECT_EQ(output.decompress, action.at("decompress"));
    EXPECT_EQ(output.sizeBytes, 12345);
    EXPECT_EQ(output.expiration, action.at("expiration"));
    EXPECT_EQ(output.timeout, action.at("timeout"));
    EXPECT_EQ(output.sha256, action.at("sha256"));
    EXPECT_EQ(output.url, action.at("url"));
    EXPECT_EQ(output.targetPath, action.at("targetPath"));
    EXPECT_EQ(output.password, action.at("password"));
}


TEST_F(ActionsUtilsTests, downloadLargeTargetPath)
{
    nlohmann::json action = getDefaultDownloadAction();
    const std::string largeStr(30000, 'a');
    const std::string largeTargetPath("/tmp/folder/" + largeStr);
    action["targetPath"] = largeTargetPath;

    auto output = ActionsUtils::readDownloadAction(action.dump());

    EXPECT_EQ(output.decompress, action.at("decompress"));
    EXPECT_EQ(output.sizeBytes, action.at("sizeBytes"));
    EXPECT_EQ(output.expiration, action.at("expiration"));
    EXPECT_EQ(output.timeout, action.at("timeout"));
    EXPECT_EQ(output.sha256, action.at("sha256"));
    EXPECT_EQ(output.url, action.at("url"));
    EXPECT_EQ(output.targetPath, action.at("targetPath"));
    EXPECT_EQ(output.password, action.at("password"));
}

TEST_F(ActionsUtilsTests, downloadWrongTypeDecompress)
{
    nlohmann::json action = getDefaultDownloadAction();
    action["decompress"] = "sickness";
    try
    {
        auto output = ActionsUtils::readDownloadAction(action.dump());
        FAIL() << "Didnt throw due to missing essential action";
    }
    catch (const InvalidCommandFormat& except)
    {
        EXPECT_STREQ(except.what(), "Invalid command format. Failed to process DownloadInfo from action JSON: [json.exception.type_error.302] type must be boolean, but is string");
    }
}

TEST_F(ActionsUtilsTests, downloadWrongTypeSizeBytes)
{
    nlohmann::json action = getDefaultDownloadAction();
    action["sizeBytes"] = "sizebytes";
    try
    {
        auto output = ActionsUtils::readDownloadAction(action.dump());
        FAIL() << "Didnt throw due to missing essential action";
    }
    catch (const InvalidCommandFormat& except)
    {
        EXPECT_STREQ(except.what(), R"(Invalid command format. Failed to process DownloadInfo from action JSON: sizeBytes is not a number: "sizebytes")");
    }
}

TEST_F(ActionsUtilsTests, downloadWrongTypeExpiration)
{
    nlohmann::json action = getDefaultDownloadAction();
    action["expiration"] = "expiration";
    try
    {
        auto output = ActionsUtils::readDownloadAction(action.dump());
        FAIL() << "Didnt throw due to missing essential action";
    }
    catch (const InvalidCommandFormat& except)
    {
        EXPECT_STREQ(except.what(), R"(Invalid command format. Failed to process DownloadInfo from action JSON: expiration is not a number: "expiration")");
    }
}

TEST_F(ActionsUtilsTests, downloadWrongTypeTimeout)
{
    nlohmann::json action = getDefaultDownloadAction();
    action["timeout"] = "timeout";
    try
    {
        auto output = ActionsUtils::readDownloadAction(action.dump());
        FAIL() << "Didnt throw due to missing essential action";
    }
    catch (const InvalidCommandFormat& except)
    {
        EXPECT_STREQ(except.what(), "Invalid command format. Failed to process DownloadInfo from action JSON: [json.exception.type_error.302] type must be number, but is string");
    }
}

TEST_F(ActionsUtilsTests, downloadWrongTypeSHA256)
{
    nlohmann::json action = getDefaultDownloadAction();
    action["sha256"] = 256;
    try
    {
        auto output = ActionsUtils::readDownloadAction(action.dump());
        FAIL() << "Didnt throw due to missing essential action";
    }
    catch (const InvalidCommandFormat& except)
    {
        EXPECT_STREQ(except.what(), "Invalid command format. Failed to process DownloadInfo from action JSON: [json.exception.type_error.302] type must be string, but is number");
    }
}

TEST_F(ActionsUtilsTests, downloadWrongTypeURL)
{
    nlohmann::json action = getDefaultDownloadAction();
    action["url"] = 123;
    try
    {
        auto output = ActionsUtils::readDownloadAction(action.dump());
        FAIL() << "Didnt throw due to missing essential action";
    }
    catch (const InvalidCommandFormat& except)
    {
        EXPECT_STREQ(except.what(), "Invalid command format. Failed to process DownloadInfo from action JSON: [json.exception.type_error.302] type must be string, but is number");
    }
}

TEST_F(ActionsUtilsTests, downloadWrongTypeTargetPath)
{
    nlohmann::json action = getDefaultDownloadAction();
    action["targetPath"] = 930752758;
    try
    {
        auto output = ActionsUtils::readDownloadAction(action.dump());
        FAIL() << "Didnt throw due to missing essential action";
    }
    catch (const InvalidCommandFormat& except)
    {
        EXPECT_STREQ(except.what(), "Invalid command format. Failed to process DownloadInfo from action JSON: [json.exception.type_error.302] type must be string, but is number");
    }
}

TEST_F(ActionsUtilsTests, downloadEmptyTargetPath)
{
    nlohmann::json action = getDefaultDownloadAction();
    action["targetPath"] = "";
    try
    {
        auto output = ActionsUtils::readDownloadAction(action.dump());
        FAIL() << "Didnt throw due to empty target path field";
    }
    catch (const InvalidCommandFormat& except)
    {
        EXPECT_STREQ(except.what(), "Invalid command format. Failed to process DownloadInfo from action JSON: Target Path field is empty");
    }
}

TEST_F(ActionsUtilsTests, downloadEmptysha256)
{
    nlohmann::json action = getDefaultDownloadAction();
    action["sha256"] = "";
    try
    {
        auto output = ActionsUtils::readDownloadAction(action.dump());
        FAIL() << "Didnt throw due to empty sha256 field";
    }
    catch (const InvalidCommandFormat& except)
    {
        EXPECT_STREQ(except.what(), "Invalid command format. Failed to process DownloadInfo from action JSON: sha256 field is empty");
    }
}

TEST_F(ActionsUtilsTests, downloadEmptyurl)
{
    nlohmann::json action = getDefaultDownloadAction();
    action["url"] = "";
    try
    {
        auto output = ActionsUtils::readDownloadAction(action.dump());
        FAIL() << "Didnt throw due to empty url field";
    }
    catch (const InvalidCommandFormat& except)
    {
        EXPECT_STREQ(except.what(), "Invalid command format. Failed to process DownloadInfo from action JSON: url field is empty");
    }
}


TEST_F(ActionsUtilsTests, downloadWrongTypePassword)
{
    nlohmann::json action = getDefaultDownloadAction();
    action["password"] = 999;
    try
    {
        auto output = ActionsUtils::readDownloadAction(action.dump());
        FAIL() << "Didnt throw due to missing essential action";
    }
    catch (const InvalidCommandFormat& except)
    {
        EXPECT_STREQ(except.what(), "Invalid command format. Failed to process DownloadInfo from action JSON: [json.exception.type_error.302] type must be string, but is number");
    }
}

//**********************RUN COMMAND ACTION***************************
namespace {
    std::string getDefaultCommandJsonString()
    {
        return R"({
            "type": "sophos.mgt.action.RunCommands",
            "commands": [
                "echo one",
                "echo two > /tmp/test.txt"
            ],
            "ignoreError": true,
            "timeout": 60,
            "expiration": 144444000000004
        })";
    }

    std::string getCommandJsonStringWithWrongValueType()
    {
        return R"({
            "type": "sophos.mgt.action.RunCommands",
            "commands": [
                "echo one",
                "echo two > /tmp/test.txt"
            ],
            "ignoreError": true,
            "timeout": "60",
            "expiration": 144444000000004
        })";
    }
}
TEST_F(ActionsUtilsTests, readCommandActionSuccess)
{
    std::string commandJson = getDefaultCommandJsonString();
    auto command = ActionsUtils::readCommandAction(commandJson);
    std::vector<std::string> expectedCommands = { { "echo one" }, { R"(echo two > /tmp/test.txt)" } };

    EXPECT_EQ(command.ignoreError, true);
    EXPECT_EQ(command.timeout, 60);
    EXPECT_EQ(command.expiration, 144444000000004);
    EXPECT_EQ(command.commands, expectedCommands);
}

TEST_F(ActionsUtilsTests, readCommandHandlesEscapedCharsInCmd)
{
    std::string commandJson = R"({
            "type": "sophos.mgt.action.RunCommands",
            "commands": [
                "echo {\"number\":13, \"string\":\"a string and a quote \" }"
            ],
            "ignoreError": true,
            "timeout": 60,
            "expiration": 144444000000004
            })";
    auto command = ActionsUtils::readCommandAction(commandJson);
    std::vector<std::string> expectedCommands = {  { R"(echo {"number":13, "string":"a string and a quote " })" } };
    EXPECT_EQ(command.ignoreError, true);
    EXPECT_EQ(command.timeout, 60);
    EXPECT_EQ(command.expiration, 144444000000004);
    EXPECT_EQ(command.commands, expectedCommands);
}

TEST_F(ActionsUtilsTests, readCommandFailsDueToMissingTypeRequiredField)
{
    std::string commandJson = R"({
        "commands": [
            "echo one",
            "echo two > /tmp/test.txt"
        ],
        "ignoreError": true,
        "timeout": 60,
        "expiration": 144444000000004
        })";
    EXPECT_THAT([&]() { std::ignore = ActionsUtils::readCommandAction(commandJson); },
                ThrowsMessage<InvalidCommandFormat>(HasSubstr("Invalid command format. No 'type' in Run Command action JSON")));
}

TEST_F(ActionsUtilsTests, readCommandFailsDueToMissingCommandsRequiredField)
{
    std::string commandJson = R"({
        "type": "sophos.mgt.action.RunCommands",
        "ignoreError": true,
        "timeout": 60,
        "expiration": 144444000000004
        })";
    EXPECT_THAT([&]() { std::ignore = ActionsUtils::readCommandAction(commandJson); },
                ThrowsMessage<InvalidCommandFormat>(HasSubstr("Invalid command format. No 'commands' in Run Command action JSON")));
}

TEST_F(ActionsUtilsTests, readCommandFailsDueToMissingIgnoreErrorRequiredField)
{
    std::string commandJson = R"({
        "type": "sophos.mgt.action.RunCommands",
        "commands": [
            "echo one",
            "echo two > /tmp/test.txt"
        ],
        "timeout": 60,
        "expiration": 144444000000004
        })";
    EXPECT_THAT([&]() { std::ignore = ActionsUtils::readCommandAction(commandJson); },
                ThrowsMessage<InvalidCommandFormat>(HasSubstr("Invalid command format. No 'ignoreError' in Run Command action JSON")));
}

TEST_F(ActionsUtilsTests, readCommandFailsDueToMissingTimeoutRequiredField)
{
    std::string commandJson = R"({
        "type": "sophos.mgt.action.RunCommands",
        "commands": [
            "echo one",
            "echo two > /tmp/test.txt"
        ],
        "ignoreError": true,
        "expiration": 144444000000004
        })";
    EXPECT_THAT([&]() { std::ignore = ActionsUtils::readCommandAction(commandJson); },
                ThrowsMessage<InvalidCommandFormat>(HasSubstr("Invalid command format. No 'timeout' in Run Command action JSON")));
}

TEST_F(ActionsUtilsTests, readCommandFailsDueToMissingExpirationRequiredField)
{
    std::string commandJson = R"({
        "type": "sophos.mgt.action.RunCommands",
        "commands": [
            "echo one",
            "echo two > /tmp/test.txt"
        ],
        "ignoreError": true,
        "timeout": 60
        })";
    EXPECT_THAT([&]() { std::ignore = ActionsUtils::readCommandAction(commandJson); },
                ThrowsMessage<InvalidCommandFormat>(HasSubstr("Invalid command format. No 'expiration' in Run Command action JSON")));
}



TEST_F(ActionsUtilsTests, readCommandFailsDueToTypeError)
{
    std::string commandJson = getCommandJsonStringWithWrongValueType();
    EXPECT_THAT([&]() { std::ignore = ActionsUtils::readCommandAction(commandJson); },
                ThrowsMessage<InvalidCommandFormat>(HasSubstr("Failed to process CommandRequest from action JSON")));
}

TEST_F(ActionsUtilsTests, readCommandFailsDueToMalformedJson)
{
    std::string commandJson = "this is not json";
    EXPECT_THAT([&]() { std::ignore = ActionsUtils::readCommandAction(commandJson); },
                ThrowsMessage<InvalidCommandFormat>(HasSubstr("syntax error")));
}

TEST_F(ActionsUtilsTests, readCommandFailsDueToEmptyJsonString)
{
    std::string commandJson = "";
    EXPECT_THAT([&]() { std::ignore = ActionsUtils::readCommandAction(commandJson); },
                    ThrowsMessage<InvalidCommandFormat>(HasSubstr("Run Command action JSON is empty")));
}

TEST_F(ActionsUtilsTests, readCommandFailsDueToMissingCommands)
{
    std::string commandJson = R"({
        "type": "sophos.mgt.action.RunCommands",
        "commands": [
        ],
        "ignoreError": true,
        "timeout": 60,
        "expiration": 144444000000004
        })";
    EXPECT_THAT([&]() { std::ignore = ActionsUtils::readCommandAction(commandJson); },
                ThrowsMessage<InvalidCommandFormat>(HasSubstr("Invalid command format. No commands to perform in run command JSON: ")));
}

TEST_F(ActionsUtilsTests, readCommandFailsDueToEmptyJsonObject)
{
    std::string commandJson = "{}";
    EXPECT_THAT([&]() { std::ignore = ActionsUtils::readCommandAction(commandJson); },
                    ThrowsMessage<InvalidCommandFormat>(HasSubstr("Invalid command format. No 'type' in Run Command action JSON")));
}