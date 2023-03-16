// Copyright 2023 Sophos Limited. All rights reserved.

#include "modules/ResponseActions/ResponseActionsImpl/DownloadFileAction.h"

#include "modules/Common/FileSystem/IFileTooLargeException.h"
#include "modules/Common/ZipUtilities/ZipUtils.h"

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

using namespace Common::HttpRequests;

class DownloadFileTests : public MemoryAppenderUsingTests
{
public:
    DownloadFileTests() : MemoryAppenderUsingTests("ResponseActionsImpl"){}

    void SetUp()
    {
        m_mockHttpRequester = std::make_shared<StrictMock<MockHTTPRequester>>();
        m_mockFileSystem = std::make_unique<StrictMock<MockFileSystem>>();
    }

    virtual void TearDown()
    {
        Tests::restoreFileSystem();
        Common::ZipUtilities::restoreZipUtils();
        Common::ApplicationConfiguration::restoreApplicationPathManager();
    }
    
    nlohmann::json getDownloadObject(bool decompress = false)
    {
        nlohmann::json action;
        action["url"] = "https://s3.com/download.zip";
        action["targetPath"] = m_destPath;
        action["sha256"] = "shastring";
        action["sizeBytes"] = 1024;
        action["decompress"] = decompress;
        action["password"] = "";
        action["expiration"] = 144444000000004;
        action["timeout"] = 10;
        return action;
    }

    std::shared_ptr<MockHTTPRequester> m_mockHttpRequester;
    std::unique_ptr<MockFileSystem> m_mockFileSystem;

    const Path m_testZipFile = "download.zip";
    const Path m_testExtractedFile = "download.txt";
    const Path m_destPath = "/path/to/download/to";
    const Path m_raTmpDir = "/opt/sophos-spl/plugins/responseactions/tmp";
    const Path m_raExtractTmpDir = "/opt/sophos-spl/plugins/responseactions/tmp/extract";
    
    void addResponseToMockRequester(long status, ResponseErrorCode errorCode)
    {
        Common::HttpRequests::Response httpresponse;
        httpresponse.status = status;
        httpresponse.errorCode = errorCode;
        EXPECT_CALL(*m_mockHttpRequester, get(_)).WillOnce(Return(httpresponse));
    }

    void addDiskSpaceExpectsToMockFileSystem(const std::uintmax_t& raAvailable = 1024 * 1024, const std::uintmax_t& tmpAvailable = 1024 * 1024)
    {
        std::filesystem::space_info raSpaceInfo;
        raSpaceInfo.available = raAvailable;

        std::filesystem::space_info tmpSpaceInfo;
        tmpSpaceInfo.available = tmpAvailable;

        EXPECT_CALL(*m_mockFileSystem, getDiskSpaceInfo(m_destPath)).WillOnce(Return(tmpSpaceInfo));
        EXPECT_CALL(*m_mockFileSystem, getDiskSpaceInfo(m_raTmpDir)).WillOnce(Return(raSpaceInfo));
    }
    
    void addExpectListFilesToMockFileSystem(const bool decompress = false)
    {
        std::vector<std::string> zippedList { m_raTmpDir + "/" + m_testZipFile };
        EXPECT_CALL(*m_mockFileSystem, listFiles(m_raTmpDir)).WillOnce(Return(zippedList));

        if (decompress)
        {
            std::vector<std::string> unzippedList { m_raExtractTmpDir + "/" + m_testExtractedFile };
            EXPECT_CALL(*m_mockFileSystem, listFiles(m_raExtractTmpDir)).WillOnce(Return(unzippedList));
        }
    }

    void setupMockZipUtils(const int& returnVal = 0, const bool& usesPassword = false)
    {
        auto mockZip = std::make_unique<NiceMock<MockZipUtils>>();

        if (!usesPassword)
        {
            EXPECT_CALL(*mockZip, unzip(_, _)).WillOnce(Return(returnVal));
        }
        else
        {
            EXPECT_CALL(*mockZip, unzip(_, _, _, _)).WillOnce(Return(returnVal));
        }
        Common::ZipUtilities::replaceZipUtils(std::move(mockZip));
    }
};


