// Copyright 2023-2024 Sophos Limited. All rights reserved.

#include "Common/UtilityImpl/TimeUtils.h"
#include "ResponseActions/ResponseActionsImpl/ActionRequiredFields.h"
#include "ResponseActions/ResponseActionsImpl/ActionsUtils.h"
#include "ResponseActions/ResponseActionsImpl/InvalidCommandFormat.h"
#include "tests/Common/Helpers/FileSystemReplaceAndRestore.h"
#include "tests/Common/Helpers/MemoryAppender.h"
#include "tests/Common/Helpers/MockFilePermissions.h"
#include "tests/Common/Helpers/MockFileSystem.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace ResponseActionsImpl;

class ActionsUtilsTests : public MemoryAppenderUsingTests
{
public:
    ActionsUtilsTests()
        : MemoryAppenderUsingTests("ResponseActionsImpl")
    {}

    void TearDown() override
    {
        Tests::restoreFileSystem();
        Tests::restoreFilePermissions();
    }
};

class ResponseActionFieldsParameterized
    : public ::testing::TestWithParam<std::string>
{
protected:
    void SetUp() override
    {
        m_loggingSetup = Common::Logging::LOGOFFFORTEST();
    }
    Common::Logging::ConsoleLoggingSetup m_loggingSetup;
};

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

nlohmann::json getDefaultRunCommandAction()
{
    nlohmann::json action;
    action["commands"] = {"echo one"};
    action["ignoreError"] = true;
    action["expiration"] = 144444000000004;
    action["timeout"] = 10;
    return action;
}

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

    json["result"] = static_cast<int>(ResponseActions::RACommon::ResponseResult::ERROR);
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

TEST_F(ActionsUtilsTests, uploadFailsWhenActionIsInvalidJson)
{
    try
    {
        auto output = ActionsUtils::readUploadAction("", ActionType::UPLOADFILE);
        FAIL() << "Didnt throw due to missing action type";
    }
    catch (const InvalidCommandFormat& except)
    {
        EXPECT_STREQ(except.what(), "Invalid command format. UploadFile action JSON is empty");
    }
}

TEST_F(ActionsUtilsTests, readFailsWhenActionIsUploadFileWhenExpectingFolder)
{
    nlohmann::json action = getDefaultUploadObject(ActionType::UPLOADFILE);

    try
    {
        auto output = ActionsUtils::readUploadAction(action.dump(), ActionType::UPLOADFOLDER);
        FAIL() << "Didnt throw due to missing action type";
    }
    catch (const InvalidCommandFormat& except)
    {
        EXPECT_STREQ(except.what(), "Invalid command format. No 'targetFolder' in UploadFolder action JSON");
    }
}

TEST_F(ActionsUtilsTests, readFailsWhenActionIsUploadFolderWhenExpectingFile)
{
    nlohmann::json action = getDefaultUploadObject(ActionType::UPLOADFOLDER);

    try
    {
        auto output = ActionsUtils::readUploadAction(action.dump(), ActionType::UPLOADFILE);
        FAIL() << "Didnt throw due to missing action type";
    }
    catch (const InvalidCommandFormat& except)
    {
        EXPECT_STREQ(except.what(), "Invalid command format. No 'targetFile' in UploadFile action JSON");
    }
}

TEST_F(ActionsUtilsTests, uploadSuccessCompressionEnabled)
{
    nlohmann::json action = getDefaultUploadObject(ActionType::UPLOADFILE);
    action["compress"] = true;
    action["password"] = "password";
    auto info = ActionsUtils::readUploadAction(action.dump(), ActionType::UPLOADFILE);

    EXPECT_EQ(info.compress, true);
    EXPECT_EQ(info.password, "password");
}

TEST_F(ActionsUtilsTests, uploadHugeurl)
{
    auto actionType = ActionType::UPLOADFILE;
    nlohmann::json action = getDefaultUploadObject(actionType);
    const std::string largeStr(30000, 'a');
    const std::string largeURL("https://s3.com/download" + largeStr + ".zip");
    action["url"] = largeURL;

    auto output = ActionsUtils::readUploadAction(action.dump(), actionType);

    EXPECT_EQ(output.compress, action.at("compress"));
    EXPECT_EQ(output.password, action.at("password"));
    EXPECT_EQ(output.expiration, action.at("expiration"));
    EXPECT_EQ(output.timeout, action.at("timeout"));
    EXPECT_EQ(output.maxSize, action.at("maxUploadSizeBytes"));
    EXPECT_EQ(output.url, action.at("url"));
    EXPECT_EQ(output.targetPath, action.at("targetFile"));
    EXPECT_EQ(output.password, action.at("password"));
}

