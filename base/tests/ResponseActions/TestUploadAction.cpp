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

using namespace Common::ApplicationConfiguration;
using namespace Common::HttpRequests;
using namespace ResponseActions::RACommon;

class UploadFileTests : public MemoryAppenderUsingTests
{
public:
    UploadFileTests() : MemoryAppenderUsingTests("responseactions") {}

    void SetUp()
    {
        m_mockFileSystem = std::make_unique<StrictMock<MockFileSystem>>();
        m_mockHttpRequester = std::make_shared<StrictMock<MockHTTPRequester>>();
    }

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
    
    void addResponseToMockRequester(const long& status, const ResponseErrorCode& errorCode, const std::string& errormsg = "")
    {
        Common::HttpRequests::Response httpresponse;
        httpresponse.status = status;
        httpresponse.errorCode = errorCode;
        httpresponse.error = errormsg;
        EXPECT_CALL(*m_mockHttpRequester, put(_)).WillOnce(Return(httpresponse));
    }
    
    void setupMockZipUtils(const int& returnVal = 0)
    {
        auto mockZip = std::make_unique<NiceMock<MockZipUtils>>();
        EXPECT_CALL(*mockZip, zip(_, _, _)).WillOnce(Return(returnVal));
        Common::ZipUtilities::replaceZipUtils(std::move(mockZip));
    }

    std::shared_ptr<MockHTTPRequester> m_mockHttpRequester;
    std::unique_ptr<MockFileSystem> m_mockFileSystem;
};

TEST_F(UploadFileTests, SuccessCase)
{
    addResponseToMockRequester(HTTP_STATUS_OK, ResponseErrorCode::OK);
    ResponseActionsImpl::UploadFileAction uploadFileAction(m_mockHttpRequester);

    nlohmann::json action = getDefaultUploadObject();

    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, isFile("path")).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, fileSize("path")).WillOnce(Return(100));
    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, "path"))
        .WillOnce(Return("sha256string"));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    nlohmann::json response = uploadFileAction.run(action.dump());

    EXPECT_EQ(response["result"], 0);
    EXPECT_EQ(response["httpStatus"], 200);
}

TEST_F(UploadFileTests, SuccessCaseWithProxy)
{
    addResponseToMockRequester(HTTP_STATUS_OK, ResponseErrorCode::OK);
    ResponseActionsImpl::UploadFileAction uploadFileAction(m_mockHttpRequester);

    nlohmann::json action = getDefaultUploadObject();

    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(true));
    std::string obfuscatedCreds =
        "CCD4E57ZjW+t5XPiMSJH1TurG3MfWCN3DpjJRINMwqNaWl+3zzlVIdyVmifCHUwcmaX6+YTSyyBM8SslIIGV5rUw";
    std::string content = R"({"proxy":"localhost","credentials":")" + obfuscatedCreds + R"("})";
    EXPECT_CALL(*m_mockFileSystem, readFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy"))
        .WillOnce(Return(content));
    EXPECT_CALL(*m_mockFileSystem, isFile("path")).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, fileSize("path")).WillOnce(Return(100));
    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, "path"))
        .WillOnce(Return("sha256string"));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    nlohmann::json response = uploadFileAction.run(action.dump());

    EXPECT_EQ(response["result"], 0);
    EXPECT_EQ(response["httpStatus"], 200);
}

TEST_F(UploadFileTests, ProxyFailureFallsbackDirect)
{
    Common::HttpRequests::Response failhttpresponse;
    Common::HttpRequests::Response httpresponse;
    httpresponse.status = Common::HttpRequests::HTTP_STATUS_OK;
    httpresponse.errorCode = Common::HttpRequests::ResponseErrorCode::OK;
    failhttpresponse.status = 400;
    failhttpresponse.errorCode = Common::HttpRequests::ResponseErrorCode::OK;
    EXPECT_CALL(*m_mockHttpRequester, put(_)).Times(2).WillOnce(Return(failhttpresponse)).WillOnce(Return(httpresponse));

    ResponseActionsImpl::UploadFileAction uploadFileAction(m_mockHttpRequester);
    nlohmann::json action = getDefaultUploadObject();

    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(true));
    std::string obfuscatedCreds =
        "CCD4E57ZjW+t5XPiMSJH1TurG3MfWCN3DpjJRINMwqNaWl+3zzlVIdyVmifCHUwcmaX6+YTSyyBM8SslIIGV5rUw";
    std::string content = R"({"proxy":"localhost","credentials":")" + obfuscatedCreds + R"("})";
    EXPECT_CALL(*m_mockFileSystem, readFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy"))
        .WillOnce(Return(content));
    EXPECT_CALL(*m_mockFileSystem, isFile("path")).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, fileSize("path")).WillOnce(Return(100));
    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, "path"))
        .WillOnce(Return("sha256string"));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    nlohmann::json response = uploadFileAction.run(action.dump());

    EXPECT_EQ(response["result"], 0);
    EXPECT_EQ(response["httpStatus"], 200);
}

