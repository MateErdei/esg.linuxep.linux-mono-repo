// Copyright 2023-2024 Sophos Limited. All rights reserved.

#include "Common/FileSystem/IFileTooLargeException.h"
#include "Common/ZipUtilities/ZipUtils.h"
#include "ResponseActions/ResponseActionsImpl/UploadFileAction.h"
#include "tests/Common/ApplicationConfiguration/MockedApplicationPathManager.h"
#include "tests/Common/Helpers/FileSystemReplaceAndRestore.h"
#include "tests/Common/Helpers/MemoryAppender.h"
#include "tests/Common/Helpers/MockFileSystem.h"
#include "tests/Common/Helpers/MockHttpRequester.h"
#include "tests/Common/Helpers/MockZipUtils.h"
#include "tests/Common/Helpers/TempDir.h"
#include "tests/Common/Helpers/MockSignalHandler.h"
#include "tests/Common/Helpers/MockSysCalls.h"
#include "tests/Common/Helpers/MockSysCallsFactory.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <minizip/mz_compat.h>

#include <filesystem>

using namespace Common::ApplicationConfiguration;
using namespace Common::HttpRequests;
using namespace ResponseActions::RACommon;

class UploadFileTests : public MemoryAppenderUsingTests
{
public:
    UploadFileTests() : MemoryAppenderUsingTests("ResponseActionsImpl") {}

    void SetUp()
    {
        m_mockFileSystem = std::make_unique<StrictMock<MockFileSystem>>();
        m_mockHttpRequester = std::make_shared<StrictMock<MockHTTPRequester>>();
        m_mockSignalHandler = std::make_shared<NiceMock<MockSignalHandler>>();

        setupMockSysCalls();
    }

    void setupMockSysCalls()
    {
        m_mockSysCallWrapper = std::make_shared<NiceMock<MockSystemCallWrapper>>();
        ON_CALL(*m_mockSysCallWrapper, ppoll).WillByDefault(Return(-1));

    }

    std::shared_ptr<MockSystemCallWrapper> m_mockSysCallWrapper;
    std::shared_ptr<MockSignalHandler> m_mockSignalHandler;

    virtual void TearDown()
    {
        Tests::restoreFileSystem();
        Common::ZipUtilities::restoreZipUtils();
        Common::ApplicationConfiguration::restoreApplicationPathManager();
    }
    nlohmann::json getDefaultUploadObject()
    {
        nlohmann::json action;
        action["targetFile"] = m_testPath;
        action["url"] = m_testURL;
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

    void setupMockZipUtils(const int& returnVal = UNZ_OK)
    {
        auto mockZip = std::make_unique<NiceMock<MockZipUtils>>();
        EXPECT_CALL(*mockZip, zip(_, _, _)).WillOnce(Return(returnVal));
        Common::ZipUtilities::replaceZipUtils(std::move(mockZip));
    }

    const std::string m_testPath = "/path";
    const std::string m_zipPath = "/opt/sophos-spl/plugins/responseactions/tmp/path.zip";
    const std::string m_testURL = "https://s3.com/somewhere";

    std::shared_ptr<MockHTTPRequester> m_mockHttpRequester;
    std::unique_ptr<MockFileSystem> m_mockFileSystem;
};

TEST_F(UploadFileTests, SuccessCase)
{
    addResponseToMockRequester(HTTP_STATUS_OK, ResponseErrorCode::OK);
    ResponseActionsImpl::UploadFileAction uploadFileAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);

    nlohmann::json action = getDefaultUploadObject();

    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_testPath)).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, fileSize(m_testPath)).WillOnce(Return(100));
    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, m_testPath))
        .WillOnce(Return("sha256string"));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    nlohmann::json response = uploadFileAction.run(action.dump());

    EXPECT_EQ(response["result"], ResponseResult::SUCCESS);
    EXPECT_EQ(response["httpStatus"], HTTP_STATUS_OK);
}