TEST_F(ActionsUtilsTests, uploadFailInvalidUrL)
{
    nlohmann::json action = getDefaultUploadObject(ActionType::UPLOADFILE);
    action["url"] = 1000;
    try
    {
        auto output = ActionsUtils::readUploadAction(action.dump(),ActionType::UPLOADFILE);
        FAIL() << "Didnt throw due to missing essential action";
    }
    catch (const InvalidCommandFormat& except)
    {
        EXPECT_STREQ(except.what(), "Invalid command format. Failed to process UploadInfo from action JSON: [json.exception.type_error.302] type must be string, but is number");
    }

    action["url"] = false;
    try
    {
        auto output = ActionsUtils::readUploadAction(action.dump(),ActionType::UPLOADFILE);
        FAIL() << "Didnt throw due to missing essential action";
    }
    catch (const InvalidCommandFormat& except)
    {
        EXPECT_STREQ(except.what(), "Invalid command format. Failed to process UploadInfo from action JSON: [json.exception.type_error.302] type must be string, but is boolean");
    }
}

class UploadFolderRequiredFieldsParameterized : public ResponseActionFieldsParameterized {};

INSTANTIATE_TEST_SUITE_P(
    ActionsUtilsTests,
    UploadFolderRequiredFieldsParameterized,
    ::testing::ValuesIn(uploadFolderRequiredFields));

TEST_P(UploadFolderRequiredFieldsParameterized, UploadFolderMissingEssentialFields)
{
    auto field = GetParam();

    nlohmann::json action = getDefaultUploadObject(ActionType::UPLOADFOLDER);
    action.erase(field);
    const std::string expectedMsg = "Invalid command format. No '" + field + "' in UploadFolder action JSON";
    try
    {
        auto output = ActionsUtils::readUploadAction(action.dump(),ActionType::UPLOADFOLDER);
        FAIL() << "Didnt throw due to missing essential field: " << field;
    }
    catch (const InvalidCommandFormat& except)
    {
        EXPECT_STREQ(except.what(), expectedMsg.c_str());
    }
}

TEST_P(UploadFolderRequiredFieldsParameterized, UploadFolderUnsetEssentialFields)
{
    auto field = GetParam();

    nlohmann::json action = getDefaultUploadObject(ActionType::UPLOADFOLDER);
    if (!action[field].is_string())
    {
        action[field] = 0;
        UploadInfo output;
        EXPECT_NO_THROW(output = ActionsUtils::readUploadAction(action.dump(),ActionType::UPLOADFOLDER));
        if (field == "maxUploadSizeBytes")
        {
            EXPECT_EQ(output.maxSize, 0);
        }
        else if (field == "timeout")
        {
            EXPECT_EQ(output.timeout, 0);
        }
        else if (field == "expiration")
        {
            EXPECT_EQ(output.expiration, 0);
        }
        else
        {
            FAIL() << "Field " << field << " is not handled by test";
        }
    }
    else
    {
        action[field] = "";
        const std::string expectedMsg = "Invalid command format. Failed to process UploadInfo from action JSON: " + field + " field is empty";
        try
        {
            auto output = ActionsUtils::readUploadAction(action.dump(),ActionType::UPLOADFOLDER);
            FAIL() << "Didnt throw due to empty essential field: " << field;
        }
        catch (const InvalidCommandFormat& except)
        {
            EXPECT_STREQ(except.what(), expectedMsg.c_str());
        }
    }
}

class UploadFileRequiredFieldsParameterized : public ResponseActionFieldsParameterized {};

INSTANTIATE_TEST_SUITE_P(
    ActionsUtilsTests,
    UploadFileRequiredFieldsParameterized,
    ::testing::ValuesIn(uploadFileRequiredFields));