TEST_F(DownloadFileTests, SuccessfulDownload_Direct_NotDecompressed)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    addResponseToMockRequester(HTTP_STATUS_OK, ResponseErrorCode::OK);

    //MockFileSystem
    addDiskSpaceExpectsToMockFileSystem();
    addExpectListFilesToMockFileSystem();
    EXPECT_CALL(*m_mockFileSystem, isDirectory(m_destPath)).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, exists(m_destPath)).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, moveFile(m_raTmpDir + "/" + m_testZipFile, m_destPath)).Times(1);
    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, m_raTmpDir + "/" + m_testZipFile)).WillOnce(Return("shastring"));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester);

    nlohmann::json action = getDownloadObject();
    std::string response = downloadFileAction.run(action.dump());
    nlohmann::json responseJson = nlohmann::json::parse(response);

    EXPECT_EQ(responseJson["result"], 0);
    EXPECT_EQ(responseJson["httpStatus"], 200);
    EXPECT_FALSE(responseJson.contains("errorType"));
    EXPECT_FALSE(responseJson.contains("errorMessage"));

    EXPECT_TRUE(appenderContains("Downloading to /opt/sophos-spl/plugins/responseactions/tmp from url: https://s3.com/download.zip"));
    EXPECT_TRUE(appenderContains("Downloading directly"));
    EXPECT_TRUE(appenderContains("/path/to/download/to/download.zip downloaded successfully"));
}

TEST_F(DownloadFileTests, SuccessfulDownload_Direct_Decompressed)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    bool decompress = true;
    setupMockZipUtils();

    addResponseToMockRequester(HTTP_STATUS_OK, ResponseErrorCode::OK);

    //MockFileSystem
    addDiskSpaceExpectsToMockFileSystem();
    addExpectListFilesToMockFileSystem(decompress);
    EXPECT_CALL(*m_mockFileSystem, isDirectory(m_destPath)).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, exists(m_destPath)).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, moveFile(m_raExtractTmpDir + "/" + m_testExtractedFile, m_destPath)).Times(1);
    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, m_raTmpDir + "/" + m_testZipFile)).WillOnce(Return("shastring"));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester);

    nlohmann::json action = getDownloadObject(decompress);
    std::string response = downloadFileAction.run(action.dump());
    nlohmann::json responseJson = nlohmann::json::parse(response);

    EXPECT_EQ(responseJson["result"], 0);
    EXPECT_EQ(responseJson["httpStatus"], 200);
    EXPECT_FALSE(responseJson.contains("errorType"));
    EXPECT_FALSE(responseJson.contains("errorMessage"));

    EXPECT_TRUE(appenderContains("Downloading to /opt/sophos-spl/plugins/responseactions/tmp from url: https://s3.com/download.zip"));
    EXPECT_TRUE(appenderContains("Downloading directly"));
    EXPECT_TRUE(appenderContains("/path/to/download/to/download.txt downloaded successfully"));
}

TEST_F(DownloadFileTests, SuccessfulDownload_WithProxy_NotDecompressed)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    addResponseToMockRequester(HTTP_STATUS_OK, ResponseErrorCode::OK);

    std::string obfuscatedCreds = "CCD4E57ZjW+t5XPiMSJH1TurG3MfWCN3DpjJRINMwqNaWl+3zzlVIdyVmifCHUwcmaX6+YTSyyBM8SslIIGV5rUw";
    std::string content = R"({"proxy":"localhost","credentials":")" + obfuscatedCreds + R"("})";

    //MockFileSystem
    addDiskSpaceExpectsToMockFileSystem();
    addExpectListFilesToMockFileSystem();
    EXPECT_CALL(*m_mockFileSystem, isDirectory(m_destPath)).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, readFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(content));
    EXPECT_CALL(*m_mockFileSystem, exists(m_destPath)).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, moveFile(m_raTmpDir + "/" + m_testZipFile, m_destPath)).Times(1);
    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, m_raTmpDir + "/" + m_testZipFile)).WillOnce(Return("shastring"));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester);

    nlohmann::json action = getDownloadObject();
    std::string response = downloadFileAction.run(action.dump());
    nlohmann::json responseJson = nlohmann::json::parse(response);

    EXPECT_EQ(responseJson["result"], 0);
    EXPECT_EQ(responseJson["httpStatus"], 200);
    EXPECT_FALSE(responseJson.contains("errorType"));
    EXPECT_FALSE(responseJson.contains("errorMessage"));

    EXPECT_TRUE(appenderContains("Downloading to /opt/sophos-spl/plugins/responseactions/tmp from url: https://s3.com/download.zip"));
    EXPECT_TRUE(appenderContains("Downloading via proxy: localhost"));
    EXPECT_TRUE(appenderContains("/path/to/download/to/download.zip downloaded successfully"));
}

