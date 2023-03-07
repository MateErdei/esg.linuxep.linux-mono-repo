// Copyright 2023 Sophos Limited. All rights reserved.

#include "modules/Common/FileSystem/IFileTooLargeException.h"
#include "modules/Common/ZipUtilities/ZipUtils.h"
#include "modules/ResponseActions/ResponseActionsImpl/UploadFileAction.h"
#include "tests/Common/ApplicationConfiguration/MockedApplicationPathManager.h"
#include "tests/Common/Helpers/FileSystemReplaceAndRestore.h"
#include "tests/Common/Helpers/MemoryAppender.h"
#include "tests/Common/Helpers/MockFileSystem.h"
#include "tests/Common/Helpers/MockHttpRequester.h"
#include "tests/Common/Helpers/MockZipUtils.h"
#include "tests/Common/Helpers/TempDir.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <json.hpp>
class UploadFileTests : public MemoryAppenderUsingTests
{
public:
    UploadFileTests() : MemoryAppenderUsingTests("responseactions") {}
    virtual void TearDown()
    {
        Tests::restoreFileSystem();
        Common::ZipUtilities::restoreZipUtils();
        Common::ApplicationConfiguration::restoreApplicationPathManager();
    }
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

TEST_F(UploadFileTests, SuccessCase)
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
    EXPECT_CALL(*mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    EXPECT_CALL(*mockFileSystem, isFile("path")).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, fileSize("path")).WillOnce(Return(100));
    EXPECT_CALL(*mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, "path"))
        .WillOnce(Return("sha256string"));

    std::string response = uploadFileAction.run(action.dump());
    nlohmann::json responseJson = nlohmann::json::parse(response);
    EXPECT_EQ(responseJson["result"], 0);
    EXPECT_EQ(responseJson["httpStatus"], 200);
}

TEST_F(UploadFileTests, SuccessCaseWithProxy)
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
    EXPECT_CALL(*mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(true));
    std::string obfuscatedCreds =
        "CCD4E57ZjW+t5XPiMSJH1TurG3MfWCN3DpjJRINMwqNaWl+3zzlVIdyVmifCHUwcmaX6+YTSyyBM8SslIIGV5rUw";
    std::string content = R"({"proxy":"localhost","credentials":")" + obfuscatedCreds + R"("})";
    EXPECT_CALL(*mockFileSystem, readFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy"))
        .WillOnce(Return(content));
    EXPECT_CALL(*mockFileSystem, isFile("path")).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, fileSize("path")).WillOnce(Return(100));
    EXPECT_CALL(*mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, "path"))
        .WillOnce(Return("sha256string"));

    std::string response = uploadFileAction.run(action.dump());
    nlohmann::json responseJson = nlohmann::json::parse(response);
    EXPECT_EQ(responseJson["result"], 0);
    EXPECT_EQ(responseJson["httpStatus"], 200);
}

TEST_F(UploadFileTests, ProxyFailureFallsbackDirect)
{
    auto httpRequester = std::make_shared<StrictMock<MockHTTPRequester>>();
    Common::HttpRequests::Response failhttpresponse;
    Common::HttpRequests::Response httpresponse;
    httpresponse.status = Common::HttpRequests::HTTP_STATUS_OK;
    httpresponse.errorCode = Common::HttpRequests::ResponseErrorCode::OK;
    failhttpresponse.status = 400;
    failhttpresponse.errorCode = Common::HttpRequests::ResponseErrorCode::OK;
    EXPECT_CALL(*httpRequester, put(_)).Times(2).WillOnce(Return(failhttpresponse)).WillOnce(Return(httpresponse));

    ResponseActionsImpl::UploadFileAction uploadFileAction(httpRequester);
    nlohmann::json action = getDefaultUploadObject();

    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>{ mockFileSystem });
    EXPECT_CALL(*mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(true));
    std::string obfuscatedCreds =
        "CCD4E57ZjW+t5XPiMSJH1TurG3MfWCN3DpjJRINMwqNaWl+3zzlVIdyVmifCHUwcmaX6+YTSyyBM8SslIIGV5rUw";
    std::string content = R"({"proxy":"localhost","credentials":")" + obfuscatedCreds + R"("})";
    EXPECT_CALL(*mockFileSystem, readFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy"))
        .WillOnce(Return(content));
    EXPECT_CALL(*mockFileSystem, isFile("path")).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, fileSize("path")).WillOnce(Return(100));
    EXPECT_CALL(*mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, "path"))
        .WillOnce(Return("sha256string"));

    std::string response = uploadFileAction.run(action.dump());
    nlohmann::json responseJson = nlohmann::json::parse(response);
    EXPECT_EQ(responseJson["result"], 0);
    EXPECT_EQ(responseJson["httpStatus"], 200);
}