TEST_P(UploadFileRequiredFieldsParameterized, UploadFileMissingEssentialFields)
{
    auto field = GetParam();

    nlohmann::json action = getDefaultUploadObject(ActionType::UPLOADFILE);
    action.erase(field);
    const std::string expectedMsg = "Invalid command format. No '" + field + "' in UploadFile action JSON";
    try
    {
        auto output = ActionsUtils::readUploadAction(action.dump(),ActionType::UPLOADFILE);
        FAIL() << "Didnt throw due to missing essential field: " << field;
    }
    catch (const InvalidCommandFormat& except)
    {
        EXPECT_STREQ(except.what(), expectedMsg.c_str());
    }
}

TEST_P(UploadFileRequiredFieldsParameterized, UploadFileUnsetEssentialFields)
{
    auto field = GetParam();

    nlohmann::json action = getDefaultUploadObject(ActionType::UPLOADFILE);
    if (!action[field].is_string())
    {
        action[field] = 0;
        UploadInfo output;
        EXPECT_NO_THROW(output = ActionsUtils::readUploadAction(action.dump(),ActionType::UPLOADFILE));
        if (field == "maxUploadSizeBytes")
        {
            EXPECT_EQ(output.maxSize, 0);
        }
        else if (field == "timeout")
        {
            EXPECT_EQ(output.timeout, 0);
        }
        else if (field == "expiration")
        {
            EXPECT_EQ(output.expiration, 0);
        }
        else
        {
            FAIL() << "Field " << field << " is not handled by test";
        }
    }
    else
    {
        action[field] = "";
        const std::string expectedMsg = "Invalid command format. Failed to process UploadInfo from action JSON: " + field + " field is empty";
        try
        {
            auto output = ActionsUtils::readUploadAction(action.dump(),ActionType::UPLOADFILE);
            FAIL() << "Didnt throw due to empty essential field: " << field;
        }
        catch (const InvalidCommandFormat& except)
        {
            EXPECT_STREQ(except.what(), expectedMsg.c_str());
        }
    }
}

TEST_F(ActionsUtilsTests, uploadWrongTypePassword)
{
    nlohmann::json fileAction = getDefaultUploadObject(ActionType::UPLOADFILE);
    fileAction["password"] = 999;
    try
    {
        auto output = ActionsUtils::readUploadAction(fileAction.dump(), ActionType::UPLOADFILE);
        FAIL() << "Didnt throw due to wrong type";
    }
    catch (const InvalidCommandFormat& except)
    {
        EXPECT_STREQ(except.what(), "Invalid command format. Failed to process UploadInfo from action JSON: [json.exception.type_error.302] type must be string, but is number");
    }

    nlohmann::json folderAction = getDefaultUploadObject(ActionType::UPLOADFOLDER);
    folderAction["password"] = 999;

    try
    {
        auto output = ActionsUtils::readUploadAction(folderAction.dump(), ActionType::UPLOADFOLDER);
        FAIL() << "Didnt throw due to wrong type";
    }
    catch (const InvalidCommandFormat& except)
    {
        EXPECT_STREQ(except.what(), "Invalid command format. Failed to process UploadInfo from action JSON: [json.exception.type_error.302] type must be string, but is number");
    }
}

TEST_F(ActionsUtilsTests, uploadWrongTypeURL)
{
    nlohmann::json fileAction = getDefaultUploadObject(ActionType::UPLOADFILE);
    fileAction["url"] = 999;
    try
    {
        auto output = ActionsUtils::readUploadAction(fileAction.dump(), ActionType::UPLOADFILE);
        FAIL() << "Didnt throw due to wrong type";
    }
    catch (const InvalidCommandFormat& except)
    {
        EXPECT_STREQ(except.what(), "Invalid command format. Failed to process UploadInfo from action JSON: [json.exception.type_error.302] type must be string, but is number");
    }

    nlohmann::json folderAction = getDefaultUploadObject(ActionType::UPLOADFOLDER);
    folderAction["url"] = 999;

    try
    {
        auto output = ActionsUtils::readUploadAction(folderAction.dump(), ActionType::UPLOADFOLDER);
        FAIL() << "Didnt throw due to wrong type";
    }
    catch (const InvalidCommandFormat& except)
    {
        EXPECT_STREQ(except.what(), "Invalid command format. Failed to process UploadInfo from action JSON: [json.exception.type_error.302] type must be string, but is number");
    }
}

