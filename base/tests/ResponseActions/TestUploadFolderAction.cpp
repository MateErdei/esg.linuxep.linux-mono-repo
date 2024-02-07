// Copyright 2023-2024 Sophos Limited. All rights reserved.

#include "Common/FileSystem/IFileTooLargeException.h"
#include "Common/ZipUtilities/ZipUtils.h"
#include "ResponseActions/RACommon/ResponseActionsCommon.h"
#include "ResponseActions/ResponseActionsImpl/UploadFolderAction.h"
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

using namespace Common::ApplicationConfiguration;
using namespace Common::HttpRequests;
using namespace ResponseActions::RACommon;

class UploadFolderTests : public MemoryAppenderUsingTests
{
public:
    UploadFolderTests() : MemoryAppenderUsingTests("ResponseActionsImpl") {}

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
        Common::ApplicationConfiguration::restoreApplicationPathManager();
        Common::ZipUtilities::restoreZipUtils();
    }
    nlohmann::json getDefaultUploadObject()
    {
        nlohmann::json action;
        action["targetFolder"] = m_targetFolder;
        action["url"] = "https://s3.com/somewhere";
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

    std::shared_ptr<MockHTTPRequester> m_mockHttpRequester;
    std::unique_ptr<MockFileSystem> m_mockFileSystem;

    const std::string m_targetFolder = "/tmp/path";
    const std::string m_defaultZipFile = "/opt/sophos-spl/plugins/responseactions/tmp/path.zip";
    const std::string m_testURL = "https://s3.com/somewhere";
};

TEST_F(UploadFolderTests, SuccessCaseWithPassword)
{
    addResponseToMockRequester(HTTP_STATUS_OK, ResponseErrorCode::OK);

    std::string tempDir = "/tmp";
    std::string folderPath = "/tmp/test/";
    std::string zipFile = "/tmp/test.zip";

    ResponseActionsImpl::UploadFolderAction uploadFolderAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);
    nlohmann::json action = getDefaultUploadObject();
    action["password"] = "password";
    action["targetFolder"] = folderPath;

    auto mockAppManager = std::make_unique<NiceMock<MockedApplicationPathManager>>();
    ON_CALL(*mockAppManager, getResponseActionTmpPath()).WillByDefault(Return(tempDir));
    replaceApplicationPathManager(std::move(mockAppManager));

    auto mockZip = std::make_unique<NiceMock<MockZipUtils>>();
    EXPECT_CALL(*mockZip, zip(_, _, _, _, _)).WillOnce(Return(0));
    Common::ZipUtilities::replaceZipUtils(std::move(mockZip));

    EXPECT_CALL(*m_mockFileSystem, isDirectory(folderPath)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, removeFile(zipFile)).Times(1);
    EXPECT_CALL(*m_mockFileSystem, isFile(zipFile)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, isFile("")).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, fileSize(zipFile)).WillOnce(Return(8));
    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, zipFile))
        .WillOnce(Return("sha256string"));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    nlohmann::json response = uploadFolderAction.run(action.dump());

    EXPECT_EQ(response["result"], ResponseResult::SUCCESS);
    EXPECT_EQ(response["httpStatus"], 200);
}

TEST_F(UploadFolderTests, SuccessCase)
{
    addResponseToMockRequester(HTTP_STATUS_OK, ResponseErrorCode::OK);
    setupMockZipUtils();

    ResponseActionsImpl::UploadFolderAction uploadFolderAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);
    nlohmann::json action = getDefaultUploadObject();

    EXPECT_CALL(*m_mockFileSystem, isDirectory("/tmp/path")).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, fileSize(m_defaultZipFile)).WillOnce(Return(100));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_defaultZipFile)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, removeFile(m_defaultZipFile)).Times(1);
    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, m_defaultZipFile))
        .WillOnce(Return("sha256string"));
    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    nlohmann::json response = uploadFolderAction.run(action.dump());

    EXPECT_EQ(response["result"], ResponseResult::SUCCESS);
    EXPECT_EQ(response["httpStatus"], 200);
}