TEST_F(UploadFileTests, successHugeURL)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    const std::string largeStr(30000, 'a');
    const std::string largeURL("https://s3.com/download" + largeStr + ".zip");
    const std::string expectedMsg("Uploading file: " + m_testPath + " to url: " + largeURL);

    addResponseToMockRequester(HTTP_STATUS_OK, ResponseErrorCode::OK);

    ResponseActionsImpl::UploadFileAction uploadFileAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);

    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_testPath)).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, fileSize(m_testPath)).WillOnce(Return(100));
    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, m_testPath))
        .WillOnce(Return("sha256string"));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    nlohmann::json action = getDefaultUploadObject();
    action["url"] = largeURL;
    nlohmann::json response = uploadFileAction.run(action.dump());

    EXPECT_EQ(response["result"], ResponseResult::SUCCESS);
    EXPECT_EQ(response["httpStatus"], HTTP_STATUS_OK);

    EXPECT_EQ(response["type"], UPLOAD_FILE_RESPONSE_TYPE);
    EXPECT_EQ(response["result"], ResponseResult::SUCCESS);
    EXPECT_EQ(response["httpStatus"], HTTP_STATUS_OK);
    EXPECT_FALSE(response.contains("errorType"));
    EXPECT_FALSE(response.contains("errorMessage"));
    EXPECT_TRUE(response.contains("duration"));

    EXPECT_TRUE(appenderContains(expectedMsg));
}

TEST_F(UploadFileTests, successHugePath)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    const std::string largeStr(30000, 'a');
    const std::string largeTargetPath(m_testPath + largeStr);
    const std::string expectedMsg("Uploading file: " + largeTargetPath + " to url: " + m_testURL);

    addResponseToMockRequester(HTTP_STATUS_OK, ResponseErrorCode::OK);

    ResponseActionsImpl::UploadFileAction uploadFileAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);

    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, isFile(largeTargetPath)).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, fileSize(largeTargetPath)).WillOnce(Return(100));
    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, largeTargetPath))
        .WillOnce(Return("sha256string"));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    nlohmann::json action = getDefaultUploadObject();
    action["targetFile"] = largeTargetPath;
    nlohmann::json response = uploadFileAction.run(action.dump());

    EXPECT_EQ(response["type"], UPLOAD_FILE_RESPONSE_TYPE);
    EXPECT_EQ(response["result"], ResponseResult::SUCCESS);
    EXPECT_EQ(response["httpStatus"], HTTP_STATUS_OK);
    EXPECT_FALSE(response.contains("errorType"));
    EXPECT_FALSE(response.contains("errorMessage"));
    EXPECT_TRUE(response.contains("duration"));

    EXPECT_TRUE(appenderContains(expectedMsg));
}

TEST_F(UploadFileTests, SuccessCaseWithProxy)
{
    addResponseToMockRequester(HTTP_STATUS_OK, ResponseErrorCode::OK);
    ResponseActionsImpl::UploadFileAction uploadFileAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);

    nlohmann::json action = getDefaultUploadObject();

    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(true));
    std::string obfuscatedCreds =
        "CCD4E57ZjW+t5XPiMSJH1TurG3MfWCN3DpjJRINMwqNaWl+3zzlVIdyVmifCHUwcmaX6+YTSyyBM8SslIIGV5rUw";
    std::string content = R"({"proxy":"localhost","credentials":")" + obfuscatedCreds + R"("})";
    EXPECT_CALL(*m_mockFileSystem, readFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy"))
        .WillOnce(Return(content));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_testPath)).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, fileSize(m_testPath)).WillOnce(Return(100));
    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, m_testPath))
        .WillOnce(Return("sha256string"));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    nlohmann::json response = uploadFileAction.run(action.dump());

    EXPECT_EQ(response["result"], ResponseResult::SUCCESS);
    EXPECT_EQ(response["httpStatus"], HTTP_STATUS_OK);
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

    ResponseActionsImpl::UploadFileAction uploadFileAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);
    nlohmann::json action = getDefaultUploadObject();

    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(true));
    std::string obfuscatedCreds =
        "CCD4E57ZjW+t5XPiMSJH1TurG3MfWCN3DpjJRINMwqNaWl+3zzlVIdyVmifCHUwcmaX6+YTSyyBM8SslIIGV5rUw";
    std::string content = R"({"proxy":"localhost","credentials":")" + obfuscatedCreds + R"("})";
    EXPECT_CALL(*m_mockFileSystem, readFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy"))
        .WillOnce(Return(content));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_testPath)).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, fileSize(m_testPath)).WillOnce(Return(100));
    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, m_testPath))
        .WillOnce(Return("sha256string"));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    nlohmann::json response = uploadFileAction.run(action.dump());

    EXPECT_EQ(response["result"], ResponseResult::SUCCESS);
    EXPECT_EQ(response["httpStatus"], HTTP_STATUS_OK);
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

    ResponseActionsImpl::UploadFileAction uploadFileAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);
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

    EXPECT_EQ(response["result"], ResponseResult::SUCCESS);
    EXPECT_EQ(response["httpStatus"], HTTP_STATUS_OK);
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