TEST_F(ActionsUtilsTests, uploadWrongTypePath)
{
    nlohmann::json fileAction = getDefaultUploadObject(ActionType::UPLOADFILE);
    fileAction["targetFile"] = 999;
    try
    {
        auto output = ActionsUtils::readUploadAction(fileAction.dump(), ActionType::UPLOADFILE);
        FAIL() << "Didnt throw due to wrong type";
    }
    catch (const InvalidCommandFormat& except)
    {
        EXPECT_STREQ(except.what(), "Invalid command format. Failed to process UploadInfo from action JSON: [json.exception.type_error.302] type must be string, but is number");
    }

    nlohmann::json folderAction = getDefaultUploadObject(ActionType::UPLOADFOLDER);
    folderAction["targetFolder"] = 999;

    try
    {
        auto output = ActionsUtils::readUploadAction(folderAction.dump(), ActionType::UPLOADFOLDER);
        FAIL() << "Didnt throw due to wrong type";
    }
    catch (const InvalidCommandFormat& except)
    {
        EXPECT_STREQ(except.what(), "Invalid command format. Failed to process UploadInfo from action JSON: [json.exception.type_error.302] type must be string, but is number");
    }
}

TEST_F(ActionsUtilsTests, uploadWrongTypeExpiration)
{
    nlohmann::json fileAction = getDefaultUploadObject(ActionType::UPLOADFILE);
    fileAction["expiration"] = "string";
    try
    {
        auto output = ActionsUtils::readUploadAction(fileAction.dump(), ActionType::UPLOADFILE);
        FAIL() << "Didnt throw due to wrong type";
    }
    catch (const InvalidCommandFormat& except)
    {
        EXPECT_STREQ(except.what(), R"(Invalid command format. Failed to process UploadInfo from action JSON: expiration is not a number: "string")");
    }

    nlohmann::json folderAction = getDefaultUploadObject(ActionType::UPLOADFOLDER);
    folderAction["expiration"] = "string";

    try
    {
        auto output = ActionsUtils::readUploadAction(folderAction.dump(), ActionType::UPLOADFOLDER);
        FAIL() << "Didnt throw due to wrong type";
    }
    catch (const InvalidCommandFormat& except)
    {
        EXPECT_STREQ(except.what(), R"(Invalid command format. Failed to process UploadInfo from action JSON: expiration is not a number: "string")");
    }
}

TEST_F(ActionsUtilsTests, uploadNegativeExpiration)
{
    nlohmann::json fileAction = getDefaultUploadObject(ActionType::UPLOADFILE);
    fileAction["expiration"] = -123456487;
    try
    {
        auto output = ActionsUtils::readUploadAction(fileAction.dump(), ActionType::UPLOADFILE);
        FAIL() << "Didnt throw due to negative value";
    }
    catch (const InvalidCommandFormat& except)
    {
        EXPECT_STREQ(except.what(), "Invalid command format. Failed to process UploadInfo from action JSON: expiration is a negative value: -123456487");
    }

    nlohmann::json folderAction = getDefaultUploadObject(ActionType::UPLOADFOLDER);
    folderAction["expiration"] = -123456487;

    try
    {
        auto output = ActionsUtils::readUploadAction(folderAction.dump(), ActionType::UPLOADFOLDER);
        FAIL() << "Didnt throw due to negative value";
    }
    catch (const InvalidCommandFormat& except)
    {
        EXPECT_STREQ(except.what(), "Invalid command format. Failed to process UploadInfo from action JSON: expiration is a negative value: -123456487");
    }
}

TEST_F(ActionsUtilsTests, uploadLargeExpiration)
{
    std::string actionFile (
        R"({"type": "sophos.mgt.action.UploadFile"
        ,"timeout": 1000
        ,"maxUploadSizeBytes": 1000
        ,"url": "https://s3.com/download.zip"
        ,"targetFile": "path"
        ,"expiration": 18446744073709551616})");
    auto resFile = ActionsUtils::readUploadAction(actionFile, ActionType::UPLOADFILE);
    EXPECT_EQ(resFile.expiration, 0);

    std::string actionFolder (
        R"({"type": "sophos.mgt.action.UploadFolder"
        ,"timeout": 1000
        ,"maxUploadSizeBytes": 1000
        ,"url": "https://s3.com/download.zip"
        ,"targetFolder": "path"
        ,"expiration": 18446744073709551616})");
    auto resFolder = ActionsUtils::readUploadAction(actionFolder, ActionType::UPLOADFOLDER);
    EXPECT_EQ(resFolder.expiration, 0);
}