TEST_F(UploadFileTests, SuccessCaseCompressionWithPassword)
{
    // TOUCHES FILE SYSTEM
    addResponseToMockRequester(HTTP_STATUS_OK, ResponseErrorCode::OK);

    Tests::TempDir tempDir;
    tempDir.makeDirs("test");
    tempDir.createFile("test/file.txt", "stuff");
    std::string filePath = tempDir.absPath("test/file.txt");
    std::string zipFile = tempDir.absPath("file.txt.zip");

    ResponseActionsImpl::UploadFileAction uploadFileAction(m_mockHttpRequester);
    nlohmann::json action = getDefaultUploadObject();
    action["compress"] = true;
    action["password"] = "password";
    action["targetFile"] = filePath;

    auto mockAppManager = std::make_unique<MockedApplicationPathManager>();
    ON_CALL(*mockAppManager, getResponseActionTmpPath()).WillByDefault(Return(tempDir.absPath("")));
    ON_CALL(*mockAppManager, getMcsCurrentProxyFilePath())
        .WillByDefault(Return("/opt/sophos-spl/base/etc/sophosspl/current_proxy"));

    replaceApplicationPathManager(std::move(mockAppManager));

    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, isFile(filePath)).Times(3).WillRepeatedly(Return(true));
    EXPECT_CALL(*m_mockFileSystem, readFile(filePath, _)).WillOnce(Return("stuff"));
    EXPECT_CALL(*m_mockFileSystem, isFile(zipFile)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, removeFile(zipFile)).Times(1).WillOnce(Return());
    EXPECT_CALL(*m_mockFileSystem, fileSize(zipFile)).WillOnce(Return(8));
    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, zipFile))
        .WillOnce(Return("sha256string"));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    nlohmann::json response = uploadFileAction.run(action.dump());

    EXPECT_EQ(response["result"], 0);
    EXPECT_EQ(response["httpStatus"], 200);
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
    ResponseActionsImpl::UploadFileAction uploadFileAction(m_mockHttpRequester);

    nlohmann::json action = getDefaultUploadObject();
    action["compress"] = true;
    std::string zipFile = "/opt/sophos-spl/plugins/responseactions/tmp/path.zip";

    EXPECT_CALL(*m_mockFileSystem, isFile("path")).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, isFile(zipFile)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, removeFile(zipFile)).Times(1).WillOnce(Return());
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    setupMockZipUtils(1);

    nlohmann::json response = uploadFileAction.run(action.dump());

    EXPECT_EQ(response["result"], 3);
    EXPECT_EQ(response["errorMessage"], "Error zipping path");
}

TEST_F(UploadFileTests, ZipFailsDueToLargeFile)
{
    ResponseActionsImpl::UploadFileAction uploadFileAction(m_mockHttpRequester);

    nlohmann::json action = getDefaultUploadObject();
    action["compress"] = true;
    std::string zipFile = "/opt/sophos-spl/plugins/responseactions/tmp/path.zip";

    EXPECT_CALL(*m_mockFileSystem, isFile("path")).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, isFile(zipFile)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, removeFile(zipFile)).Times(1).WillOnce(Return());
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    auto mockZip = std::make_unique<NiceMock<MockZipUtils>>();
    ON_CALL(*mockZip, zip(_, _, _)).WillByDefault(Throw(std::runtime_error("")));
    Common::ZipUtilities::replaceZipUtils(std::move(mockZip));

    nlohmann::json response = uploadFileAction.run(action.dump());

    EXPECT_EQ(response["result"], 3);
    EXPECT_EQ(response["errorMessage"], "Error zipping path");
}

TEST_F(UploadFileTests, cannotParseActions)
{
    ResponseActionsImpl::UploadFileAction uploadFileAction(m_mockHttpRequester);

    nlohmann::json response = uploadFileAction.run("");

    EXPECT_EQ(response["result"], 1);
    EXPECT_EQ(response["errorMessage"], "Error parsing command from Central");
}

TEST_F(UploadFileTests, actionExipired)
{
    ResponseActionsImpl::UploadFileAction uploadFileAction(m_mockHttpRequester);
    nlohmann::json action = getDefaultUploadObject();
    action["expiration"] = 0;
    nlohmann::json response = uploadFileAction.run(action.dump());

    EXPECT_EQ(response["result"],4);
    EXPECT_EQ(response["errorMessage"],"Action has expired");
}