TEST_F(UploadFileTests, successEmptyPassword)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    addResponseToMockRequester(HTTP_STATUS_OK, ResponseErrorCode::OK);

    auto mockZip = std::make_unique<NiceMock<MockZipUtils>>();
    EXPECT_CALL(*mockZip, zip(_, _, _)).WillOnce(Return(UNZ_OK));
    EXPECT_CALL(*mockZip, zip(_, _, _, _, _)).Times(0);
    Common::ZipUtilities::replaceZipUtils(std::move(mockZip));

    ResponseActionsImpl::UploadFileAction uploadFileAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);

    nlohmann::json action = getDefaultUploadObject();
    action["compress"] = true;
    action["password"] = "";

    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_testPath)).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_zipPath)).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, fileSize(m_zipPath)).WillOnce(Return(100));
    EXPECT_CALL(*m_mockFileSystem, removeFile(m_zipPath)).Times(1);
    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, m_zipPath))
        .WillOnce(Return("sha256string"));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    nlohmann::json response = uploadFileAction.run(action.dump());

    EXPECT_EQ(response["result"], ResponseResult::SUCCESS);
    EXPECT_EQ(response["httpStatus"], HTTP_STATUS_OK);
    EXPECT_FALSE(response.contains("errorType"));
    EXPECT_FALSE(response.contains("errorMessage"));

    EXPECT_TRUE(appenderContains("Upload for /opt/sophos-spl/plugins/responseactions/tmp/path.zip succeeded"));
}

TEST_F(UploadFileTests, successHugePassword)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    const std::string largePassword(30000, 'a');

    addResponseToMockRequester(HTTP_STATUS_OK, ResponseErrorCode::OK);

    auto mockZip = std::make_unique<NiceMock<MockZipUtils>>();
    EXPECT_CALL(*mockZip, zip(_, _, _)).Times(0);
    EXPECT_CALL(*mockZip, zip(_, _, _, _, _)).WillOnce(Return(UNZ_OK));
    Common::ZipUtilities::replaceZipUtils(std::move(mockZip));

    ResponseActionsImpl::UploadFileAction uploadFileAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);

    nlohmann::json action = getDefaultUploadObject();
    action["compress"] = true;
    action["password"] = largePassword;

    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_testPath)).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_zipPath)).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, fileSize(m_zipPath)).WillOnce(Return(100));
    EXPECT_CALL(*m_mockFileSystem, removeFile(m_zipPath)).Times(1);
    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, m_zipPath))
        .WillOnce(Return("sha256string"));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    nlohmann::json response = uploadFileAction.run(action.dump());

    EXPECT_EQ(response["result"], ResponseResult::SUCCESS);
    EXPECT_EQ(response["httpStatus"], HTTP_STATUS_OK);
    EXPECT_FALSE(response.contains("errorType"));
    EXPECT_FALSE(response.contains("errorMessage"));

    EXPECT_TRUE(appenderContains("Upload for /opt/sophos-spl/plugins/responseactions/tmp/path.zip succeeded"));
}

TEST_F(UploadFileTests, ZipFails)
{
    ResponseActionsImpl::UploadFileAction uploadFileAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);

    nlohmann::json action = getDefaultUploadObject();
    action["compress"] = true;
    std::string zipFile = "/opt/sophos-spl/plugins/responseactions/tmp/path.zip";

    EXPECT_CALL(*m_mockFileSystem, isFile(m_testPath)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, isFile(zipFile)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, removeFile(zipFile)).Times(1).WillOnce(Return());
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    setupMockZipUtils(UNZ_BADZIPFILE);

    nlohmann::json response = uploadFileAction.run(action.dump());

    EXPECT_EQ(response["result"], ResponseResult::INTERNAL_ERROR);
    EXPECT_EQ(response["errorMessage"], "Error zipping /path");
}

TEST_F(UploadFileTests, ZipFailsDueToLargeFile)
{
    ResponseActionsImpl::UploadFileAction uploadFileAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);

    nlohmann::json action = getDefaultUploadObject();
    action["compress"] = true;
    std::string zipFile = "/opt/sophos-spl/plugins/responseactions/tmp/path.zip";

    EXPECT_CALL(*m_mockFileSystem, isFile(m_testPath)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, isFile(zipFile)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, removeFile(zipFile)).Times(1).WillOnce(Return());
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    auto mockZip = std::make_unique<NiceMock<MockZipUtils>>();
    ON_CALL(*mockZip, zip(_, _, _)).WillByDefault(Throw(std::runtime_error("")));
    Common::ZipUtilities::replaceZipUtils(std::move(mockZip));

    nlohmann::json response = uploadFileAction.run(action.dump());

    EXPECT_EQ(response["result"], ResponseResult::INTERNAL_ERROR);
    EXPECT_EQ(response["errorMessage"], "Error zipping /path");
}