TEST_F(ActionsUtilsTests, uploadWrongTypeMaxSize)
{
    nlohmann::json fileAction = getDefaultUploadObject(ActionType::UPLOADFILE);
    fileAction["maxUploadSizeBytes"] = "string";
    try
    {
        auto output = ActionsUtils::readUploadAction(fileAction.dump(), ActionType::UPLOADFILE);
        FAIL() << "Didnt throw due to wrong type";
    }
    catch (const InvalidCommandFormat& except)
    {
        EXPECT_STREQ(except.what(), R"(Invalid command format. Failed to process UploadInfo from action JSON: maxUploadSizeBytes is not a number: "string")");
    }

    nlohmann::json folderAction = getDefaultUploadObject(ActionType::UPLOADFOLDER);
    folderAction["maxUploadSizeBytes"] = "string";

    try
    {
        auto output = ActionsUtils::readUploadAction(folderAction.dump(), ActionType::UPLOADFOLDER);
        FAIL() << "Didnt throw due to wrong type";
    }
    catch (const InvalidCommandFormat& except)
    {
        EXPECT_STREQ(except.what(), R"(Invalid command format. Failed to process UploadInfo from action JSON: maxUploadSizeBytes is not a number: "string")");
    }
}

TEST_F(ActionsUtilsTests, uploadMaxSizeToLarge)
{
    nlohmann::json fileAction = getDefaultUploadObject(ActionType::UPLOADFILE);
    fileAction["maxUploadSizeBytes"] = 214748364734;
    try
    {
        auto output = ActionsUtils::readUploadAction(fileAction.dump(), ActionType::UPLOADFILE);
        FAIL() << "Didnt throw due to large value";
    }
    catch (const InvalidCommandFormat& except)
    {
        EXPECT_STREQ(except.what(), "Invalid command format. Failed to process UploadInfo from action JSON: maxUploadSizeBytes is to large: 214748364734");
    }

    nlohmann::json folderAction = getDefaultUploadObject(ActionType::UPLOADFOLDER);
    folderAction["maxUploadSizeBytes"] = 214748364734;

    try
    {
        auto output = ActionsUtils::readUploadAction(folderAction.dump(), ActionType::UPLOADFOLDER);
        FAIL() << "Didnt throw due to large value";
    }
    catch (const InvalidCommandFormat& except)
    {
        EXPECT_STREQ(except.what(), "Invalid command format. Failed to process UploadInfo from action JSON: maxUploadSizeBytes is to large: 214748364734");
    }
}

TEST_F(ActionsUtilsTests, uploadMaxSizeNegative)
{
    nlohmann::json fileAction = getDefaultUploadObject(ActionType::UPLOADFILE);
    fileAction["maxUploadSizeBytes"] = -1;
    try
    {
        auto output = ActionsUtils::readUploadAction(fileAction.dump(), ActionType::UPLOADFILE);
        FAIL() << "Didnt throw due to wrong type";
    }
    catch (const InvalidCommandFormat& except)
    {
        EXPECT_STREQ(except.what(), "Invalid command format. Failed to process UploadInfo from action JSON: maxUploadSizeBytes is a negative value: -1");
    }

    nlohmann::json folderAction = getDefaultUploadObject(ActionType::UPLOADFOLDER);
    folderAction["maxUploadSizeBytes"] = -1;

    try
    {
        auto output = ActionsUtils::readUploadAction(folderAction.dump(), ActionType::UPLOADFOLDER);
        FAIL() << "Didnt throw due to wrong type";
    }
    catch (const InvalidCommandFormat& except)
    {
        EXPECT_STREQ(except.what(), "Invalid command format. Failed to process UploadInfo from action JSON: maxUploadSizeBytes is a negative value: -1");
    }
}