TEST_F(DownloadFileTests, SuccessfulDownload_WithProxy_Decompressed)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    std::string obfuscatedCreds = "CCD4E57ZjW+t5XPiMSJH1TurG3MfWCN3DpjJRINMwqNaWl+3zzlVIdyVmifCHUwcmaX6+YTSyyBM8SslIIGV5rUw";
    std::string content = R"({"proxy":"localhost","credentials":")" + obfuscatedCreds + R"("})";

    bool decompress = true;
    setupMockZipUtils();

    addResponseToMockRequester(HTTP_STATUS_OK, ResponseErrorCode::OK);

    //MockFileSystem
    addDiskSpaceExpectsToMockFileSystem();
    addExpectListFilesToMockFileSystem(decompress);
    EXPECT_CALL(*m_mockFileSystem, isDirectory(m_destPath)).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, readFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(content));
    EXPECT_CALL(*m_mockFileSystem, exists(m_destPath)).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, moveFile(m_raExtractTmpDir + "/" + m_testExtractedFile, m_destPath)).Times(1);
    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, m_raTmpDir + "/" + m_testZipFile)).WillOnce(Return("shastring"));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester);

    nlohmann::json action = getDownloadObject(decompress);
    std::string response = downloadFileAction.run(action.dump());
    nlohmann::json responseJson = nlohmann::json::parse(response);

    EXPECT_EQ(responseJson["result"], 0);
    EXPECT_EQ(responseJson["httpStatus"], 200);
    EXPECT_FALSE(responseJson.contains("errorType"));
    EXPECT_FALSE(responseJson.contains("errorMessage"));

    EXPECT_TRUE(appenderContains("Downloading to /opt/sophos-spl/plugins/responseactions/tmp from url: https://s3.com/download.zip"));
    EXPECT_TRUE(appenderContains("Downloading via proxy: localhost"));
    EXPECT_TRUE(appenderContains("/path/to/download/to/download.txt downloaded successfully"));
}

TEST_F(DownloadFileTests, ProxyFailureFallsbackDirect)
{
    auto httpRequester = std::make_shared<StrictMock<MockHTTPRequester>>();
    Common::HttpRequests::Response failhttpresponse;
    Common::HttpRequests::Response httpresponse;
    httpresponse.status = Common::HttpRequests::HTTP_STATUS_OK;
    httpresponse.errorCode = Common::HttpRequests::ResponseErrorCode::OK;
    failhttpresponse.status = 400;
    failhttpresponse.errorCode = Common::HttpRequests::ResponseErrorCode::OK;
    EXPECT_CALL(*httpRequester, get(_)).Times(2).WillOnce(Return(failhttpresponse)).WillOnce(Return(httpresponse));

    ResponseActionsImpl::DownloadFileAction downloadFileAction(httpRequester);
    nlohmann::json action = getDownloadObject();

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

    std::string response = downloadFileAction.run(action.dump());
    nlohmann::json responseJson = nlohmann::json::parse(response);
    EXPECT_EQ(responseJson["result"], 0);
    EXPECT_EQ(responseJson["httpStatus"], 200);
}