TEST_F(UploadFileTests, FileDoesNotExist)
{
    ResponseActionsImpl::UploadFileAction uploadFileAction(m_mockHttpRequester);
    nlohmann::json action = getDefaultUploadObject();

    EXPECT_CALL(*m_mockFileSystem, isFile("path")).WillOnce(Return(false));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    nlohmann::json response = uploadFileAction.run(action.dump());

    EXPECT_EQ(response["result"],1);
    EXPECT_EQ(response["errorType"],"invalid_path");
    EXPECT_EQ(response["errorMessage"],"path is not a file");
}

TEST_F(UploadFileTests, FileOverSizeLimit)
{
    ResponseActionsImpl::UploadFileAction uploadFileAction(m_mockHttpRequester);
    nlohmann::json action = getDefaultUploadObject();

    EXPECT_CALL(*m_mockFileSystem, isFile("path")).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, fileSize("path")).WillOnce(Return(10000));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    nlohmann::json response = uploadFileAction.run(action.dump());

    EXPECT_EQ(response["result"],1);
    EXPECT_EQ(response["errorType"],"exceed_size_limit");
    EXPECT_EQ(response["errorMessage"],"File at path path is size 10000 bytes which is above the size limit 1000 bytes");
}

TEST_F(UploadFileTests, FileBeingWrittenToAndOverSizeLimit)
{
    ResponseActionsImpl::UploadFileAction uploadFileAction(m_mockHttpRequester);
    nlohmann::json action = getDefaultUploadObject();

    EXPECT_CALL(*m_mockFileSystem, isFile("path")).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, fileSize("path")).WillOnce(Return(100));
    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256,"path")).WillOnce(Throw(Common::FileSystem::IFileSystemException("")));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    nlohmann::json response = uploadFileAction.run(action.dump());

    EXPECT_EQ(response["result"],1);
    EXPECT_EQ(response["errorType"],"access_denied");
    EXPECT_EQ(response["errorMessage"],"File to be uploaded cannot be accessed");
}

TEST_F(UploadFileTests, FailureDueToTimeout)
{
    addResponseToMockRequester(500, ResponseErrorCode::TIMEOUT);
    ResponseActionsImpl::UploadFileAction uploadFileAction(m_mockHttpRequester);

    nlohmann::json action = getDefaultUploadObject();

    EXPECT_CALL(*m_mockFileSystem, isFile("path")).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, fileSize("path")).WillOnce(Return(100));
    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, "path"))
        .WillOnce(Return("sha256string"));
    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    nlohmann::json response = uploadFileAction.run(action.dump());

    EXPECT_EQ(response["result"], 2);
    EXPECT_EQ(response["httpStatus"], 500);
    EXPECT_EQ(response["errorMessage"], "Timeout Uploading file: path");
}

TEST_F(UploadFileTests, FailureDueToNetworkError)
{
    Common::HttpRequests::Response httpresponse;
    httpresponse.status = 500;
    httpresponse.errorCode = Common::HttpRequests::ResponseErrorCode::FAILED;
    httpresponse.error = "failure";
    EXPECT_CALL(*m_mockHttpRequester, put(_)).WillOnce(Return(httpresponse));
    ResponseActionsImpl::UploadFileAction uploadFileAction(m_mockHttpRequester);

    nlohmann::json action = getDefaultUploadObject();

    EXPECT_CALL(*m_mockFileSystem, isFile("path")).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, fileSize("path")).WillOnce(Return(100));
    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, "path"))
        .WillOnce(Return("sha256string"));
    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    nlohmann::json response = uploadFileAction.run(action.dump());

    EXPECT_EQ(response["result"], 1);
    EXPECT_EQ(response["httpStatus"], 500);
    EXPECT_EQ(response["errorMessage"], "Failed to upload file: path with error: failure");
    EXPECT_EQ(response["errorType"], "network_error");
}

TEST_F(UploadFileTests, FailureDueToServerError)
{
    addResponseToMockRequester(500, ResponseErrorCode::OK);
    ResponseActionsImpl::UploadFileAction uploadFileAction(m_mockHttpRequester);

    nlohmann::json action = getDefaultUploadObject();

    EXPECT_CALL(*m_mockFileSystem, isFile("path")).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, fileSize("path")).WillOnce(Return(100));
    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, "path"))
        .WillOnce(Return("sha256string"));
    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    nlohmann::json response = uploadFileAction.run(action.dump());

    EXPECT_EQ(response["result"], 1);
    EXPECT_EQ(response["httpStatus"], 500);
    EXPECT_EQ(response["errorMessage"], "Failed to upload file: path with http error code 500");
    EXPECT_EQ(response["errorType"], "network_error");
}