TEST_F(ActionsUtilsTests, uploadWrongTypeCompress)
{
    nlohmann::json fileAction = getDefaultUploadObject(ActionType::UPLOADFILE);
    fileAction["compress"] = "string";
    try
    {
        auto output = ActionsUtils::readUploadAction(fileAction.dump(), ActionType::UPLOADFILE);
        FAIL() << "Didnt throw due to wrong type";
    }
    catch (const InvalidCommandFormat& except)
    {
        EXPECT_STREQ(except.what(), "Invalid command format. Failed to process UploadInfo from action JSON: [json.exception.type_error.302] type must be boolean, but is string");
    }

    nlohmann::json folderAction = getDefaultUploadObject(ActionType::UPLOADFOLDER);
    folderAction["compress"] = "string";

    try
    {
        auto output = ActionsUtils::readUploadAction(folderAction.dump(), ActionType::UPLOADFOLDER);
        FAIL() << "Didnt throw due to wrong type";
    }
    catch (const InvalidCommandFormat& except)
    {
        EXPECT_STREQ(except.what(), "Invalid command format. Failed to process UploadInfo from action JSON: [json.exception.type_error.302] type must be boolean, but is string");
    }
}

TEST_F(ActionsUtilsTests, uploadActionWithNoURLFieldThrowsError)
{
    nlohmann::json action = getDefaultUploadObject(ActionType::UPLOADFILE);
    action.erase("url");

    try
    {
        auto info = ActionsUtils::readUploadAction(action.dump(),ActionType::UPLOADFILE);
        FAIL() << "Should have thrown due to missing URL field in action json but didn't";
    }
    catch (const InvalidCommandFormat& ex)
    {
        EXPECT_STREQ(ex.what(), "Invalid command format. No 'url' in UploadFile action JSON");
    }
}

//**********************DOWNLOAD ACTION***************************
class DownloadRequiredFieldsParameterized : public ResponseActionFieldsParameterized {};

INSTANTIATE_TEST_SUITE_P(
    ActionsUtilsTests,
    DownloadRequiredFieldsParameterized,
    ::testing::ValuesIn(downloadRequiredFields));

TEST_P(DownloadRequiredFieldsParameterized, DownloadMissingEssentialFields)
{
    auto field = GetParam();

    nlohmann::json action = getDefaultDownloadAction();
    action.erase(field);
    const std::string expectedMsg = "Invalid command format. No '" + field + "' in Download action JSON";
    try
    {
        auto output = ActionsUtils::readDownloadAction(action.dump());
        FAIL() << "Didnt throw due to missing essential field: " << field;
    }
    catch (const InvalidCommandFormat& except)
    {
        EXPECT_STREQ(except.what(), expectedMsg.c_str());
    }
}

TEST_P(DownloadRequiredFieldsParameterized, DownloadUnsetEssentialFields)
{
    auto field = GetParam();

    nlohmann::json action = getDefaultDownloadAction();
    if (!action[field].is_string())
    {
        action[field] = 0;
        DownloadInfo output;
        if (field == "sizeBytes")
        {
            try
            {
                output = ActionsUtils::readDownloadAction(action.dump());
                FAIL() << "Didnt throw due to 0 sizeByte field: " << field;
            }
            catch (const InvalidCommandFormat& except)
            {
                EXPECT_STREQ(except.what()
                                 , "Invalid command format. sizeBytes field has been evaluated to 0. Very large values can also cause this.");
            }
        }
        else
        {
            EXPECT_NO_THROW(output = ActionsUtils::readDownloadAction(action.dump()));
            if (field == "timeout")
            {
                EXPECT_EQ(output.timeout, 0);
            }
            else if (field == "expiration")
            {
                EXPECT_EQ(output.expiration, 0);
            }
            else
            {
                FAIL() << "Field " << field << " is not handled by test";
            }
        }
    }
    else
    {
        action[field] = "";
        const std::string expectedMsg = "Invalid command format. Failed to process DownloadInfo from action JSON: " + field + " field is empty";
        try
        {
            auto output = ActionsUtils::readDownloadAction(action.dump());
            FAIL() << "Didnt throw due to empty essential field: " << field;
        }
        catch (const InvalidCommandFormat& except)
        {
            EXPECT_STREQ(except.what(), expectedMsg.c_str());
        }
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
    EXPECT_EQ(res.expiration, 0);
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
        FAIL() << "Didnt throw due to type error";
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
        FAIL() << "Didnt throw due to type error";
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
        FAIL() << "Didnt throw due to type error";
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
        FAIL() << "Didnt throw due to type error";
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
        FAIL() << "Didnt throw due to type error";
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
        FAIL() << "Didnt throw due to type error";
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
        FAIL() << "Didnt throw due to type error";
    }
    catch (const InvalidCommandFormat& except)
    {
        EXPECT_STREQ(except.what(), "Invalid command format. Failed to process DownloadInfo from action JSON: [json.exception.type_error.302] type must be string, but is number");
    }
}

