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

TEST_F(ActionsUtilsTests, testExpiry)
{
    EXPECT_TRUE(ActionsUtils::isExpired(1000));
    // time is set here to Tue 8 Feb 17:12:46 GMT 2033
    EXPECT_FALSE(ActionsUtils::isExpired(1991495566));
    Common::UtilityImpl::FormattedTime time;
    u_int64_t currentTime = time.currentEpochTimeInSecondsAsInteger();
    EXPECT_TRUE(ActionsUtils::isExpired(currentTime-10));
    EXPECT_FALSE(ActionsUtils::isExpired(currentTime+10));
}

TEST_F(ActionsUtilsTests, testSucessfulParseUploadFile)
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

TEST_F(ActionsUtilsTests, testSucessfulParseUploadFolder)
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

TEST_F(ActionsUtilsTests, testReadFailsWhenActionIsInvalidJson)
{
    EXPECT_THROW(
        std::ignore = ActionsUtils::readUploadAction("", ActionType::UPLOADFILE),
        InvalidCommandFormat);
}

TEST_F(ActionsUtilsTests, testReadFailsWhenActionISUploadFileWhenExpectingFolder)
{
    nlohmann::json action = getDefaultUploadObject(ActionType::UPLOADFILE);
    EXPECT_THROW(
        std::ignore = ActionsUtils::readUploadAction(action.dump(), ActionType::UPLOADFOLDER),
        InvalidCommandFormat);
}

TEST_F(ActionsUtilsTests, testReadFailsWhenActionISUploadFolderWhenExpectingFile)
{
    nlohmann::json action = getDefaultUploadObject(ActionType::UPLOADFOLDER);
    EXPECT_THROW(
        std::ignore = ActionsUtils::readUploadAction(action.dump(), ActionType::UPLOADFILE),
        InvalidCommandFormat);
}

TEST_F(ActionsUtilsTests, testSucessfulParseCompressionEnabled)
{
    nlohmann::json action = getDefaultUploadObject(ActionType::UPLOADFILE);
    action["compress"] = true;
    action["password"] = "password";
    UploadInfo info =
        ActionsUtils::readUploadAction(action.dump(), ActionType::UPLOADFILE);

    EXPECT_EQ(info.compress, true);
    EXPECT_EQ(info.password, "password");
}

TEST_F(ActionsUtilsTests, testFailedParseInvalidValue)
{
    nlohmann::json action = getDefaultUploadObject(ActionType::UPLOADFILE);
    action["url"] = 1000;
    EXPECT_THROW(std::ignore = ActionsUtils::readUploadAction(action.dump(),ActionType::UPLOADFILE),InvalidCommandFormat);
}

TEST_F(ActionsUtilsTests, testFailedParseMissingUrl)
{
    nlohmann::json action = getDefaultUploadObject(ActionType::UPLOADFILE);
    action.erase("url");
    EXPECT_THROW(std::ignore = ActionsUtils::readUploadAction(action.dump(),ActionType::UPLOADFILE),InvalidCommandFormat);
}

TEST_F(ActionsUtilsTests, testFailedParseMissingExpiration)
{
    nlohmann::json action = getDefaultUploadObject(ActionType::UPLOADFILE);
    action.erase("expiration");
    EXPECT_THROW(std::ignore = ActionsUtils::readUploadAction(action.dump(),ActionType::UPLOADFILE),InvalidCommandFormat);
}

TEST_F(ActionsUtilsTests, testFailedParseMissingTimeout)
{
    nlohmann::json action = getDefaultUploadObject(ActionType::UPLOADFILE);
    action.erase("timeout");
    EXPECT_THROW(std::ignore = ActionsUtils::readUploadAction(action.dump(),ActionType::UPLOADFILE),InvalidCommandFormat);
}

TEST_F(ActionsUtilsTests, testFailedParseMissingMaxUploadSizeBytes)
{
    nlohmann::json action = getDefaultUploadObject(ActionType::UPLOADFILE);
    action.erase("maxUploadSizeBytes");
    EXPECT_THROW(std::ignore = ActionsUtils::readUploadAction(action.dump(),ActionType::UPLOADFILE),InvalidCommandFormat);
}

TEST_F(ActionsUtilsTests, testFailedParseMissingTargetFile)
{
    nlohmann::json action = getDefaultUploadObject(ActionType::UPLOADFILE);
    action.erase("targetFile");
    EXPECT_THROW(std::ignore = ActionsUtils::readUploadAction(action.dump(),ActionType::UPLOADFILE),InvalidCommandFormat);
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
TEST_F(ActionsUtilsTests, downloadActionTestMissingUrl)
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

TEST_F(ActionsUtilsTests, downloadActionTestMissingtargetPath)
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

TEST_F(ActionsUtilsTests, downloadActionTestMissingsha256)
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

TEST_F(ActionsUtilsTests, downloadActionTestMissingtimeout)
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

TEST_F(ActionsUtilsTests, downloadActionTestMissingsizeBytes)
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

TEST_F(ActionsUtilsTests, downloadActionTestMissingexpiration)
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

TEST_F(ActionsUtilsTests, downloadActionTestMissingpassword)
{
    nlohmann::json action = getDefaultDownloadAction();
    action.erase("password");
    EXPECT_NO_THROW(std::ignore = ActionsUtils::readDownloadAction(action.dump()));
}

TEST_F(ActionsUtilsTests, downloadActionTestMissingdecompress)
{
    nlohmann::json action = getDefaultDownloadAction();
    action.erase("decompress");
    EXPECT_NO_THROW(std::ignore = ActionsUtils::readDownloadAction(action.dump()));
}

TEST_F(ActionsUtilsTests, downloadActionTestSuccessfulParsing)
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

TEST_F(ActionsUtilsTests, downloadActionHugeurl)
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

TEST_F(ActionsUtilsTests, downloadActionLargeTargetPath)
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

TEST_F(ActionsUtilsTests, downloadActionTestWrongTypeDecompress)
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
        EXPECT_STREQ(except.what(), "Invalid command format. Failed to parse download command json, json value in unexpected type: [json.exception.type_error.302] type must be boolean, but is string");
    }
}

