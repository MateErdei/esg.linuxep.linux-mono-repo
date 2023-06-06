// Copyright 2023 Sophos Limited. All rights reserved.

#include "modules/Common/FileSystem/IFileTooLargeException.h"
#include "modules/Common/ZipUtilities/ZipUtils.h"
#include "modules/ResponseActions/ResponseActionsImpl/UploadFolderAction.h"
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
class UploadFolderTests : public MemoryAppenderUsingTests
{
public:
    UploadFolderTests() : MemoryAppenderUsingTests("responseactions") {}
    virtual void TearDown()
    {
        Tests::restoreFileSystem();
        Common::ApplicationConfiguration::restoreApplicationPathManager();
        Common::ZipUtilities::restoreZipUtils();
    }
    nlohmann::json getDefaultUploadObject()
    {
        nlohmann::json action;
        action["targetFolder"] = "/tmp/path";
        action["url"] = "https://s3.com/somewhere";
        action["password"] = "";
        action["expiration"] = 144444000000004;
        action["timeout"] = 10;
        action["maxUploadSizeBytes"] = 1000;
        return action;
    }
    std::string defaultZipFile = "/opt/sophos-spl/plugins/responseactions/tmp/path.zip";
};

TEST_F(UploadFolderTests, SuccessCaseWithPassword)
{
    // TOUCHES FILE SYSTEM
    auto httpRequester = std::make_shared<StrictMock<MockHTTPRequester>>();
    Common::HttpRequests::Response httpresponse;
    httpresponse.status = Common::HttpRequests::HTTP_STATUS_OK;
    httpresponse.errorCode = Common::HttpRequests::ResponseErrorCode::OK;
    EXPECT_CALL(*httpRequester, put(_)).WillOnce(Return(httpresponse));

    Tests::TempDir tempDir;
    tempDir.makeDirs("test");
    std::string folderPath = tempDir.absPath("test/");
    std::string zipFile = tempDir.absPath("test.zip");

    ResponseActionsImpl::UploadFolderAction uploadFolderAction(httpRequester);
    nlohmann::json action = getDefaultUploadObject();
    action["password"] = "password";
    action["targetFolder"] = folderPath;

    MockedApplicationPathManager* mockAppManager = new NiceMock<MockedApplicationPathManager>();
    MockedApplicationPathManager& mock(*mockAppManager);
    ON_CALL(mock, getResponseActionTmpPath()).WillByDefault(Return(tempDir.absPath("")));
    Common::ApplicationConfiguration::replaceApplicationPathManager(
        std::unique_ptr<Common::ApplicationConfiguration::IApplicationPathManager>(mockAppManager));

    auto mockZip = std::make_unique<NiceMock<MockZipUtils>>();
    MockZipUtils& mockzipUtil(*mockZip);
    ON_CALL(mockzipUtil, zip(_, _, _, _, _)).WillByDefault(Return(0));
    Common::ZipUtilities::replaceZipUtils(std::move(mockZip));

    auto mockFileSystem = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockFileSystem, isDirectory(folderPath)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, removeFile(zipFile)).Times(1).WillOnce(Return());
    EXPECT_CALL(*mockFileSystem, isFile(zipFile)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, isFile("")).WillOnce(Return(false));
    EXPECT_CALL(*mockFileSystem, fileSize(zipFile)).WillOnce(Return(8));
    EXPECT_CALL(*mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, zipFile))
        .WillOnce(Return("sha256string"));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockFileSystem));

    nlohmann::json response = uploadFolderAction.run(action.dump());
    
    EXPECT_EQ(response["result"], 0);
    EXPECT_EQ(response["httpStatus"], 200);
}

TEST_F(UploadFolderTests, SuccessCase)
{
    auto httpRequester = std::make_shared<StrictMock<MockHTTPRequester>>();
    Common::HttpRequests::Response httpresponse;
    httpresponse.status = Common::HttpRequests::HTTP_STATUS_OK;
    httpresponse.errorCode = Common::HttpRequests::ResponseErrorCode::OK;
    EXPECT_CALL(*httpRequester, put(_)).WillOnce(Return(httpresponse));

    ResponseActionsImpl::UploadFolderAction uploadFolderAction(httpRequester);

    nlohmann::json action = getDefaultUploadObject();

    auto mockFileSystem = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockFileSystem, isDirectory("/tmp/path")).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, fileSize(defaultZipFile)).WillOnce(Return(100));
    EXPECT_CALL(*mockFileSystem, isFile(defaultZipFile)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, removeFile(defaultZipFile)).WillOnce(Return());
    EXPECT_CALL(*mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, defaultZipFile))
        .WillOnce(Return("sha256string"));
    EXPECT_CALL(*mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockFileSystem));

    auto mockZip = std::make_unique<NiceMock<MockZipUtils>>();
    MockZipUtils& mockzipUtil(*mockZip);
    ON_CALL(mockzipUtil, zip(_, _, _)).WillByDefault(Return(0));
    Common::ZipUtilities::replaceZipUtils(std::move(mockZip));

    nlohmann::json response = uploadFolderAction.run(action.dump());
    
    EXPECT_EQ(response["result"], 0);
    EXPECT_EQ(response["httpStatus"], 200);
}