TEST_F(UploadFolderTests, successHugeURL)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    const std::string largeStr(30000, 'a');
    const std::string largeURL("https://s3.com/download" + largeStr + ".zip");
    const std::string expectedMsg("Uploading folder: " + m_targetFolder
                                  + " as zip file: " + m_defaultZipFile + " to url: " + largeURL);

    addResponseToMockRequester(HTTP_STATUS_OK, ResponseErrorCode::OK);
    setupMockZipUtils();

    ResponseActionsImpl::UploadFolderAction uploadFolderAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);

    EXPECT_CALL(*m_mockFileSystem, isDirectory("/tmp/path")).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, fileSize(m_defaultZipFile)).WillOnce(Return(100));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_defaultZipFile)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, removeFile(m_defaultZipFile)).Times(1);
    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, m_defaultZipFile))
        .WillOnce(Return("sha256string"));
    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    nlohmann::json action = getDefaultUploadObject();
    action["url"] = largeURL;
    nlohmann::json response = uploadFolderAction.run(action.dump());

    EXPECT_EQ(response["result"], ResponseResult::SUCCESS);
    EXPECT_EQ(response["httpStatus"], 200);

    EXPECT_EQ(response["type"], UPLOAD_FOLDER_RESPONSE_TYPE);
    EXPECT_EQ(response["result"], ResponseResult::SUCCESS);
    EXPECT_EQ(response["httpStatus"], HTTP_STATUS_OK);
    EXPECT_FALSE(response.contains("errorType"));
    EXPECT_FALSE(response.contains("errorMessage"));
    EXPECT_TRUE(response.contains("duration"));

    EXPECT_TRUE(appenderContains(expectedMsg));
}

TEST_F(UploadFolderTests, successHugePath)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    const std::string largeStr(30000, 'a');
    const std::string largeTargetPath(m_targetFolder + largeStr + "/");
    const std::string zipfile("/opt/sophos-spl/plugins/responseactions/tmp/path" + largeStr + ".zip");
    const std::string expectedMsg("Uploading folder: " + largeTargetPath + " as zip file: " + zipfile + " to url: " + m_testURL);
    PRINT(expectedMsg);
    addResponseToMockRequester(HTTP_STATUS_OK, ResponseErrorCode::OK);
    setupMockZipUtils();

    ResponseActionsImpl::UploadFolderAction uploadFolderAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);

    EXPECT_CALL(*m_mockFileSystem, isDirectory(largeTargetPath)).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, isFile(zipfile)).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, fileSize(zipfile)).WillOnce(Return(100));
    EXPECT_CALL(*m_mockFileSystem, removeFile(zipfile)).Times(1);
    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, zipfile))
        .WillOnce(Return("sha256string"));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    nlohmann::json action = getDefaultUploadObject();
    action["targetFolder"] = largeTargetPath;
    nlohmann::json response = uploadFolderAction.run(action.dump());

    EXPECT_EQ(response["type"], UPLOAD_FOLDER_RESPONSE_TYPE);
    EXPECT_EQ(response["result"], ResponseResult::SUCCESS);
    EXPECT_EQ(response["httpStatus"], HTTP_STATUS_OK);
    EXPECT_FALSE(response.contains("errorType"));
    EXPECT_FALSE(response.contains("errorMessage"));
    EXPECT_TRUE(response.contains("duration"));

    EXPECT_TRUE(appenderContains(expectedMsg));
}