TEST_F(ActionsUtilsTests, downloadActionTestWrongTypeSizeBytes)
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
        EXPECT_STREQ(except.what(), "Invalid command format. Failed to parse download command json, json value in unexpected type: [json.exception.type_error.302] type must be number, but is string");
    }
}

TEST_F(ActionsUtilsTests, downloadActionTestWrongTypeExpiration)
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
        EXPECT_STREQ(except.what(), "Invalid command format. Failed to parse download command json, json value in unexpected type: [json.exception.type_error.302] type must be number, but is string");
    }
}

TEST_F(ActionsUtilsTests, downloadActionTestWrongTypeTimeout)
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
        EXPECT_STREQ(except.what(), "Invalid command format. Failed to parse download command json, json value in unexpected type: [json.exception.type_error.302] type must be number, but is string");
    }
}

TEST_F(ActionsUtilsTests, downloadActionTestWrongTypeSHA256)
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
        EXPECT_STREQ(except.what(), "Invalid command format. Failed to parse download command json, json value in unexpected type: [json.exception.type_error.302] type must be string, but is number");
    }
}

TEST_F(ActionsUtilsTests, downloadActionTestWrongTypeURL)
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
        EXPECT_STREQ(except.what(), "Invalid command format. Failed to parse download command json, json value in unexpected type: [json.exception.type_error.302] type must be string, but is number");
    }
}

TEST_F(ActionsUtilsTests, downloadActionTestWrongTypeTargetPath)
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
        EXPECT_STREQ(except.what(), "Invalid command format. Failed to parse download command json, json value in unexpected type: [json.exception.type_error.302] type must be string, but is number");
    }
}

TEST_F(ActionsUtilsTests, downloadActionTestEmptyTargetPath)
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
        EXPECT_STREQ(except.what(), "Invalid command format. Target Path field is empty");
    }
}

TEST_F(ActionsUtilsTests, downloadActionTestEmptysha256)
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
        EXPECT_STREQ(except.what(), "Invalid command format. sha256 field is empty");
    }
}

TEST_F(ActionsUtilsTests, downloadActionTestEmptyurl)
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
        EXPECT_STREQ(except.what(), "Invalid command format. url field is empty");
    }
}


TEST_F(ActionsUtilsTests, downloadActionTestWrongTypePassword)
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
        EXPECT_STREQ(except.what(), "Invalid command format. Failed to parse download command json, json value in unexpected type: [json.exception.type_error.302] type must be string, but is number");
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
TEST_F(ActionsUtilsTests, testReadCommandActionSuccess)
{
    std::string commandJson = getDefaultCommandJsonString();
    auto command = ActionsUtils::readCommandAction(commandJson);
    std::vector<std::string> expectedCommands = { { "echo one" }, { R"(echo two > /tmp/test.txt)" } };

    EXPECT_EQ(command.ignoreError, true);
    EXPECT_EQ(command.timeout, 60);
    EXPECT_EQ(command.expiration, 144444000000004);
    EXPECT_EQ(command.commands, expectedCommands);
}

TEST_F(ActionsUtilsTests, testReadCommandHandlesEscapedCharsInCmd)
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

TEST_F(ActionsUtilsTests, testReadCommandFailsDueToMissingTypeRequiredField)
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

TEST_F(ActionsUtilsTests, testReadCommandFailsDueToMissingCommandsRequiredField)
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

TEST_F(ActionsUtilsTests, testReadCommandFailsDueToMissingIgnoreErrorRequiredField)
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

TEST_F(ActionsUtilsTests, testReadCommandFailsDueToMissingTimeoutRequiredField)
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

TEST_F(ActionsUtilsTests, testReadCommandFailsDueToMissingExpirationRequiredField)
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



TEST_F(ActionsUtilsTests, testReadCommandFailsDueToTypeError)
{
    std::string commandJson = getCommandJsonStringWithWrongValueType();
    EXPECT_THAT([&]() { std::ignore = ActionsUtils::readCommandAction(commandJson); },
                ThrowsMessage<InvalidCommandFormat>(HasSubstr("Failed to create Command Request object from run command JSON")));
}

TEST_F(ActionsUtilsTests, testReadCommandFailsDueToMalformedJson)
{
    std::string commandJson = "this is not json";
    EXPECT_THAT([&]() { std::ignore = ActionsUtils::readCommandAction(commandJson); },
                ThrowsMessage<InvalidCommandFormat>(HasSubstr("syntax error")));
}

TEST_F(ActionsUtilsTests, testReadCommandFailsDueToEmptyJsonString)
{
    std::string commandJson = "";
    EXPECT_THAT([&]() { std::ignore = ActionsUtils::readCommandAction(commandJson); },
                    ThrowsMessage<InvalidCommandFormat>(HasSubstr("Run Command action JSON is empty")));
}

TEST_F(ActionsUtilsTests, testReadCommandFailsDueToMissingCommands)
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

TEST_F(ActionsUtilsTests, testReadCommandFailsDueToEmptyJsonObject)
{
    std::string commandJson = "{}";
    EXPECT_THAT([&]() { std::ignore = ActionsUtils::readCommandAction(commandJson); },
                    ThrowsMessage<InvalidCommandFormat>(HasSubstr("Invalid command format. No 'type' in Run Command action JSON")));
}