TEST_F(UploadFileTests, SuccessCaseCompressionWithPassword)
{
    // TOUCHES FILE SYSTEM
    auto httpRequester = std::make_shared<StrictMock<MockHTTPRequester>>();
    Common::HttpRequests::Response httpresponse;
    httpresponse.status = Common::HttpRequests::HTTP_STATUS_OK;
    httpresponse.errorCode = Common::HttpRequests::ResponseErrorCode::OK;
    EXPECT_CALL(*httpRequester, put(_)).WillOnce(Return(httpresponse));

    Tests::TempDir tempDir;
    tempDir.makeDirs("test");
    tempDir.createFile("test/file.txt", "stuff");
    std::string filePath = tempDir.absPath("test/file.txt");
    std::string zipFile = tempDir.absPath("file.txt.zip");

    ResponseActionsImpl::UploadFileAction uploadFileAction(httpRequester);
    nlohmann::json action = getDefaultUploadObject();
    action["compress"] = true;
    action["password"] = "password";
    action["targetFile"] = filePath;

    MockedApplicationPathManager* mockAppManager = new NiceMock<MockedApplicationPathManager>();
    MockedApplicationPathManager& mock(*mockAppManager);
    ON_CALL(mock, getResponseActionTmpPath()).WillByDefault(Return(tempDir.absPath("")));
    ON_CALL(mock, getMcsCurrentProxyFilePath())
        .WillByDefault(Return("/opt/sophos-spl/base/etc/sophosspl/current_proxy"));

    Common::ApplicationConfiguration::replaceApplicationPathManager(
        std::unique_ptr<Common::ApplicationConfiguration::IApplicationPathManager>(mockAppManager));
    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>{ mockFileSystem });
    EXPECT_CALL(*mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    EXPECT_CALL(*mockFileSystem, isFile(filePath)).Times(3).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockFileSystem, readFile(filePath, _)).WillOnce(Return("stuff"));

    EXPECT_CALL(*mockFileSystem, isFile(zipFile)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, removeFile(zipFile)).Times(1).WillOnce(Return());
    EXPECT_CALL(*mockFileSystem, fileSize(zipFile)).WillOnce(Return(8));
    EXPECT_CALL(*mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, zipFile))
        .WillOnce(Return("sha256string"));

    std::string response = uploadFileAction.run(action.dump());
    nlohmann::json responseJson = nlohmann::json::parse(response);
    EXPECT_EQ(responseJson["result"], 0);
    EXPECT_EQ(responseJson["httpStatus"], 200);
    EXPECT_TRUE(std::filesystem::is_regular_file(zipFile));
    std::string unpack = tempDir.absPath("file.txt");
    int ret = Common::ZipUtilities::zipUtils().unzip(zipFile, tempDir.absPath(""), true, "password");
    EXPECT_EQ(0, ret);

    EXPECT_TRUE(std::filesystem::is_regular_file(unpack));
    auto ss = std::ostringstream{};
    std::ifstream file(unpack);
    ss << file.rdbuf();
    EXPECT_EQ("stuff", ss.str());
}

TEST_F(UploadFileTests, ZipFails)
{
    auto httpRequester = std::make_shared<StrictMock<MockHTTPRequester>>();
    Common::HttpRequests::Response httpresponse;
    httpresponse.status = Common::HttpRequests::HTTP_STATUS_OK;
    httpresponse.errorCode = Common::HttpRequests::ResponseErrorCode::OK;

    ResponseActionsImpl::UploadFileAction uploadFileAction(httpRequester);
    nlohmann::json action = getDefaultUploadObject();
    action["compress"] = true;
    std::string zipFile = "/opt/sophos-spl/plugins/responseactions/tmp/path.zip";

    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>{ mockFileSystem });
    EXPECT_CALL(*mockFileSystem, isFile("path")).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, isFile(zipFile)).Times(1).WillOnce(Return(true));

    EXPECT_CALL(*mockFileSystem, removeFile(zipFile)).Times(1).WillOnce(Return());

    auto mockZip = std::make_unique<NiceMock<MockZipUtils>>();
    MockZipUtils& mockzipUtil(*mockZip);
    ON_CALL(mockzipUtil, zip(_, _, _)).WillByDefault(Return(1));
    Common::ZipUtilities::replaceZipUtils(std::move(mockZip));

    std::string response = uploadFileAction.run(action.dump());
    nlohmann::json responseJson = nlohmann::json::parse(response);
    EXPECT_EQ(responseJson["result"], 3);
    EXPECT_EQ(responseJson["errorMessage"], "Error zipping path");
}

TEST_F(UploadFileTests, ZipFailsDueToLargeFile)
{
    auto httpRequester = std::make_shared<StrictMock<MockHTTPRequester>>();
    Common::HttpRequests::Response httpresponse;
    httpresponse.status = Common::HttpRequests::HTTP_STATUS_OK;
    httpresponse.errorCode = Common::HttpRequests::ResponseErrorCode::OK;

    ResponseActionsImpl::UploadFileAction uploadFileAction(httpRequester);
    nlohmann::json action = getDefaultUploadObject();
    action["compress"] = true;
    std::string zipFile = "/opt/sophos-spl/plugins/responseactions/tmp/path.zip";

    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>{ mockFileSystem });
    EXPECT_CALL(*mockFileSystem, isFile("path")).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, isFile(zipFile)).Times(1).WillOnce(Return(true));

    EXPECT_CALL(*mockFileSystem, removeFile(zipFile)).Times(1).WillOnce(Return());

    auto mockZip = std::make_unique<NiceMock<MockZipUtils>>();
    MockZipUtils& mockzipUtil(*mockZip);
    ON_CALL(mockzipUtil, zip(_, _, _)).WillByDefault(Throw(std::runtime_error("")));
    Common::ZipUtilities::replaceZipUtils(std::move(mockZip));

    std::string response = uploadFileAction.run(action.dump());
    nlohmann::json responseJson = nlohmann::json::parse(response);
    EXPECT_EQ(responseJson["result"], 3);
    EXPECT_EQ(responseJson["errorMessage"], "Error zipping path");
}