TEST_F(ActionsUtilsTests, downloadWrongTypePassword)
{
    nlohmann::json action = getDefaultDownloadAction();
    action["password"] = 999;
    try
    {
        auto output = ActionsUtils::readDownloadAction(action.dump());
        FAIL() << "Didnt throw due to type error";
    }
    catch (const InvalidCommandFormat& except)
    {
        EXPECT_STREQ(except.what(), "Invalid command format. Failed to process DownloadInfo from action JSON: [json.exception.type_error.302] type must be string, but is number");
    }
}

TEST_F(ActionsUtilsTests, downloadActionWithNoURLFieldThrowsError)
{
    nlohmann::json action = getDefaultDownloadAction();
    action.erase("url");

    try
    {
        auto info = ActionsUtils::readDownloadAction(action.dump());
        FAIL() << "Should have thrown due to missing URL field in action json but didn't";
    }
    catch (const InvalidCommandFormat& ex)
    {
        EXPECT_STREQ(ex.what(), "Invalid command format. No 'url' in Download action JSON");
    }
}

//**********************RUN COMMAND ACTION***************************
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

class RunCommandRequiredFieldsParameterized : public ResponseActionFieldsParameterized {};

INSTANTIATE_TEST_SUITE_P(
    ActionsUtilsTests,
    RunCommandRequiredFieldsParameterized,
    ::testing::ValuesIn(runCommandRequiredFields));

TEST_P(RunCommandRequiredFieldsParameterized, RunCommandMissingEssentialFields)
{
    auto field = GetParam();
    nlohmann::json action = getDefaultRunCommandAction();
    action.erase(field);
    const std::string expectedMsg = "Invalid command format. No '" + field + "' in RunCommand action JSON";
    try
    {
        auto output = ActionsUtils::readCommandAction(action.dump());
        FAIL() << "Didnt throw due to missing essential field: " << field;
    }
    catch (const InvalidCommandFormat& except)
    {
        EXPECT_STREQ(except.what(), expectedMsg.c_str());
    }
}

TEST_P(RunCommandRequiredFieldsParameterized, RunCommandUnsetEssentialFields)
{
    auto field = GetParam();

    nlohmann::json action = getDefaultRunCommandAction();
    if (!action[field].is_array())
    {
        if (field == "ignoreError")
        {
            action[field].clear();
            CommandRequest output;
            EXPECT_NO_THROW(output = ActionsUtils::readCommandAction(action.dump()));
            EXPECT_EQ(output.ignoreError, false);
        }
        else
        {
            assert(action[field].is_number());
            action[field] = 0;
            CommandRequest output;
            EXPECT_NO_THROW(output = ActionsUtils::readCommandAction(action.dump()));
            if (field == "timeout")
            {
                EXPECT_EQ(output.timeout, 0);
            }
            else if (field == "expiration")
            {
                EXPECT_EQ(output.expiration, 0);
            }
            else
            {
                FAIL() << "Field " << field << " is not handled by test";
            }
        }
    }
    else
    {
        action[field] = {};
        const std::string expectedMsg = "Invalid command format. Failed to process CommandRequest from action JSON: [json.exception.type_error.302] type must be array, but is null";
        try
        {
            auto output = ActionsUtils::readCommandAction(action.dump());
            FAIL() << "Didnt throw due to empty essential field: " << field;
        }
        catch (const InvalidCommandFormat& except)
        {
            EXPECT_STREQ(except.what(), expectedMsg.c_str());
        }
    }
}

TEST_F(ActionsUtilsTests, readCommandWrongTypeCommands_NotArray)
{
    nlohmann::json action = getDefaultRunCommandAction();
    action["commands"] = "930752758";
    try
    {
        auto output = ActionsUtils::readCommandAction(action.dump());
        FAIL() << "Didnt throw due to type error";
    }
    catch (const InvalidCommandFormat& except)
    {
        EXPECT_STREQ(except.what(), "Invalid command format. Failed to process CommandRequest from action JSON: [json.exception.type_error.302] type must be array, but is string");
    }
}