TEST_F(UploadFolderTests, successEmptyPassword)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    addResponseToMockRequester(HTTP_STATUS_OK, ResponseErrorCode::OK);

    auto mockZip = std::make_unique<NiceMock<MockZipUtils>>();
    EXPECT_CALL(*mockZip, zip(_, _, _)).WillOnce(Return(UNZ_OK));
    EXPECT_CALL(*mockZip, zip(_, _, _, _, _)).Times(0);
    Common::ZipUtilities::replaceZipUtils(std::move(mockZip));

    ResponseActionsImpl::UploadFolderAction uploadFolderAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);

    EXPECT_CALL(*m_mockFileSystem, isDirectory("/tmp/path")).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, fileSize(m_defaultZipFile)).WillOnce(Return(100));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_defaultZipFile)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, removeFile(m_defaultZipFile)).Times(1);
    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, m_defaultZipFile))
        .WillOnce(Return("sha256string"));
    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    nlohmann::json action = getDefaultUploadObject();
    action["password"] = "";
    nlohmann::json response = uploadFolderAction.run(action.dump());

    EXPECT_EQ(response["result"], ResponseResult::SUCCESS);
    EXPECT_EQ(response["httpStatus"], 200);

    EXPECT_EQ(response["type"], UPLOAD_FOLDER_RESPONSE_TYPE);
    EXPECT_EQ(response["result"], ResponseResult::SUCCESS);
    EXPECT_EQ(response["httpStatus"], HTTP_STATUS_OK);
    EXPECT_FALSE(response.contains("errorType"));
    EXPECT_FALSE(response.contains("errorMessage"));
    EXPECT_TRUE(response.contains("duration"));

    EXPECT_TRUE(appenderContains("Upload for /opt/sophos-spl/plugins/responseactions/tmp/path.zip succeeded"));
}

TEST_F(UploadFolderTests, successHugePassword)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    const std::string largePassword(30000, 'a');

    addResponseToMockRequester(HTTP_STATUS_OK, ResponseErrorCode::OK);

    auto mockZip = std::make_unique<NiceMock<MockZipUtils>>();
    EXPECT_CALL(*mockZip, zip(_, _, _)).Times(0);
    EXPECT_CALL(*mockZip, zip(_, _, _, _, _)).WillOnce(Return(UNZ_OK));
    Common::ZipUtilities::replaceZipUtils(std::move(mockZip));

    ResponseActionsImpl::UploadFolderAction uploadFolderAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);

    EXPECT_CALL(*m_mockFileSystem, isDirectory("/tmp/path")).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, fileSize(m_defaultZipFile)).WillOnce(Return(100));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_defaultZipFile)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, removeFile(m_defaultZipFile)).Times(1);
    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, m_defaultZipFile))
        .WillOnce(Return("sha256string"));
    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    nlohmann::json action = getDefaultUploadObject();
    action["password"] = largePassword;
    nlohmann::json response = uploadFolderAction.run(action.dump());

    EXPECT_EQ(response["result"], ResponseResult::SUCCESS);
    EXPECT_EQ(response["httpStatus"], 200);

    EXPECT_EQ(response["type"], UPLOAD_FOLDER_RESPONSE_TYPE);
    EXPECT_EQ(response["result"], ResponseResult::SUCCESS);
    EXPECT_EQ(response["httpStatus"], HTTP_STATUS_OK);
    EXPECT_FALSE(response.contains("errorType"));
    EXPECT_FALSE(response.contains("errorMessage"));
    EXPECT_TRUE(response.contains("duration"));

    EXPECT_TRUE(appenderContains("Upload for /opt/sophos-spl/plugins/responseactions/tmp/path.zip succeeded"));
}