TEST_F(UploadFileTests, cannotParseActions)
{
    ResponseActionsImpl::UploadFileAction uploadFileAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);

    nlohmann::json response = uploadFileAction.run("");

    EXPECT_EQ(response["result"], ResponseResult::ERROR);
    EXPECT_EQ(response["errorMessage"], "Error parsing command from Central");
}

TEST_F(UploadFileTests, actionExpired)
{
    ResponseActionsImpl::UploadFileAction uploadFileAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);
    nlohmann::json action = getDefaultUploadObject();
    action["expiration"] = 0;
    nlohmann::json response = uploadFileAction.run(action.dump());

    EXPECT_EQ(response["result"],4);
    EXPECT_EQ(response["errorMessage"],"Upload file action has expired");
}

TEST_F(UploadFileTests, actionExpiryLarge)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    const std::string expectedMsg = "Upload file action has expired";
    ResponseActionsImpl::UploadFileAction uploadFileAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);

    std::string action (
        R"({"type": "sophos.mgt.action.UploadFile"
        ,"timeout": 1000
        ,"maxUploadSizeBytes": 1000
        ,"url": "https://s3.com/download.zip"
        ,"targetFile": "path"
        ,"expiration": 18446744073709551616})");

    nlohmann::json response = uploadFileAction.run(action);

    EXPECT_EQ(response["type"], UPLOAD_FILE_RESPONSE_TYPE);
    EXPECT_EQ(response["result"], ResponseResult::EXPIRED);
    EXPECT_EQ(response["errorMessage"], expectedMsg);
    EXPECT_FALSE(response.contains("errorType"));
    EXPECT_FALSE(response.contains("httpStatus"));
    EXPECT_FALSE(response.contains("duration"));

    EXPECT_TRUE(appenderContains(expectedMsg));
}

TEST_F(UploadFileTests, actionExpiryNegative)
{
    ResponseActionsImpl::UploadFileAction uploadFileAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);

    nlohmann::json action = getDefaultUploadObject();
    action["expiration"] = -123456;
    nlohmann::json response = uploadFileAction.run(action.dump());

    EXPECT_EQ(response["type"], UPLOAD_FILE_RESPONSE_TYPE);
    EXPECT_EQ(response["result"], ResponseResult::ERROR);
    EXPECT_EQ(response["errorMessage"], "Error parsing command from Central");
    EXPECT_FALSE(response.contains("errorType"));
    EXPECT_FALSE(response.contains("httpStatus"));
    EXPECT_FALSE(response.contains("duration"));
}

TEST_F(UploadFileTests, FileDoesNotExist)
{
    ResponseActionsImpl::UploadFileAction uploadFileAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);
    nlohmann::json action = getDefaultUploadObject();

    EXPECT_CALL(*m_mockFileSystem, isFile(m_testPath)).WillOnce(Return(false));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    nlohmann::json response = uploadFileAction.run(action.dump());

    EXPECT_EQ(response["result"],1);
    EXPECT_EQ(response["errorType"],"invalid_path");
    EXPECT_EQ(response["errorMessage"],"/path is not a file");
}

TEST_F(UploadFileTests, emptyPath)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    const std::string expectedMsg = "Invalid command format. Failed to process UploadInfo from action JSON: targetFile field is empty";

    ResponseActionsImpl::UploadFileAction uploadFileAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);
    nlohmann::json action = getDefaultUploadObject();
    action["targetFile"] = "";

    nlohmann::json response = uploadFileAction.run(action.dump());

    EXPECT_EQ(response["result"], ResponseResult::ERROR);
    EXPECT_EQ(response["errorMessage"], "Error parsing command from Central");
    EXPECT_FALSE(response.contains("errorType"));

    EXPECT_TRUE(appenderContains(expectedMsg));
}

TEST_F(UploadFileTests, SizeLimitSetToZero)
{
    ResponseActionsImpl::UploadFileAction uploadFileAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);
    nlohmann::json action = getDefaultUploadObject();
    action.at("maxUploadSizeBytes") = 0;

    EXPECT_CALL(*m_mockFileSystem, isFile(m_testPath)).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, fileSize(m_testPath)).WillOnce(Return(1));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    nlohmann::json response = uploadFileAction.run(action.dump());

    EXPECT_EQ(response["result"],1);
    EXPECT_EQ(response["errorType"],"exceed_size_limit");
    EXPECT_EQ(response["errorMessage"],"File at path /path is size 1 bytes which is above the size limit 0 bytes");
}