TEST_F(UploadFolderTests, SuccessCaseWithProxy)
{
    auto httpRequester = std::make_shared<StrictMock<MockHTTPRequester>>();
    Common::HttpRequests::Response httpresponse;
    httpresponse.status = Common::HttpRequests::HTTP_STATUS_OK;
    httpresponse.errorCode = Common::HttpRequests::ResponseErrorCode::OK;
    EXPECT_CALL(*httpRequester, put(_)).WillOnce(Return(httpresponse));

    ResponseActionsImpl::UploadFolderAction uploadFolderAction(httpRequester);

    nlohmann::json action = getDefaultUploadObject();

    auto mockFileSystem = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockFileSystem, isDirectory("/tmp/path")).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, fileSize(defaultZipFile)).WillOnce(Return(100));
    EXPECT_CALL(*mockFileSystem, isFile(defaultZipFile)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, removeFile(defaultZipFile)).WillOnce(Return());
    EXPECT_CALL(*mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, defaultZipFile))
        .WillOnce(Return("sha256string"));
    EXPECT_CALL(*mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(true));
    std::string obfuscatedCreds =
        "CCD4E57ZjW+t5XPiMSJH1TurG3MfWCN3DpjJRINMwqNaWl+3zzlVIdyVmifCHUwcmaX6+YTSyyBM8SslIIGV5rUw";
    std::string content = R"({"proxy":"localhost","credentials":")" + obfuscatedCreds + R"("})";
    EXPECT_CALL(*mockFileSystem, readFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy"))
        .WillOnce(Return(content));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockFileSystem));

    auto mockZip = std::make_unique<NiceMock<MockZipUtils>>();
    MockZipUtils& mockzipUtil(*mockZip);
    ON_CALL(mockzipUtil, zip(_, _, _)).WillByDefault(Return(0));
    Common::ZipUtilities::replaceZipUtils(std::move(mockZip));

    nlohmann::json response = uploadFolderAction.run(action.dump());
    
    EXPECT_EQ(response["result"], 0);
    EXPECT_EQ(response["httpStatus"], 200);
}

TEST_F(UploadFolderTests, ProxyFallsBackToDirect)
{
    auto httpRequester = std::make_shared<StrictMock<MockHTTPRequester>>();
    Common::HttpRequests::Response failhttpresponse;
    Common::HttpRequests::Response httpresponse;
    httpresponse.status = Common::HttpRequests::HTTP_STATUS_OK;
    httpresponse.errorCode = Common::HttpRequests::ResponseErrorCode::OK;
    failhttpresponse.status = 400;
    failhttpresponse.errorCode = Common::HttpRequests::ResponseErrorCode::OK;
    EXPECT_CALL(*httpRequester, put(_)).Times(2).WillOnce(Return(failhttpresponse)).WillOnce(Return(httpresponse));

    ResponseActionsImpl::UploadFolderAction uploadFolderAction(httpRequester);

    nlohmann::json action = getDefaultUploadObject();

    auto mockFileSystem = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockFileSystem, isDirectory("/tmp/path")).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, fileSize(defaultZipFile)).WillOnce(Return(100));
    EXPECT_CALL(*mockFileSystem, isFile(defaultZipFile)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, removeFile(defaultZipFile)).WillOnce(Return());
    EXPECT_CALL(*mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, defaultZipFile))
        .WillOnce(Return("sha256string"));
    EXPECT_CALL(*mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(true));
    std::string obfuscatedCreds =
        "CCD4E57ZjW+t5XPiMSJH1TurG3MfWCN3DpjJRINMwqNaWl+3zzlVIdyVmifCHUwcmaX6+YTSyyBM8SslIIGV5rUw";
    std::string content = R"({"proxy":"localhost","credentials":")" + obfuscatedCreds + R"("})";
    EXPECT_CALL(*mockFileSystem, readFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy"))
        .WillOnce(Return(content));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockFileSystem));

    auto mockZip = std::make_unique<NiceMock<MockZipUtils>>();
    MockZipUtils& mockzipUtil(*mockZip);
    ON_CALL(mockzipUtil, zip(_, _, _)).WillByDefault(Return(0));
    Common::ZipUtilities::replaceZipUtils(std::move(mockZip));

    nlohmann::json response = uploadFolderAction.run(action.dump());
    
    EXPECT_EQ(response["result"], 0);
    EXPECT_EQ(response["httpStatus"], 200);
}