TEST_F(UploadFolderTests, SuccessCaseWithProxy)
{
    addResponseToMockRequester(HTTP_STATUS_OK, ResponseErrorCode::OK);
    setupMockZipUtils();

    ResponseActionsImpl::UploadFolderAction uploadFolderAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);
    nlohmann::json action = getDefaultUploadObject();

    EXPECT_CALL(*m_mockFileSystem, isDirectory("/tmp/path")).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, fileSize(m_defaultZipFile)).WillOnce(Return(100));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_defaultZipFile)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, removeFile(m_defaultZipFile)).Times(1);
    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, m_defaultZipFile))
        .WillOnce(Return("sha256string"));
    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(true));
    std::string obfuscatedCreds =
        "CCD4E57ZjW+t5XPiMSJH1TurG3MfWCN3DpjJRINMwqNaWl+3zzlVIdyVmifCHUwcmaX6+YTSyyBM8SslIIGV5rUw";
    std::string content = R"({"proxy":"localhost","credentials":")" + obfuscatedCreds + R"("})";
    EXPECT_CALL(*m_mockFileSystem, readFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy"))
        .WillOnce(Return(content));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    nlohmann::json response = uploadFolderAction.run(action.dump());
    
    EXPECT_EQ(response["result"], ResponseResult::SUCCESS);
    EXPECT_EQ(response["httpStatus"], 200);
}

TEST_F(UploadFolderTests, ProxyFallsBackToDirect)
{
    Common::HttpRequests::Response failhttpresponse;
    Common::HttpRequests::Response httpresponse;
    httpresponse.status = Common::HttpRequests::HTTP_STATUS_OK;
    httpresponse.errorCode = Common::HttpRequests::ResponseErrorCode::OK;
    failhttpresponse.status = 400;
    failhttpresponse.errorCode = Common::HttpRequests::ResponseErrorCode::OK;
    EXPECT_CALL(*m_mockHttpRequester, put(_)).Times(2).WillOnce(Return(failhttpresponse)).WillOnce(Return(httpresponse));
    setupMockZipUtils();

    ResponseActionsImpl::UploadFolderAction uploadFolderAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);
    nlohmann::json action = getDefaultUploadObject();

    EXPECT_CALL(*m_mockFileSystem, isDirectory("/tmp/path")).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, fileSize(m_defaultZipFile)).WillOnce(Return(100));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_defaultZipFile)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, removeFile(m_defaultZipFile)).Times(1);
    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, m_defaultZipFile))
        .WillOnce(Return("sha256string"));
    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(true));
    std::string obfuscatedCreds =
        "CCD4E57ZjW+t5XPiMSJH1TurG3MfWCN3DpjJRINMwqNaWl+3zzlVIdyVmifCHUwcmaX6+YTSyyBM8SslIIGV5rUw";
    std::string content = R"({"proxy":"localhost","credentials":")" + obfuscatedCreds + R"("})";
    EXPECT_CALL(*m_mockFileSystem, readFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy"))
        .WillOnce(Return(content));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    nlohmann::json response = uploadFolderAction.run(action.dump());
    
    EXPECT_EQ(response["result"], ResponseResult::SUCCESS);
    EXPECT_EQ(response["httpStatus"], 200);
}

TEST_F(UploadFolderTests, cannotParseActions)
{
    ResponseActionsImpl::UploadFolderAction uploadFolderAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);

    nlohmann::json response = uploadFolderAction.run("");
    
    EXPECT_EQ(response["result"], ResponseResult::ERROR);
    EXPECT_EQ(response["errorMessage"], "Error parsing command from Central");
}

TEST_F(UploadFolderTests, actionExpired)
{
    ResponseActionsImpl::UploadFolderAction uploadFolderAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);
    nlohmann::json action = getDefaultUploadObject();
    action["expiration"] = 0;
    nlohmann::json response = uploadFolderAction.run(action.dump());
    
    EXPECT_EQ(response["result"], ResponseResult::EXPIRED);
    EXPECT_EQ(response["errorMessage"], "Upload folder action has expired");
}

TEST_F(UploadFolderTests, actionExpiryLarge)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    const std::string expectedMsg = "Upload folder action has expired";
    ResponseActionsImpl::UploadFolderAction uploadFolderAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);

    std::string action (
        R"({"type": "sophos.mgt.action.UploadFolder"
        ,"timeout": 1000
        ,"maxUploadSizeBytes": 1000
        ,"url": "https://s3.com/download.zip"
        ,"targetFolder": "path"
        ,"expiration": 18446744073709551616})");

    nlohmann::json response = uploadFolderAction.run(action);

    EXPECT_EQ(response["type"], UPLOAD_FOLDER_RESPONSE_TYPE);
    EXPECT_EQ(response["result"], ResponseResult::EXPIRED);
    EXPECT_EQ(response["errorMessage"], expectedMsg);
    EXPECT_FALSE(response.contains("errorType"));
    EXPECT_FALSE(response.contains("httpStatus"));
    EXPECT_FALSE(response.contains("duration"));

    EXPECT_TRUE(appenderContains(expectedMsg));
}