TEST_F(DownloadFileTests, SuccessCaseCompressionWithPassword)
{
    // TOUCHES FILE SYSTEM
    auto httpRequester = std::make_shared<StrictMock<MockHTTPRequester>>();
    Common::HttpRequests::Response httpresponse;
    httpresponse.status = Common::HttpRequests::HTTP_STATUS_OK;
    httpresponse.errorCode = Common::HttpRequests::ResponseErrorCode::OK;
    EXPECT_CALL(*httpRequester, get(_)).WillOnce(Return(httpresponse));

    Tests::TempDir tempDir;
    tempDir.makeDirs("test");
    tempDir.createFile("test/file.txt", "stuff");
    std::string filePath = tempDir.absPath("test/file.txt");
    std::string zipFile = tempDir.absPath("file.txt.zip");

    ResponseActionsImpl::DownloadFileAction downloadFileAction(httpRequester);
    nlohmann::json action = getDownloadObject();
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

    std::string response = downloadFileAction.run(action.dump());
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

TEST_F(DownloadFileTests, ZipFails)
{
    auto httpRequester = std::make_shared<StrictMock<MockHTTPRequester>>();
    Common::HttpRequests::Response httpresponse;
    httpresponse.status = Common::HttpRequests::HTTP_STATUS_OK;
    httpresponse.errorCode = Common::HttpRequests::ResponseErrorCode::OK;

    ResponseActionsImpl::DownloadFileAction downloadFileAction(httpRequester);
    nlohmann::json action = getDownloadObject();
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

    std::string response = downloadFileAction.run(action.dump());
    nlohmann::json responseJson = nlohmann::json::parse(response);
    EXPECT_EQ(responseJson["result"], 3);
    EXPECT_EQ(responseJson["errorMessage"], "Error zipping path");
}

TEST_F(DownloadFileTests, ZipFailsDueToLargeFile)
{
    auto httpRequester = std::make_shared<StrictMock<MockHTTPRequester>>();
    Common::HttpRequests::Response httpresponse;
    httpresponse.status = Common::HttpRequests::HTTP_STATUS_OK;
    httpresponse.errorCode = Common::HttpRequests::ResponseErrorCode::OK;

    ResponseActionsImpl::DownloadFileAction downloadFileAction(httpRequester);
    nlohmann::json action = getDownloadObject();
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

    std::string response = downloadFileAction.run(action.dump());
    nlohmann::json responseJson = nlohmann::json::parse(response);
    EXPECT_EQ(responseJson["result"], 3);
    EXPECT_EQ(responseJson["errorMessage"], "Error zipping path");
}

TEST_F(DownloadFileTests, cannotParseActions)
{
    auto httpRequester = std::make_shared<StrictMock<MockHTTPRequester>>();
    ResponseActionsImpl::DownloadFileAction downloadFileAction(httpRequester);

    std::string response = downloadFileAction.run("");
    nlohmann::json responseJson = nlohmann::json::parse(response);
    EXPECT_EQ(responseJson["result"], 1);
    EXPECT_EQ(responseJson["errorMessage"], "Error parsing command from Central");
}

TEST_F(DownloadFileTests, actionExipired)
{
    auto httpRequester = std::make_shared<StrictMock<MockHTTPRequester>>();
    ResponseActionsImpl::DownloadFileAction downloadFileAction(httpRequester);
    nlohmann::json action = getDownloadObject();
    action["expiration"] = 0;
    std::string response = downloadFileAction.run(action.dump());
    nlohmann::json responseJson = nlohmann::json::parse(response);
    EXPECT_EQ(responseJson["result"],4);
    EXPECT_EQ(responseJson["errorMessage"],"Action has expired");
}

TEST_F(DownloadFileTests, FileDoesNotExist)
{
    auto httpRequester = std::make_shared<StrictMock<MockHTTPRequester>>();
    ResponseActionsImpl::DownloadFileAction downloadFileAction(httpRequester);
    nlohmann::json action = getDownloadObject();

    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>{ mockFileSystem });
    EXPECT_CALL(*mockFileSystem, isFile("path")).WillOnce(Return(false));

    std::string response = downloadFileAction.run(action.dump());
    nlohmann::json responseJson = nlohmann::json::parse(response);
    EXPECT_EQ(responseJson["result"],1);
    EXPECT_EQ(responseJson["errorType"],"invalid_path");
    EXPECT_EQ(responseJson["errorMessage"],"path is not a file");
}

TEST_F(DownloadFileTests, FileOverSizeLimit)
{
    auto httpRequester = std::make_shared<StrictMock<MockHTTPRequester>>();
    ResponseActionsImpl::DownloadFileAction downloadFileAction(httpRequester);
    nlohmann::json action = getDownloadObject();

    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>{ mockFileSystem });
    EXPECT_CALL(*mockFileSystem, isFile("path")).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, fileSize("path")).WillOnce(Return(10000));

    std::string response = downloadFileAction.run(action.dump());
    nlohmann::json responseJson = nlohmann::json::parse(response);
    EXPECT_EQ(responseJson["result"],1);
    EXPECT_EQ(responseJson["errorType"],"exceed_size_limit");
    EXPECT_EQ(responseJson["errorMessage"],"File at path path is size 10000 bytes which is above the size limit 1000 bytes");
}