TEST_F(UploadFolderTests, cannotParseActions)
{
    auto httpRequester = std::make_shared<StrictMock<MockHTTPRequester>>();
    ResponseActionsImpl::UploadFolderAction uploadFolderAction(httpRequester);

    nlohmann::json response = uploadFolderAction.run("");
    
    EXPECT_EQ(response["result"], 1);
    EXPECT_EQ(response["errorMessage"], "Error parsing command from Central");
}

TEST_F(UploadFolderTests, actionExipired)
{
    auto httpRequester = std::make_shared<StrictMock<MockHTTPRequester>>();
    ResponseActionsImpl::UploadFolderAction uploadFolderAction(httpRequester);
    nlohmann::json action = getDefaultUploadObject();
    action["expiration"] = 0;
    nlohmann::json response = uploadFolderAction.run(action.dump());
    
    EXPECT_EQ(response["result"], 4);
    EXPECT_EQ(response["errorMessage"], "Action has expired");
}

TEST_F(UploadFolderTests, FolderDoesNotExist)
{
    auto httpRequester = std::make_shared<StrictMock<MockHTTPRequester>>();
    ResponseActionsImpl::UploadFolderAction uploadFolderAction(httpRequester);
    nlohmann::json action = getDefaultUploadObject();

    auto mockFileSystem = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockFileSystem, isDirectory("/tmp/path")).WillOnce(Return(false));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockFileSystem));

    nlohmann::json response = uploadFolderAction.run(action.dump());
    
    EXPECT_EQ(response["result"], 1);
    EXPECT_EQ(response["errorType"], "invalid_path");
    EXPECT_EQ(response["errorMessage"], "/tmp/path is not a directory");
}

TEST_F(UploadFolderTests, ZippedFolderOverSizeLimit)
{
    auto httpRequester = std::make_shared<StrictMock<MockHTTPRequester>>();
    ResponseActionsImpl::UploadFolderAction uploadFolderAction(httpRequester);
    nlohmann::json action = getDefaultUploadObject();

    auto mockFileSystem = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockFileSystem, isDirectory("/tmp/path")).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, fileSize(defaultZipFile)).WillOnce(Return(10000));
    EXPECT_CALL(*mockFileSystem, isFile(defaultZipFile)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, removeFile(defaultZipFile)).WillOnce(Return());
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockFileSystem));

    auto mockZip = std::make_unique<NiceMock<MockZipUtils>>();
    MockZipUtils& mockzipUtil(*mockZip);
    ON_CALL(mockzipUtil, zip(_, _, _)).WillByDefault(Return(0));
    Common::ZipUtilities::replaceZipUtils(std::move(mockZip));

    nlohmann::json response = uploadFolderAction.run(action.dump());
    
    EXPECT_EQ(response["result"], 1);
    EXPECT_EQ(response["errorType"], "exceed_size_limit");
    EXPECT_EQ(
        response["errorMessage"],
        "Folder at path /tmp/path after being compressed is size 10000 bytes which is above the size limit 1000 bytes");
}

TEST_F(UploadFolderTests, ZipFails)
{
    auto httpRequester = std::make_shared<StrictMock<MockHTTPRequester>>();
    ResponseActionsImpl::UploadFolderAction uploadFolderAction(httpRequester);
    nlohmann::json action = getDefaultUploadObject();

    auto mockFileSystem = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockFileSystem, isDirectory("/tmp/path")).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, isFile(defaultZipFile)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, removeFile(defaultZipFile)).WillOnce(Return());
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockFileSystem));

    auto mockZip = std::make_unique<NiceMock<MockZipUtils>>();
    MockZipUtils& mockzipUtil(*mockZip);
    ON_CALL(mockzipUtil, zip(_, _, _)).WillByDefault(Return(1));
    Common::ZipUtilities::replaceZipUtils(std::move(mockZip));

    nlohmann::json response = uploadFolderAction.run(action.dump());
    
    EXPECT_EQ(response["result"], 3);
    EXPECT_EQ(response["errorMessage"], "Error zipping /tmp/path");
}