TEST_F(UploadFolderTests, actionExpiryNegative)
{
    ResponseActionsImpl::UploadFolderAction uploadFolderAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);

    nlohmann::json action = getDefaultUploadObject();
    action["expiration"] = -123456;
    nlohmann::json response = uploadFolderAction.run(action.dump());

    EXPECT_EQ(response["type"], UPLOAD_FOLDER_RESPONSE_TYPE);
    EXPECT_EQ(response["result"], ResponseResult::ERROR);
    EXPECT_EQ(response["errorMessage"], "Error parsing command from Central");
    EXPECT_FALSE(response.contains("errorType"));
    EXPECT_FALSE(response.contains("httpStatus"));
    EXPECT_FALSE(response.contains("duration"));
}

TEST_F(UploadFolderTests, FolderDoesNotExist)
{
    ResponseActionsImpl::UploadFolderAction uploadFolderAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);
    nlohmann::json action = getDefaultUploadObject();

    EXPECT_CALL(*m_mockFileSystem, isDirectory("/tmp/path")).WillOnce(Return(false));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    nlohmann::json response = uploadFolderAction.run(action.dump());
    
    EXPECT_EQ(response["result"], ResponseResult::ERROR);
    EXPECT_EQ(response["errorType"], "invalid_path");
    EXPECT_EQ(response["errorMessage"], "/tmp/path is not a directory");
}

TEST_F(UploadFolderTests, NegativeSizeLimit)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    ResponseActionsImpl::UploadFolderAction uploadFolderAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);
    nlohmann::json action = getDefaultUploadObject();
    action.at("maxUploadSizeBytes") = -123545;

    nlohmann::json response = uploadFolderAction.run(action.dump());

    EXPECT_EQ(response["result"],1);
    EXPECT_EQ(response["result"], ResponseResult::ERROR);
    EXPECT_EQ(response["errorMessage"], "Error parsing command from Central");

    EXPECT_TRUE(appenderContains("Invalid command format. Failed to process UploadInfo from action JSON: maxUploadSizeBytes is a negative value: -123545"));
}

TEST_F(UploadFolderTests, LargeSizeLimit)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    ResponseActionsImpl::UploadFolderAction uploadFolderAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);
    nlohmann::json action = getDefaultUploadObject();
    action.at("maxUploadSizeBytes") = 214748364734;

    nlohmann::json response = uploadFolderAction.run(action.dump());

    EXPECT_EQ(response["result"],1);
    EXPECT_EQ(response["result"], ResponseResult::ERROR);
    EXPECT_EQ(response["errorMessage"], "Error parsing command from Central");

    EXPECT_TRUE(appenderContains("Invalid command format. Failed to process UploadInfo from action JSON: maxUploadSizeBytes is to large: 214748364734"));
}

TEST_F(UploadFolderTests, emptyPath)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    const std::string expectedMsg = "Invalid command format. Failed to process UploadInfo from action JSON: targetFolder field is empty";

    ResponseActionsImpl::UploadFolderAction uploadFolderAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);
    nlohmann::json action = getDefaultUploadObject();
    action["targetFolder"] = "";

    nlohmann::json response = uploadFolderAction.run(action.dump());

    EXPECT_EQ(response["result"],1);
    EXPECT_EQ(response["errorMessage"], "Error parsing command from Central");
    EXPECT_FALSE(response.contains("errorType"));

    EXPECT_TRUE(appenderContains(expectedMsg));
}