TEST_F(UploadFileTests, NegativeSizeLimit)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    ResponseActionsImpl::UploadFileAction uploadFileAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);
    nlohmann::json action = getDefaultUploadObject();
    action.at("maxUploadSizeBytes") = -123545;

    nlohmann::json response = uploadFileAction.run(action.dump());

    EXPECT_EQ(response["result"],1);
    EXPECT_EQ(response["result"], ResponseResult::ERROR);
    EXPECT_EQ(response["errorMessage"], "Error parsing command from Central");

    EXPECT_TRUE(appenderContains("Invalid command format. Failed to process UploadInfo from action JSON: maxUploadSizeBytes is a negative value: -123545"));
}

TEST_F(UploadFileTests, LargeSizeLimit)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    ResponseActionsImpl::UploadFileAction uploadFileAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);
    nlohmann::json action = getDefaultUploadObject();
    action.at("maxUploadSizeBytes") = 214748364734;

    nlohmann::json response = uploadFileAction.run(action.dump());

    EXPECT_EQ(response["result"],1);
    EXPECT_EQ(response["result"], ResponseResult::ERROR);
    EXPECT_EQ(response["errorMessage"], "Error parsing command from Central");

    EXPECT_TRUE(appenderContains("Invalid command format. Failed to process UploadInfo from action JSON: maxUploadSizeBytes is to large: 214748364734"));
}

TEST_F(UploadFileTests, FileOverSizeLimit)
{
    ResponseActionsImpl::UploadFileAction uploadFileAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);
    nlohmann::json action = getDefaultUploadObject();

    EXPECT_CALL(*m_mockFileSystem, isFile(m_testPath)).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, fileSize(m_testPath)).WillOnce(Return(10000));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    nlohmann::json response = uploadFileAction.run(action.dump());

    EXPECT_EQ(response["result"],1);
    EXPECT_EQ(response["errorType"],"exceed_size_limit");
    EXPECT_EQ(response["errorMessage"],"File at path /path is size 10000 bytes which is above the size limit 1000 bytes");
}

TEST_F(UploadFileTests, FileBeingWrittenToAndOverSizeLimit)
{
    ResponseActionsImpl::UploadFileAction uploadFileAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);
    nlohmann::json action = getDefaultUploadObject();

    EXPECT_CALL(*m_mockFileSystem, isFile(m_testPath)).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, fileSize(m_testPath)).WillOnce(Return(100));
    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256,m_testPath)).WillOnce(Throw(Common::FileSystem::IFileSystemException("")));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    nlohmann::json response = uploadFileAction.run(action.dump());

    EXPECT_EQ(response["result"],1);
    EXPECT_EQ(response["errorType"],"access_denied");
    EXPECT_EQ(response["errorMessage"],"File to be uploaded cannot be accessed");
}

TEST_F(UploadFileTests, FailureDueToTimeout)
{
    addResponseToMockRequester(500, ResponseErrorCode::TIMEOUT);
    ResponseActionsImpl::UploadFileAction uploadFileAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);

    nlohmann::json action = getDefaultUploadObject();

    EXPECT_CALL(*m_mockFileSystem, isFile(m_testPath)).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, fileSize(m_testPath)).WillOnce(Return(100));
    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, m_testPath))
        .WillOnce(Return("sha256string"));
    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    nlohmann::json response = uploadFileAction.run(action.dump());

    EXPECT_EQ(response["result"], ResponseResult::TIMEOUT);
    EXPECT_EQ(response["httpStatus"], 500);
    EXPECT_EQ(response["errorMessage"], "Timeout Uploading file: path");
}

TEST_F(UploadFileTests, FailureDueToNetworkError)
{
    addResponseToMockRequester(500, ResponseErrorCode::FAILED, "failure");
    ResponseActionsImpl::UploadFileAction uploadFileAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);

    nlohmann::json action = getDefaultUploadObject();

    EXPECT_CALL(*m_mockFileSystem, isFile(m_testPath)).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, fileSize(m_testPath)).WillOnce(Return(100));
    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, m_testPath))
        .WillOnce(Return("sha256string"));
    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    nlohmann::json response = uploadFileAction.run(action.dump());

    EXPECT_EQ(response["result"], ResponseResult::ERROR);
    EXPECT_EQ(response["httpStatus"], 500);
    EXPECT_EQ(response["errorMessage"], "Failed to upload file: path with error: failure");
    EXPECT_EQ(response["errorType"], "network_error");
}