TEST_F(UploadFolderTests, ZipFailsDueToLargeFile)
{
    auto httpRequester = std::make_shared<StrictMock<MockHTTPRequester>>();
    ResponseActionsImpl::UploadFolderAction uploadFolderAction(httpRequester);
    nlohmann::json action = getDefaultUploadObject();

    auto mockFileSystem = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockFileSystem, isDirectory("/tmp/path")).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, isFile(defaultZipFile)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, removeFile(defaultZipFile)).WillOnce(Return());
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockFileSystem));

    auto mockZip = std::make_unique<NiceMock<MockZipUtils>>();
    MockZipUtils& mockzipUtil(*mockZip);
    ON_CALL(mockzipUtil, zip(_, _, _)).WillByDefault(Throw(std::runtime_error("")));
    Common::ZipUtilities::replaceZipUtils(std::move(mockZip));

    nlohmann::json response = uploadFolderAction.run(action.dump());
    
    EXPECT_EQ(response["result"], 3);
    EXPECT_EQ(response["errorMessage"], "Error zipping /tmp/path");
}

TEST_F(UploadFolderTests, FileBeingWrittenToAndOverSizeLimit)
{
    auto httpRequester = std::make_shared<StrictMock<MockHTTPRequester>>();
    ResponseActionsImpl::UploadFolderAction uploadFolderAction(httpRequester);
    nlohmann::json action = getDefaultUploadObject();

    auto mockFileSystem = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockFileSystem, isDirectory("/tmp/path")).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, fileSize(defaultZipFile)).WillOnce(Return(100));
    EXPECT_CALL(*mockFileSystem, isFile(defaultZipFile)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, removeFile(defaultZipFile)).WillOnce(Return());
    EXPECT_CALL(*mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, defaultZipFile))
        .WillOnce(Throw(Common::FileSystem::IFileSystemException("")));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockFileSystem));

    auto mockZip = std::make_unique<NiceMock<MockZipUtils>>();
    MockZipUtils& mockzipUtil(*mockZip);
    ON_CALL(mockzipUtil, zip(_, _, _)).WillByDefault(Return(0));
    Common::ZipUtilities::replaceZipUtils(std::move(mockZip));

    nlohmann::json response = uploadFolderAction.run(action.dump());
    
    EXPECT_EQ(response["result"], 1);
    EXPECT_EQ(response["errorType"], "access_denied");
    EXPECT_EQ(response["errorMessage"], "Zip file to be uploaded cannot be accessed");
}

TEST_F(UploadFolderTests, FailureDueToTimeout)
{
    auto httpRequester = std::make_shared<StrictMock<MockHTTPRequester>>();
    Common::HttpRequests::Response httpresponse;
    httpresponse.status = 500;
    httpresponse.errorCode = Common::HttpRequests::ResponseErrorCode::TIMEOUT;
    EXPECT_CALL(*httpRequester, put(_)).WillOnce(Return(httpresponse));
    ResponseActionsImpl::UploadFolderAction uploadFolderAction(httpRequester);

    nlohmann::json action = getDefaultUploadObject();

    auto mockFileSystem = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockFileSystem, isDirectory("/tmp/path")).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, fileSize(defaultZipFile)).WillOnce(Return(100));
    EXPECT_CALL(*mockFileSystem, isFile(defaultZipFile)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, removeFile(defaultZipFile)).WillOnce(Return());
    EXPECT_CALL(*mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, defaultZipFile))
        .WillOnce(Return("sha256string"));
    EXPECT_CALL(*mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockFileSystem));

    auto mockZip = std::make_unique<NiceMock<MockZipUtils>>();
    MockZipUtils& mockzipUtil(*mockZip);
    ON_CALL(mockzipUtil, zip(_, _, _)).WillByDefault(Return(0));
    Common::ZipUtilities::replaceZipUtils(std::move(mockZip));

    nlohmann::json response = uploadFolderAction.run(action.dump());
    
    EXPECT_EQ(response["result"], 2);
    EXPECT_EQ(response["httpStatus"], 500);
    EXPECT_EQ(response["errorMessage"], "Timeout uploading zip file: " + defaultZipFile);
}