TEST_F(ActionsUtilsTests, readCommandWrongTypeCommands_ArrayNumbers)
{
    nlohmann::json action = getDefaultRunCommandAction();
    action["commands"] = {93075, 2758};
    try
    {
        auto output = ActionsUtils::readCommandAction(action.dump());
        FAIL() << "Didnt throw due to type error";
    }
    catch (const InvalidCommandFormat& except)
    {
        EXPECT_STREQ(except.what(), "Invalid command format. Failed to process CommandRequest from action JSON: [json.exception.type_error.302] type must be string, but is number");
    }
}

TEST_F(ActionsUtilsTests, readCommandWrongTypeIgnoreError)
{
    nlohmann::json action = getDefaultRunCommandAction();
    action["ignoreError"] = 0;
    try
    {
        auto output = ActionsUtils::readCommandAction(action.dump());
        FAIL() << "Didnt throw due to type error";
    }
    catch (const InvalidCommandFormat& except)
    {
        EXPECT_STREQ(except.what(), "Invalid command format. Failed to process CommandRequest from action JSON: [json.exception.type_error.302] type must be boolean, but is number");
    }
}

TEST_F(ActionsUtilsTests, runCommandWrongTypeExpiration)
{
    nlohmann::json action = getDefaultRunCommandAction();
    action["expiration"] = "expiration";
    try
    {
        auto output = ActionsUtils::readCommandAction(action.dump());
        FAIL() << "Didnt throw due to type error";
    }
    catch (const InvalidCommandFormat& except)
    {
        EXPECT_STREQ(except.what(), R"(Invalid command format. Failed to process CommandRequest from action JSON: expiration is not a number: "expiration")");
    }
}

TEST_F(ActionsUtilsTests, runCommandNegativeExpiration)
{
    nlohmann::json fileAction = getDefaultRunCommandAction();
    fileAction["expiration"] = -123456487;
    try
    {
        auto output = ActionsUtils::readCommandAction(fileAction.dump());
        FAIL() << "Didnt throw due to negative value";
    }
    catch (const InvalidCommandFormat& except)
    {
        EXPECT_STREQ(except.what(), "Invalid command format. Failed to process CommandRequest from action JSON: expiration is a negative value: -123456487");
    }
}

TEST_F(ActionsUtilsTests, runCommandLargeExpiration)
{
    std::string actionFile (
        R"({"type": "sophos.mgt.action.RunCommands",
            "commands": [
                "echo one",
                "echo two > /tmp/test.txt"
            ],
            "ignoreError": true,
            "timeout": 60,
            "expiration": 18446744073709551616
        })");

    auto resFile = ActionsUtils::readCommandAction(actionFile);
    EXPECT_EQ(resFile.expiration, 0);
}

TEST_F(ActionsUtilsTests, readCommandFailsDueToMalformedJson)
{
    std::string commandJson = "this is not json";
    try
    {
        auto output = ActionsUtils::readCommandAction(commandJson);
        FAIL() << "Didnt throw due to incorrect format";
    }
    catch (const InvalidCommandFormat& except)
    {
        EXPECT_STREQ(except.what(), "Invalid command format. Cannot parse RunCommand action with JSON error: [json.exception.parse_error.101] "
                                    "parse error at line 1, column 2: syntax error while parsing value - invalid literal; last read: 'th'");
    }
}

TEST_F(ActionsUtilsTests, readCommandFailsDueToEmptyJsonString)
{
    std::string commandJson = "";
    try
    {
        auto output = ActionsUtils::readCommandAction(commandJson);
        FAIL() << "Didnt throw due to empty json string";
    }
    catch (const InvalidCommandFormat& except)
    {
        EXPECT_STREQ(
            except.what(), "Invalid command format. RunCommand action JSON is empty");
    }
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

    try
    {
        auto output = ActionsUtils::readCommandAction(commandJson);
        FAIL() << "Didnt throw due to empty commands list";
    }
    catch (const InvalidCommandFormat& except)
    {
        EXPECT_STREQ(
            except.what(), "Invalid command format. Failed to process CommandRequest from action JSON: commands field is empty");
    }
}

TEST_F(ActionsUtilsTests, readCommandFailsDueToEmptyJsonObject)
{
    std::string commandJson = "{}";

    try
    {
        auto output = ActionsUtils::readCommandAction(commandJson);
        FAIL() << "Didnt throw due to empty json object";
    }
    catch (const InvalidCommandFormat& except)
    {
        EXPECT_STREQ(
            except.what(), "Invalid command format. No 'commands' in RunCommand action JSON");
    }
}