TEST_F(UploadFileTests, cannotParseActions)
{
    auto httpRequester = std::make_shared<StrictMock<MockHTTPRequester>>();
    ResponseActionsImpl::UploadFileAction uploadFileAction(httpRequester);

    std::string response = uploadFileAction.run("");
    nlohmann::json responseJson = nlohmann::json::parse(response);
    EXPECT_EQ(responseJson["result"], 1);
    EXPECT_EQ(responseJson["errorMessage"], "Error parsing command from Central");
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

TEST_F(UploadFileTests, FailureDueToTimeout)
{
    auto httpRequester = std::make_shared<StrictMock<MockHTTPRequester>>();
    Common::HttpRequests::Response httpresponse;
    httpresponse.status = 500;
    httpresponse.errorCode = Common::HttpRequests::ResponseErrorCode::TIMEOUT;
    EXPECT_CALL(*httpRequester, put(_)).WillOnce(Return(httpresponse));
    ResponseActionsImpl::UploadFileAction uploadFileAction(httpRequester);
    nlohmann::json action = getDefaultUploadObject();

    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>{ mockFileSystem });
    EXPECT_CALL(*mockFileSystem, isFile("path")).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, fileSize("path")).WillOnce(Return(100));
    EXPECT_CALL(*mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, "path"))
        .WillOnce(Return("sha256string"));
    EXPECT_CALL(*mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    std::string response = uploadFileAction.run(action.dump());
    nlohmann::json responseJson = nlohmann::json::parse(response);
    EXPECT_EQ(responseJson["result"], 2);
    EXPECT_EQ(responseJson["httpStatus"], 500);
    EXPECT_EQ(responseJson["errorMessage"], "Timeout Uploading file: path");
}

TEST_F(UploadFileTests, FailureDueToNetworkError)
{
    auto httpRequester = std::make_shared<StrictMock<MockHTTPRequester>>();
    Common::HttpRequests::Response httpresponse;
    httpresponse.status = 500;
    httpresponse.errorCode = Common::HttpRequests::ResponseErrorCode::FAILED;
    httpresponse.error = "failure";
    EXPECT_CALL(*httpRequester, put(_)).WillOnce(Return(httpresponse));
    ResponseActionsImpl::UploadFileAction uploadFileAction(httpRequester);
    nlohmann::json action = getDefaultUploadObject();

    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>{ mockFileSystem });
    EXPECT_CALL(*mockFileSystem, isFile("path")).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, fileSize("path")).WillOnce(Return(100));
    EXPECT_CALL(*mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, "path"))
        .WillOnce(Return("sha256string"));
    EXPECT_CALL(*mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));

    std::string response = uploadFileAction.run(action.dump());
    nlohmann::json responseJson = nlohmann::json::parse(response);
    EXPECT_EQ(responseJson["result"], 1);
    EXPECT_EQ(responseJson["httpStatus"], 500);
    EXPECT_EQ(responseJson["errorMessage"], "Failed to upload file: path with error: failure");
    EXPECT_EQ(responseJson["errorType"], "network_error");
}

TEST_F(UploadFileTests, FailureDueToServerError)
{
    auto httpRequester = std::make_shared<StrictMock<MockHTTPRequester>>();
    Common::HttpRequests::Response httpresponse;
    httpresponse.status = 500;
    httpresponse.errorCode = Common::HttpRequests::ResponseErrorCode::OK;
    EXPECT_CALL(*httpRequester, put(_)).WillOnce(Return(httpresponse));
    ResponseActionsImpl::UploadFileAction uploadFileAction(httpRequester);
    nlohmann::json action = getDefaultUploadObject();

    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>{ mockFileSystem });
    EXPECT_CALL(*mockFileSystem, isFile("path")).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, fileSize("path")).WillOnce(Return(100));
    EXPECT_CALL(*mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, "path"))
        .WillOnce(Return("sha256string"));
    EXPECT_CALL(*mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));

    std::string response = uploadFileAction.run(action.dump());
    nlohmann::json responseJson = nlohmann::json::parse(response);
    EXPECT_EQ(responseJson["result"], 1);
    EXPECT_EQ(responseJson["httpStatus"], 500);
    EXPECT_EQ(responseJson["errorMessage"], "Failed to upload file: path with http error code 500");
    EXPECT_EQ(responseJson["errorType"], "network_error");
}