TEST_F(UploadFolderTests, FailureDueToNetworkError)
{
    auto httpRequester = std::make_shared<StrictMock<MockHTTPRequester>>();
    Common::HttpRequests::Response httpresponse;
    httpresponse.status = 500;
    httpresponse.errorCode = Common::HttpRequests::ResponseErrorCode::FAILED;
    httpresponse.error = "failure";
    EXPECT_CALL(*httpRequester, put(_)).WillOnce(Return(httpresponse));
    ResponseActionsImpl::UploadFolderAction uploadFolderAction(httpRequester);
    nlohmann::json action = getDefaultUploadObject();

    auto mockFileSystem = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockFileSystem, isDirectory("/tmp/path")).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, isFile(defaultZipFile)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, removeFile(defaultZipFile)).WillOnce(Return());
    EXPECT_CALL(*mockFileSystem, fileSize(defaultZipFile)).WillOnce(Return(100));
    EXPECT_CALL(*mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, defaultZipFile))
        .WillOnce(Return("sha256string"));
    EXPECT_CALL(*mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockFileSystem));

    auto mockZip = std::make_unique<NiceMock<MockZipUtils>>();
    MockZipUtils& mockzipUtil(*mockZip);
    ON_CALL(mockzipUtil, zip(_, _, _)).WillByDefault(Return(0));
    Common::ZipUtilities::replaceZipUtils(std::move(mockZip));

    nlohmann::json response = uploadFolderAction.run(action.dump());
    
    EXPECT_EQ(response["result"], 1);
    EXPECT_EQ(response["httpStatus"], 500);
    EXPECT_EQ(response["errorMessage"], "Failed to upload zip file: " + defaultZipFile + " with error: failure");
    EXPECT_EQ(response["errorType"], "network_error");
}

TEST_F(UploadFolderTests, FailureDueToServerError)
{
    auto httpRequester = std::make_shared<StrictMock<MockHTTPRequester>>();
    Common::HttpRequests::Response httpresponse;
    httpresponse.status = 500;
    httpresponse.errorCode = Common::HttpRequests::ResponseErrorCode::OK;
    EXPECT_CALL(*httpRequester, put(_)).WillOnce(Return(httpresponse));
    ResponseActionsImpl::UploadFolderAction uploadFolderAction(httpRequester);
    nlohmann::json action = getDefaultUploadObject();

    auto mockFileSystem = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockFileSystem, isDirectory("/tmp/path")).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, fileSize(defaultZipFile)).WillOnce(Return(100));
    EXPECT_CALL(*mockFileSystem, isFile(defaultZipFile)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, removeFile(defaultZipFile)).WillOnce(Return());
    EXPECT_CALL(*mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, defaultZipFile))
        .WillOnce(Return("sha256string"));
    EXPECT_CALL(*mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockFileSystem));

    auto mockZip = std::make_unique<NiceMock<MockZipUtils>>();
    MockZipUtils& mockzipUtil(*mockZip);
    ON_CALL(mockzipUtil, zip(_, _, _)).WillByDefault(Return(0));
    Common::ZipUtilities::replaceZipUtils(std::move(mockZip));

    nlohmann::json response = uploadFolderAction.run(action.dump());
    
    EXPECT_EQ(response["result"], 1);
    EXPECT_EQ(response["httpStatus"], 500);
    EXPECT_EQ(response["errorMessage"], "Failed to upload zip file: " + defaultZipFile + " with http error code 500");
    EXPECT_EQ(response["errorType"], "network_error");
}

TEST_F(UploadFolderTests, FailureDueToServerCertError)
{
    auto httpRequester = std::make_shared<StrictMock<MockHTTPRequester>>();
    Common::HttpRequests::Response httpresponse;
    httpresponse.status = 500;
    httpresponse.errorCode = Common::HttpRequests::ResponseErrorCode::CERTIFICATE_ERROR;
    httpresponse.error = "SSL issues";
    EXPECT_CALL(*httpRequester, put(_)).WillOnce(Return(httpresponse));
    ResponseActionsImpl::UploadFolderAction uploadFolderAction(httpRequester);
    nlohmann::json action = getDefaultUploadObject();

    auto mockFileSystem = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockFileSystem, isDirectory("/tmp/path")).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, fileSize(defaultZipFile)).WillOnce(Return(100));
    EXPECT_CALL(*mockFileSystem, isFile(defaultZipFile)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, removeFile(defaultZipFile)).WillOnce(Return());
    EXPECT_CALL(*mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, defaultZipFile))
        .WillOnce(Return("sha256string"));
    EXPECT_CALL(*mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockFileSystem));

    auto mockZip = std::make_unique<NiceMock<MockZipUtils>>();
    MockZipUtils& mockzipUtil(*mockZip);
    ON_CALL(mockzipUtil, zip(_, _, _)).WillByDefault(Return(0));
    Common::ZipUtilities::replaceZipUtils(std::move(mockZip));

    nlohmann::json response = uploadFolderAction.run(action.dump());

    EXPECT_EQ(response["result"], 1);
    EXPECT_EQ(response["httpStatus"], 500);
    EXPECT_EQ(response["errorMessage"], "Failed to upload zip file: " + defaultZipFile + " with error: SSL issues");
    EXPECT_EQ(response["errorType"], "network_error");
}