TEST_F(UploadFolderTests, ZippedFolderOverSizeLimit)
{
    setupMockZipUtils();

    ResponseActionsImpl::UploadFolderAction uploadFolderAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);
    nlohmann::json action = getDefaultUploadObject();

    EXPECT_CALL(*m_mockFileSystem, isDirectory("/tmp/path")).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, fileSize(m_defaultZipFile)).WillOnce(Return(10000));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_defaultZipFile)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, removeFile(m_defaultZipFile)).Times(1);
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    nlohmann::json response = uploadFolderAction.run(action.dump());
    
    EXPECT_EQ(response["result"], ResponseResult::ERROR);
    EXPECT_EQ(response["errorType"], "exceed_size_limit");
    EXPECT_EQ(
        response["errorMessage"],
        "Folder at path /tmp/path after being compressed is size 10000 bytes which is above the size limit 1000 bytes");
}

TEST_F(UploadFolderTests, ZippedFolderOverSizeLimitNotSet)
{
    setupMockZipUtils();

    ResponseActionsImpl::UploadFolderAction uploadFolderAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);
    nlohmann::json action = getDefaultUploadObject();
    action.at("maxUploadSizeBytes") = 0;

    EXPECT_CALL(*m_mockFileSystem, isDirectory("/tmp/path")).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, fileSize(m_defaultZipFile)).WillOnce(Return(1));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_defaultZipFile)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, removeFile(m_defaultZipFile)).Times(1);
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    nlohmann::json response = uploadFolderAction.run(action.dump());

    EXPECT_EQ(response["result"], ResponseResult::ERROR);
    EXPECT_EQ(response["errorType"], "exceed_size_limit");
    EXPECT_EQ(
        response["errorMessage"],
        "Folder at path /tmp/path after being compressed is size 1 bytes which is above the size limit 0 bytes");
}

TEST_F(UploadFolderTests, ZipFails)
{
    setupMockZipUtils(UNZ_BADZIPFILE);

    ResponseActionsImpl::UploadFolderAction uploadFolderAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);
    nlohmann::json action = getDefaultUploadObject();

    EXPECT_CALL(*m_mockFileSystem, isDirectory("/tmp/path")).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_defaultZipFile)).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, removeFile(m_defaultZipFile)).Times(1);
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    nlohmann::json response = uploadFolderAction.run(action.dump());
    
    EXPECT_EQ(response["result"], ResponseResult::INTERNAL_ERROR);
    EXPECT_EQ(response["errorMessage"], "Error zipping /tmp/path");
}

TEST_F(UploadFolderTests, ZipFailsDueToLargeFile)
{
    ResponseActionsImpl::UploadFolderAction uploadFolderAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);
    nlohmann::json action = getDefaultUploadObject();

    EXPECT_CALL(*m_mockFileSystem, isDirectory("/tmp/path")).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_defaultZipFile)).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, removeFile(m_defaultZipFile)).Times(1);
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    auto mockZip = std::make_unique<NiceMock<MockZipUtils>>();
    ON_CALL(*mockZip, zip(_, _, _)).WillByDefault(Throw(std::runtime_error("")));
    Common::ZipUtilities::replaceZipUtils(std::move(mockZip));

    nlohmann::json response = uploadFolderAction.run(action.dump());
    
    EXPECT_EQ(response["result"], ResponseResult::INTERNAL_ERROR);
    EXPECT_EQ(response["errorMessage"], "Error zipping /tmp/path");
}

TEST_F(UploadFolderTests, FileBeingWrittenToAndOverSizeLimit)
{
    setupMockZipUtils();

    ResponseActionsImpl::UploadFolderAction uploadFolderAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);
    nlohmann::json action = getDefaultUploadObject();

    EXPECT_CALL(*m_mockFileSystem, isDirectory("/tmp/path")).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, fileSize(m_defaultZipFile)).WillOnce(Return(100));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_defaultZipFile)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, removeFile(m_defaultZipFile)).Times(1);
    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, m_defaultZipFile))
        .WillOnce(Throw(Common::FileSystem::IFileSystemException("")));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    nlohmann::json response = uploadFolderAction.run(action.dump());
    
    EXPECT_EQ(response["result"], ResponseResult::ERROR);
    EXPECT_EQ(response["errorType"], "access_denied");
    EXPECT_EQ(response["errorMessage"], "Zip file to be uploaded cannot be accessed");
}