TEST_F(UploadFileTests, FailureDueToServerError)
{
    addResponseToMockRequester(500, ResponseErrorCode::OK);
    ResponseActionsImpl::UploadFileAction uploadFileAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);

    nlohmann::json action = getDefaultUploadObject();

    EXPECT_CALL(*m_mockFileSystem, isFile(m_testPath)).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, fileSize(m_testPath)).WillOnce(Return(100));
    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, m_testPath))
        .WillOnce(Return("sha256string"));
    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    nlohmann::json response = uploadFileAction.run(action.dump());

    EXPECT_EQ(response["result"], ResponseResult::ERROR);
    EXPECT_EQ(response["httpStatus"], 500);
    EXPECT_EQ(response["errorMessage"], "Failed to upload file: path with http error code 500");
    EXPECT_EQ(response["errorType"], "network_error");
}

TEST_F(UploadFileTests, FailureDueToServerCertError)
{
    addResponseToMockRequester(100, ResponseErrorCode::CERTIFICATE_ERROR, "SSL issues");
    ResponseActionsImpl::UploadFileAction uploadFileAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);
    nlohmann::json action = getDefaultUploadObject();

    EXPECT_CALL(*m_mockFileSystem, isFile(m_testPath)).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, fileSize(m_testPath)).WillOnce(Return(100));
    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, m_testPath))
        .WillOnce(Return("sha256string"));
    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    nlohmann::json response = uploadFileAction.run(action.dump());

    EXPECT_EQ(response["result"], ResponseResult::ERROR);
    EXPECT_EQ(response["httpStatus"], 100);
    EXPECT_EQ(response["errorMessage"], "Failed to upload file: path with error: SSL issues");
    EXPECT_EQ(response["errorType"], "network_error");
}

TEST_F(UploadFileTests, terminateSignal)
{
    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, _, _, _)).WillOnce(pollReturnsWithRevents(0, POLLIN));

    addResponseToMockRequester(26, ResponseErrorCode::OK);
    EXPECT_CALL(*m_mockHttpRequester, sendTerminate());
    ResponseActionsImpl::UploadFileAction uploadFileAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);

    nlohmann::json action = getDefaultUploadObject();

    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_testPath)).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, fileSize(m_testPath)).WillOnce(Return(100));
    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, m_testPath))
            .WillOnce(Return("sha256string"));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    nlohmann::json response = uploadFileAction.run(action.dump());

    EXPECT_EQ(response["result"], ResponseResult::ERROR);
    EXPECT_EQ(response["httpStatus"], 26);
}

TEST_F(UploadFileTests, timeoutSignal)
{
    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, _, _, _)).WillOnce(pollReturnsWithRevents(1, POLLIN));

    addResponseToMockRequester(26, ResponseErrorCode::OK);
    EXPECT_CALL(*m_mockHttpRequester, sendTerminate());
    ResponseActionsImpl::UploadFileAction uploadFileAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);

    nlohmann::json action = getDefaultUploadObject();

    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_testPath)).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, fileSize(m_testPath)).WillOnce(Return(100));
    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, m_testPath))
            .WillOnce(Return("sha256string"));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    nlohmann::json response = uploadFileAction.run(action.dump());

    EXPECT_EQ(response["result"], ResponseResult::TIMEOUT);
    EXPECT_EQ(response["httpStatus"], 26);
}

TEST_F(UploadFileTests, PpollErrorContinueWhenEINTR)
{
    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, _, _, _))
            .WillOnce(SetErrnoAndReturn(EINTR, -1))
            .WillOnce(pollReturnsWithRevents(1, POLLIN));

    addResponseToMockRequester(26, ResponseErrorCode::OK);
    EXPECT_CALL(*m_mockHttpRequester, sendTerminate());
    ResponseActionsImpl::UploadFileAction uploadFileAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);

    nlohmann::json action = getDefaultUploadObject();

    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_testPath)).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, fileSize(m_testPath)).WillOnce(Return(100));
    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, m_testPath))
            .WillOnce(Return("sha256string"));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    nlohmann::json response = uploadFileAction.run(action.dump());

    EXPECT_EQ(response["result"], ResponseResult::TIMEOUT);
    EXPECT_EQ(response["httpStatus"], 26);
}