TEST_F(DownloadFileTests, FileBeingWrittenToAndOverSizeLimit)
{
    auto httpRequester = std::make_shared<StrictMock<MockHTTPRequester>>();
    ResponseActionsImpl::DownloadFileAction downloadFileAction(httpRequester);
    nlohmann::json action = getDownloadObject();

    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>{ mockFileSystem });
    EXPECT_CALL(*mockFileSystem, isFile("path")).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, fileSize("path")).WillOnce(Return(100));
    EXPECT_CALL(*mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256,"path")).WillOnce(Throw(Common::FileSystem::IFileSystemException("")));

    std::string response = downloadFileAction.run(action.dump());
    nlohmann::json responseJson = nlohmann::json::parse(response);
    EXPECT_EQ(responseJson["result"],1);
    EXPECT_EQ(responseJson["errorType"],"access_denied");
    EXPECT_EQ(responseJson["errorMessage"],"File to be uploaded cannot be accessed");
}

TEST_F(DownloadFileTests, FailureDueToTimeout)
{
    auto httpRequester = std::make_shared<StrictMock<MockHTTPRequester>>();
    Common::HttpRequests::Response httpresponse;
    httpresponse.status = 500;
    httpresponse.errorCode = Common::HttpRequests::ResponseErrorCode::TIMEOUT;
    EXPECT_CALL(*httpRequester, get(_)).WillOnce(Return(httpresponse));
    ResponseActionsImpl::DownloadFileAction downloadFileAction(httpRequester);
    nlohmann::json action = getDownloadObject();

    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>{ mockFileSystem });
    EXPECT_CALL(*mockFileSystem, isFile("path")).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, fileSize("path")).WillOnce(Return(100));
    EXPECT_CALL(*mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, "path"))
        .WillOnce(Return("sha256string"));
    EXPECT_CALL(*mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    std::string response = downloadFileAction.run(action.dump());
    nlohmann::json responseJson = nlohmann::json::parse(response);
    EXPECT_EQ(responseJson["result"], 2);
    EXPECT_EQ(responseJson["httpStatus"], 500);
    EXPECT_EQ(responseJson["errorMessage"], "Timeout Uploading file: path");
}

TEST_F(DownloadFileTests, FailureDueToNetworkError)
{
    auto httpRequester = std::make_shared<StrictMock<MockHTTPRequester>>();
    Common::HttpRequests::Response httpresponse;
    httpresponse.status = 500;
    httpresponse.errorCode = Common::HttpRequests::ResponseErrorCode::FAILED;
    httpresponse.error = "failure";
    EXPECT_CALL(*httpRequester, get(_)).WillOnce(Return(httpresponse));
    ResponseActionsImpl::DownloadFileAction downloadFileAction(httpRequester);
    nlohmann::json action = getDownloadObject();

    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>{ mockFileSystem });
    EXPECT_CALL(*mockFileSystem, isFile("path")).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, fileSize("path")).WillOnce(Return(100));
    EXPECT_CALL(*mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, "path"))
        .WillOnce(Return("sha256string"));
    EXPECT_CALL(*mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));

    std::string response = downloadFileAction.run(action.dump());
    nlohmann::json responseJson = nlohmann::json::parse(response);
    EXPECT_EQ(responseJson["result"], 1);
    EXPECT_EQ(responseJson["httpStatus"], 500);
    EXPECT_EQ(responseJson["errorMessage"], "Failed to upload file: path with error: failure");
    EXPECT_EQ(responseJson["errorType"], "network_error");
}

TEST_F(DownloadFileTests, FailureDueToServerError)
{
    auto httpRequester = std::make_shared<StrictMock<MockHTTPRequester>>();
    Common::HttpRequests::Response httpresponse;
    httpresponse.status = 500;
    httpresponse.errorCode = Common::HttpRequests::ResponseErrorCode::OK;
    EXPECT_CALL(*httpRequester, get(_)).WillOnce(Return(httpresponse));
    ResponseActionsImpl::DownloadFileAction downloadFileAction(httpRequester);
    nlohmann::json action = getDownloadObject();

    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>{ mockFileSystem });
    EXPECT_CALL(*mockFileSystem, isFile("path")).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, fileSize("path")).WillOnce(Return(100));
    EXPECT_CALL(*mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, "path"))
        .WillOnce(Return("sha256string"));
    EXPECT_CALL(*mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));

    std::string response = downloadFileAction.run(action.dump());
    nlohmann::json responseJson = nlohmann::json::parse(response);
    EXPECT_EQ(responseJson["result"], 1);
    EXPECT_EQ(responseJson["httpStatus"], 500);
    EXPECT_EQ(responseJson["errorMessage"], "Failed to upload file: path with http error code 500");
    EXPECT_EQ(responseJson["errorType"], "network_error");
}