TEST_F(UploadFolderTests, FailureDueToTimeout)
{
    addResponseToMockRequester(500, ResponseErrorCode::TIMEOUT);
    setupMockZipUtils();
    ResponseActionsImpl::UploadFolderAction uploadFolderAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);

    nlohmann::json action = getDefaultUploadObject();

    EXPECT_CALL(*m_mockFileSystem, isDirectory("/tmp/path")).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, fileSize(m_defaultZipFile)).WillOnce(Return(100));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_defaultZipFile)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, removeFile(m_defaultZipFile)).Times(1);
    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, m_defaultZipFile))
        .WillOnce(Return("sha256string"));
    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    nlohmann::json response = uploadFolderAction.run(action.dump());
    
    EXPECT_EQ(response["result"], ResponseResult::TIMEOUT);
    EXPECT_EQ(response["httpStatus"], 500);
    EXPECT_EQ(response["errorMessage"], "Timeout uploading zip file: " + m_defaultZipFile);
}

TEST_F(UploadFolderTests, FailureDueToNetworkError)
{
    addResponseToMockRequester(500, ResponseErrorCode::FAILED, "failure");
    setupMockZipUtils();
    ResponseActionsImpl::UploadFolderAction uploadFolderAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);

    nlohmann::json action = getDefaultUploadObject();

    EXPECT_CALL(*m_mockFileSystem, isDirectory("/tmp/path")).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_defaultZipFile)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, removeFile(m_defaultZipFile)).Times(1);
    EXPECT_CALL(*m_mockFileSystem, fileSize(m_defaultZipFile)).WillOnce(Return(100));
    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, m_defaultZipFile))
        .WillOnce(Return("sha256string"));
    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    nlohmann::json response = uploadFolderAction.run(action.dump());
    
    EXPECT_EQ(response["result"], ResponseResult::ERROR);
    EXPECT_EQ(response["httpStatus"], 500);
    EXPECT_EQ(response["errorMessage"], "Failed to upload zip file: " + m_defaultZipFile + " with error: failure");
    EXPECT_EQ(response["errorType"], "network_error");
}

TEST_F(UploadFolderTests, FailureDueToServerError)
{
    addResponseToMockRequester(500, ResponseErrorCode::OK);
    setupMockZipUtils();
    ResponseActionsImpl::UploadFolderAction uploadFolderAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);

    nlohmann::json action = getDefaultUploadObject();

    EXPECT_CALL(*m_mockFileSystem, isDirectory("/tmp/path")).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, fileSize(m_defaultZipFile)).WillOnce(Return(100));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_defaultZipFile)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, removeFile(m_defaultZipFile)).Times(1);
    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, m_defaultZipFile))
        .WillOnce(Return("sha256string"));
    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    nlohmann::json response = uploadFolderAction.run(action.dump());
    
    EXPECT_EQ(response["result"], ResponseResult::ERROR);
    EXPECT_EQ(response["httpStatus"], 500);
    EXPECT_EQ(response["errorMessage"], "Failed to upload zip file: " + m_defaultZipFile + " with http error code 500");
    EXPECT_EQ(response["errorType"], "network_error");
}

