// Copyright 2023 Sophos Limited. All rights reserved.

#include "modules/ResponseActions/ResponseActionsImpl/UploadFileAction.h"

#include "modules/Common/UtilityImpl/TimeUtils.h"
#include "modules/Common/FileSystem/IFileTooLargeException.h"

#include <tests/Common/Helpers/FileSystemReplaceAndRestore.h>
#include <tests/Common/Helpers/MockHttpRequester.h>
#include <tests/Common/Helpers/MockFileSystem.h>
#include "tests/Common/Helpers/MemoryAppender.h"

#include <json.hpp>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

class UploadFileTests : public MemoryAppenderUsingTests
{
public:
    UploadFileTests()
        : MemoryAppenderUsingTests("responseactions")
    {}
    virtual void TearDown() { Tests::restoreFileSystem(); }
    nlohmann::json getDefaultUploadObject()
    {
        nlohmann::json action;
        action["targetFile"] = "path";
        action["url"] = "https://s3.com/somewhere";
        action["compress"] = false;
        action["password"] = "";
        action["expiration"] = 144444000000004;
        action["timeout"] = 10;
        action["maxUploadSizeBytes"] = 1000;
        return action;
    }
};

TEST_F(UploadFileTests, SucessCase)
{
    auto httpRequester = std::make_shared<StrictMock<MockHTTPRequester>>();
    Common::HttpRequests::Response httpresponse;
    httpresponse.status = Common::HttpRequests::HTTP_STATUS_OK;
    httpresponse.errorCode = Common::HttpRequests::ResponseErrorCode::OK;
    EXPECT_CALL(*httpRequester, put(_)).WillOnce(Return(httpresponse));
    ResponseActionsImpl::UploadFileAction uploadFileAction(httpRequester);
    nlohmann::json action = getDefaultUploadObject();

    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>{ mockFileSystem });
    EXPECT_CALL(*mockFileSystem, isFile("path")).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, fileSize("path")).WillOnce(Return(100));
    EXPECT_CALL(*mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256,"path")).WillOnce(Return("sha256string"));

    std::string response = uploadFileAction.run(action.dump());
    nlohmann::json responseJson = nlohmann::json::parse(response);
    EXPECT_EQ(responseJson["result"],0);
    EXPECT_EQ(responseJson["httpStatus"],200);
}

TEST_F(UploadFileTests, cannotParseActions)
{
    auto httpRequester = std::make_shared<StrictMock<MockHTTPRequester>>();
    ResponseActionsImpl::UploadFileAction uploadFileAction(httpRequester);

    std::string response = uploadFileAction.run("");
    nlohmann::json responseJson = nlohmann::json::parse(response);
    EXPECT_EQ(responseJson["result"],1);
    EXPECT_EQ(responseJson["errorType"],"invalid_path");
    EXPECT_EQ(responseJson["errorMessage"],"Error parsing command from Central");
}

TEST_F(UploadFileTests, actionExipired)
{
    auto httpRequester = std::make_shared<StrictMock<MockHTTPRequester>>();
    ResponseActionsImpl::UploadFileAction uploadFileAction(httpRequester);
    nlohmann::json action = getDefaultUploadObject();
    action["expiration"] = 0;
    std::string response = uploadFileAction.run(action.dump());
    nlohmann::json responseJson = nlohmann::json::parse(response);
    EXPECT_EQ(responseJson["result"],4);
    EXPECT_EQ(responseJson["errorMessage"],"Action has expired");
}

TEST_F(UploadFileTests, FileDoesNotExist)
{
    auto httpRequester = std::make_shared<StrictMock<MockHTTPRequester>>();
    ResponseActionsImpl::UploadFileAction uploadFileAction(httpRequester);
    nlohmann::json action = getDefaultUploadObject();

    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>{ mockFileSystem });
    EXPECT_CALL(*mockFileSystem, isFile("path")).WillOnce(Return(false));

    std::string response = uploadFileAction.run(action.dump());
    nlohmann::json responseJson = nlohmann::json::parse(response);
    EXPECT_EQ(responseJson["result"],1);
    EXPECT_EQ(responseJson["errorType"],"invalid_path");
    EXPECT_EQ(responseJson["errorMessage"],"path is not a file");
}

TEST_F(UploadFileTests, FileOverSizeLimit)
{
    auto httpRequester = std::make_shared<StrictMock<MockHTTPRequester>>();
    ResponseActionsImpl::UploadFileAction uploadFileAction(httpRequester);
    nlohmann::json action = getDefaultUploadObject();

    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>{ mockFileSystem });
    EXPECT_CALL(*mockFileSystem, isFile("path")).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, fileSize("path")).WillOnce(Return(10000));

    std::string response = uploadFileAction.run(action.dump());
    nlohmann::json responseJson = nlohmann::json::parse(response);
    EXPECT_EQ(responseJson["result"],1);
    EXPECT_EQ(responseJson["errorType"],"exceed_size_limit");
    EXPECT_EQ(responseJson["errorMessage"],"File at path path is size 10000 bytes which is above the size limit 1000 bytes");
}

TEST_F(UploadFileTests, FileBeingWrittenToAndOverSizeLimit)
{
    auto httpRequester = std::make_shared<StrictMock<MockHTTPRequester>>();
    ResponseActionsImpl::UploadFileAction uploadFileAction(httpRequester);
    nlohmann::json action = getDefaultUploadObject();

    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>{ mockFileSystem });
    EXPECT_CALL(*mockFileSystem, isFile("path")).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, fileSize("path")).WillOnce(Return(100));
    EXPECT_CALL(*mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256,"path")).WillOnce(Throw(Common::FileSystem::IFileSystemException("")));

    std::string response = uploadFileAction.run(action.dump());
    nlohmann::json responseJson = nlohmann::json::parse(response);
    EXPECT_EQ(responseJson["result"],1);
    EXPECT_EQ(responseJson["errorType"],"access_denied");
    EXPECT_EQ(responseJson["errorMessage"],"File to be uploaded cannot be accessed");
}