TEST_F(UploadFolderTests, FailureDueToServerCertError)
{
    addResponseToMockRequester(500, ResponseErrorCode::CERTIFICATE_ERROR, "SSL issues");
    ResponseActionsImpl::UploadFolderAction uploadFolderAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);
    nlohmann::json action = getDefaultUploadObject();
    setupMockZipUtils();

    EXPECT_CALL(*m_mockFileSystem, isDirectory("/tmp/path")).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, fileSize(m_defaultZipFile)).WillOnce(Return(100));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_defaultZipFile)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, removeFile(m_defaultZipFile)).WillOnce(Return());
    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, m_defaultZipFile))
        .WillOnce(Return("sha256string"));
    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    nlohmann::json response = uploadFolderAction.run(action.dump());

    EXPECT_EQ(response["result"], ResponseResult::ERROR);
    EXPECT_EQ(response["httpStatus"], 500);
    EXPECT_EQ(response["errorMessage"], "Failed to upload zip file: " + m_defaultZipFile + " with error: SSL issues");
    EXPECT_EQ(response["errorType"], "network_error");
}

TEST_F(UploadFolderTests, terminateSignal)
{
    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, _, _, _)).WillOnce(pollReturnsWithRevents(0, POLLIN));

    addResponseToMockRequester(26, ResponseErrorCode::OK);
    EXPECT_CALL(*m_mockHttpRequester, sendTerminate());
    setupMockZipUtils();

    ResponseActionsImpl::UploadFolderAction uploadFolderAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);
    nlohmann::json action = getDefaultUploadObject();

    EXPECT_CALL(*m_mockFileSystem, isDirectory("/tmp/path")).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, fileSize(m_defaultZipFile)).WillOnce(Return(100));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_defaultZipFile)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, removeFile(m_defaultZipFile)).Times(1);
    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, m_defaultZipFile))
            .WillOnce(Return("sha256string"));
    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    nlohmann::json response = uploadFolderAction.run(action.dump());

    EXPECT_EQ(response["result"], ResponseResult::ERROR);
    EXPECT_EQ(response["httpStatus"], 26);
}

TEST_F(UploadFolderTests, timeoutSignal)
{
    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, _, _, _)).WillOnce(pollReturnsWithRevents(1, POLLIN));

    addResponseToMockRequester(26, ResponseErrorCode::OK);
    EXPECT_CALL(*m_mockHttpRequester, sendTerminate());
    setupMockZipUtils();

    ResponseActionsImpl::UploadFolderAction uploadFolderAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);
    nlohmann::json action = getDefaultUploadObject();

    EXPECT_CALL(*m_mockFileSystem, isDirectory("/tmp/path")).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, fileSize(m_defaultZipFile)).WillOnce(Return(100));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_defaultZipFile)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, removeFile(m_defaultZipFile)).Times(1);
    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, m_defaultZipFile))
            .WillOnce(Return("sha256string"));
    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    nlohmann::json response = uploadFolderAction.run(action.dump());

    EXPECT_EQ(response["result"], ResponseResult::TIMEOUT);
    EXPECT_EQ(response["httpStatus"], 26);
}

TEST_F(UploadFolderTests, PpollErrorContinueWhenEINTR)
{
    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, _, _, _))
            .WillOnce(SetErrnoAndReturn(EINTR, -1))
            .WillOnce(pollReturnsWithRevents(1, POLLIN));

    addResponseToMockRequester(26, ResponseErrorCode::OK);
    EXPECT_CALL(*m_mockHttpRequester, sendTerminate());
    setupMockZipUtils();

    ResponseActionsImpl::UploadFolderAction uploadFolderAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);
    nlohmann::json action = getDefaultUploadObject();

    EXPECT_CALL(*m_mockFileSystem, isDirectory("/tmp/path")).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, fileSize(m_defaultZipFile)).WillOnce(Return(100));
    EXPECT_CALL(*m_mockFileSystem, isFile(m_defaultZipFile)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, removeFile(m_defaultZipFile)).Times(1);
    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, m_defaultZipFile))
            .WillOnce(Return("sha256string"));
    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    nlohmann::json response = uploadFolderAction.run(action.dump());

    EXPECT_EQ(response["result"], ResponseResult::TIMEOUT);
    EXPECT_EQ(response["httpStatus"], 26);
}