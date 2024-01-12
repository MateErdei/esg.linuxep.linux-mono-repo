// Copyright 2023-2024 Sophos Limited. All rights reserved.

#define TEST_PUBLIC public

#include "ResponseActions/RACommon/ResponseActionsCommon.h"
#include "ResponseActions/ResponseActionsImpl/DownloadFileAction.h"

#include "Common/FileSystem/IFileTooLargeException.h"
#include "Common/ZipUtilities/ZipUtils.h"

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

using namespace Common::HttpRequests;
using namespace ResponseActions::RACommon;

class DownloadFileTests : public MemoryAppenderUsingTests
{
public:
    DownloadFileTests() : MemoryAppenderUsingTests("ResponseActionsImpl"){}

    void SetUp()
    {
        m_mockHttpRequester = std::make_shared<StrictMock<MockHTTPRequester>>();
        m_mockFileSystem = std::make_unique<StrictMock<MockFileSystem>>();
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

    std::shared_ptr<MockHTTPRequester> m_mockHttpRequester;
    std::unique_ptr<MockFileSystem> m_mockFileSystem;

    const Path m_testZipFile = "testdownload.zip";
    const Path m_testExtractedFile = "download.txt";
    const Path m_defaultZipName = "download.zip";
    const Path m_destPath = "/path/to/download/to/";
    const Path m_raTmpDir = "/opt/sophos-spl/plugins/responseactions/tmp";
    const Path m_raTmpFile = "/opt/sophos-spl/plugins/responseactions/tmp/tmp_download.zip";
    const Path m_raExtractTmpDir = "/opt/sophos-spl/plugins/responseactions/tmp/extract";
    const std::string m_password = "password";

    nlohmann::json getDownloadObject(
        const bool decompress = false,
        const std::string& password = "",
        const unsigned long& bytes = 1024,
        bool includeFileName = true)
    {
        Path file = "";
        if (includeFileName)
        {
            file = decompress ? m_testExtractedFile : m_testZipFile;
        }

        nlohmann::json action;
        action["url"] = "https://s3.com/download.zip";
        action["targetPath"] = m_destPath + file;
        action["sha256"] = "shastring";
        action["sizeBytes"] = bytes;
        action["decompress"] = decompress;
        action["password"] = password;
        action["expiration"] = 144444000000004;
        action["timeout"] = 10;
        return action;
    }

    void addResponseToMockRequester(const long& status, const ResponseErrorCode& errorCode, const std::string& errormsg = "")
    {
        Common::HttpRequests::Response httpresponse;
        httpresponse.status = status;
        httpresponse.errorCode = errorCode;
        httpresponse.error = errormsg;
        EXPECT_CALL(*m_mockHttpRequester, get(_)).WillOnce(Return(httpresponse));
    }

    void addDiskSpaceExpectsToMockFileSystem(const std::uintmax_t& raAvailable = 1024 * 1024, const std::uintmax_t& tmpAvailable = 1024 * 1024)
    {
        std::filesystem::space_info raSpaceInfo;
        raSpaceInfo.available = raAvailable;

        std::filesystem::space_info tmpSpaceInfo;
        tmpSpaceInfo.available = tmpAvailable;

        EXPECT_CALL(*m_mockFileSystem, getDiskSpaceInfo("/path", _)).WillOnce(Return(tmpSpaceInfo));
        EXPECT_CALL(*m_mockFileSystem, getDiskSpaceInfo("/opt", _)).WillOnce(Return(raSpaceInfo));
    }

    void setupMockZipUtils(const int& returnVal = 0, const std::string& password = "")
    {
        auto mockZip = std::make_unique<NiceMock<MockZipUtils>>();

        if (password.empty())
        {
            EXPECT_CALL(*mockZip, unzip(_, _)).WillOnce(Return(returnVal));
        }
        else
        {
            EXPECT_CALL(*mockZip, unzip(_, _, true, password)).WillOnce(Return(returnVal));
        }
        Common::ZipUtilities::replaceZipUtils(std::move(mockZip));
    }

    void addListFilesExpectsToMockFileSystem(const bool decompress = false, const bool multipleFiles = false, const Path subDirs = "")
    {
        std::vector<std::string> zippedList { m_raTmpFile };
        EXPECT_CALL(*m_mockFileSystem, listFiles(m_raTmpDir)).WillOnce(Return(zippedList));

        if (decompress)
        {
            std::vector<std::string> unzippedList { m_raExtractTmpDir + "/" + subDirs + m_testExtractedFile };
            if (multipleFiles)
            {
                for (int i = 1; i != 4; i++)
                {
                    auto tmpString = m_raExtractTmpDir + "/" + subDirs + m_testExtractedFile + (std::to_string(i));
                    unzippedList.push_back(tmpString);
                }
            }

            EXPECT_CALL(*m_mockFileSystem, listAllFilesInDirectoryTree(m_raExtractTmpDir)).WillOnce(Return(unzippedList));
        }
    }

    void addDownloadAndExtractExpectsToMockFileSystem(const std::string& targetPath, const bool decompress = false, const Path subDirs = "")
    {
        std::string filename = m_testZipFile;

        if (decompress)
        {
            EXPECT_CALL(*m_mockFileSystem, exists(m_raExtractTmpDir))
                .WillOnce(Return(false))
                .RetiresOnSaturation();
            EXPECT_CALL(*m_mockFileSystem, makedirs(m_raExtractTmpDir))
                .Times(1)
                .RetiresOnSaturation();
        }

        EXPECT_CALL(*m_mockFileSystem, exists(m_destPath)).WillOnce(Return(false)).RetiresOnSaturation();
        EXPECT_CALL(*m_mockFileSystem, exists(targetPath)).WillOnce(Return(false)).RetiresOnSaturation();
        EXPECT_CALL(*m_mockFileSystem, makedirs(m_destPath)).Times(1);
        if (!subDirs.empty())
        {
            Path subDirNoSlash = subDirs;
            subDirNoSlash.pop_back();
            EXPECT_CALL(*m_mockFileSystem, makedirs(m_destPath + subDirNoSlash)).Times(AnyNumber());
        }
    }

    void addSingleCleanupChecksToMockFileSystem(const bool downloadExists = false, const bool extractExists = false)
    {
        EXPECT_CALL(*m_mockFileSystem, exists(m_raTmpFile))
            .WillOnce(Return(downloadExists));
        EXPECT_CALL(*m_mockFileSystem, exists(m_raExtractTmpDir))
            .WillOnce(Return(extractExists))
            .RetiresOnSaturation();

        if (downloadExists)
        {
            EXPECT_CALL(*m_mockFileSystem, removeFile(m_raTmpFile)).Times(1);
        }
        if (extractExists)
        {
            EXPECT_CALL(*m_mockFileSystem, removeFileOrDirectory(m_raExtractTmpDir)).Times(1);
        }

    }

    void addCleanupChecksToMockFileSystem(const bool downloadExists = false, const bool extractExists = false)
    {
        EXPECT_CALL(*m_mockFileSystem, exists(m_raTmpFile))
            .Times(2).WillRepeatedly(Return(downloadExists));
        EXPECT_CALL(*m_mockFileSystem, exists(m_raExtractTmpDir))
            .Times(2).WillRepeatedly(Return(extractExists))
            .RetiresOnSaturation();

        if (downloadExists)
        {
            EXPECT_CALL(*m_mockFileSystem, removeFile(m_raTmpFile)).Times(2);
        }
        if (extractExists)
        {
            EXPECT_CALL(*m_mockFileSystem, removeFileOrDirectory(m_raExtractTmpDir)).Times(2);
        }

    }
};


//Not Decompressed End to End

TEST_F(DownloadFileTests, SuccessfulDownload_Direct_NotDecompressed)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    addResponseToMockRequester(HTTP_STATUS_OK, ResponseErrorCode::OK);

    addDiskSpaceExpectsToMockFileSystem();
    addListFilesExpectsToMockFileSystem();
    addDownloadAndExtractExpectsToMockFileSystem(m_destPath + m_testZipFile);
    addCleanupChecksToMockFileSystem();

    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, moveFileTryCopy(m_raTmpFile, m_destPath + m_testZipFile)).Times(1);
    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, m_raTmpFile))
        .WillOnce(Return("shastring"));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);

    nlohmann::json action = getDownloadObject();
    nlohmann::json response = downloadFileAction.run(action.dump());

    EXPECT_EQ(response["type"], DOWNLOAD_FILE_RESPONSE_TYPE);
    EXPECT_EQ(response["result"], ResponseResult::SUCCESS);
    EXPECT_EQ(response["httpStatus"], HTTP_STATUS_OK);
    EXPECT_FALSE(response.contains("errorType"));
    EXPECT_FALSE(response.contains("errorMessage"));
    EXPECT_TRUE(response.contains("duration"));

    EXPECT_TRUE(appenderContains("Beginning download to /path/to/download/to/testdownload.zip"));
    EXPECT_TRUE(appenderContains("Downloading directly"));
    EXPECT_TRUE(appenderContains("/path/to/download/to/testdownload.zip downloaded successfully"));
}

TEST_F(DownloadFileTests, SuccessfulDownload_Direct_HugeURL)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    const std::string largeStr(30000, 'a');
    const std::string largeURL("https://s3.com/download" + largeStr + ".zip");
    const std::string expectedMsg("Download URL is " + largeURL);

    addResponseToMockRequester(HTTP_STATUS_OK, ResponseErrorCode::OK);

    addDiskSpaceExpectsToMockFileSystem();
    addListFilesExpectsToMockFileSystem();
    addDownloadAndExtractExpectsToMockFileSystem(m_destPath + m_testZipFile);
    addCleanupChecksToMockFileSystem();

    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, moveFileTryCopy(m_raTmpFile, m_destPath + m_testZipFile)).Times(1);
    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, m_raTmpFile))
        .WillOnce(Return("shastring"));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);

    nlohmann::json action = getDownloadObject();
    action["url"] = largeURL;
    nlohmann::json response = downloadFileAction.run(action.dump());

    EXPECT_EQ(response["type"], DOWNLOAD_FILE_RESPONSE_TYPE);
    EXPECT_EQ(response["result"], ResponseResult::SUCCESS);
    EXPECT_EQ(response["httpStatus"], HTTP_STATUS_OK);
    EXPECT_FALSE(response.contains("errorType"));
    EXPECT_FALSE(response.contains("errorMessage"));
    EXPECT_TRUE(response.contains("duration"));

    EXPECT_TRUE(appenderContains(expectedMsg));
}

TEST_F(DownloadFileTests, SuccessfulDownload_Direct_HugePath)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    const std::string largeStr(30000, 'a');
    const std::string largeTargetPath(m_destPath + largeStr);
    const std::string expectedMsg(largeTargetPath + " downloaded successfully");

    addResponseToMockRequester(HTTP_STATUS_OK, ResponseErrorCode::OK);

    addDiskSpaceExpectsToMockFileSystem();
    addListFilesExpectsToMockFileSystem();
    addDownloadAndExtractExpectsToMockFileSystem(largeTargetPath);
    addCleanupChecksToMockFileSystem();

    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, moveFileTryCopy(m_raTmpFile, largeTargetPath)).Times(1);
    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, m_raTmpFile))
        .WillOnce(Return("shastring"));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);

    nlohmann::json action = getDownloadObject();
    action["targetPath"] = largeTargetPath;
    nlohmann::json response = downloadFileAction.run(action.dump());

    EXPECT_EQ(response["type"], DOWNLOAD_FILE_RESPONSE_TYPE);
    EXPECT_EQ(response["result"], ResponseResult::SUCCESS);
    EXPECT_EQ(response["httpStatus"], HTTP_STATUS_OK);
    EXPECT_FALSE(response.contains("errorType"));
    EXPECT_FALSE(response.contains("errorMessage"));
    EXPECT_TRUE(response.contains("duration"));

    EXPECT_TRUE(appenderContains(expectedMsg));
}

TEST_F(DownloadFileTests, SuccessfulDownload_Direct_NotDecompressed_NoFileNameInTargetPath)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    addResponseToMockRequester(HTTP_STATUS_OK, ResponseErrorCode::OK);

    addDiskSpaceExpectsToMockFileSystem();
    addListFilesExpectsToMockFileSystem();
    addDownloadAndExtractExpectsToMockFileSystem(m_destPath + m_defaultZipName);
    addCleanupChecksToMockFileSystem();

    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, moveFileTryCopy(m_raTmpFile, m_destPath + m_defaultZipName)).Times(1);
    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, m_raTmpFile))
        .WillOnce(Return("shastring"));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);

    nlohmann::json action = getDownloadObject(false, "", 1024, false);
    nlohmann::json response = downloadFileAction.run(action.dump());

    EXPECT_EQ(response["type"], DOWNLOAD_FILE_RESPONSE_TYPE);
    EXPECT_EQ(response["result"], ResponseResult::SUCCESS);
    EXPECT_EQ(response["httpStatus"], HTTP_STATUS_OK);
    EXPECT_FALSE(response.contains("errorType"));
    EXPECT_FALSE(response.contains("errorMessage"));
    EXPECT_TRUE(response.contains("duration"));

    EXPECT_TRUE(appenderContains("Beginning download to /path/to/download/to/"));
    EXPECT_TRUE(appenderContains("Downloading directly"));
    EXPECT_TRUE(appenderContains("/path/to/download/to/download.zip downloaded successfully"));
}

TEST_F(DownloadFileTests, SuccessfulDownload_Direct_NotDecompressed_RATmpAndExtractFileExistsAtCleanup)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    addResponseToMockRequester(HTTP_STATUS_OK, ResponseErrorCode::OK);

    addDiskSpaceExpectsToMockFileSystem();
    addListFilesExpectsToMockFileSystem();
    addDownloadAndExtractExpectsToMockFileSystem(m_destPath + m_testZipFile);

    //Test makes sure that remove expects set in below function are called
    addCleanupChecksToMockFileSystem(true, true);

    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, moveFileTryCopy(m_raTmpFile, m_destPath + m_testZipFile)).Times(1);
    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, m_raTmpFile))
        .WillOnce(Return("shastring"));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);

    nlohmann::json action = getDownloadObject();
    nlohmann::json response = downloadFileAction.run(action.dump());
}

//Decompressed End to End Single File No Password

TEST_F(DownloadFileTests, SuccessfulDownload_Direct_Decompressed_WithFileNameInTargetPath)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    bool decompress = true;
    setupMockZipUtils();

    addResponseToMockRequester(HTTP_STATUS_OK, ResponseErrorCode::OK);

    addDiskSpaceExpectsToMockFileSystem();
    addListFilesExpectsToMockFileSystem(decompress);
    addDownloadAndExtractExpectsToMockFileSystem(m_destPath + m_testExtractedFile, decompress);
    addCleanupChecksToMockFileSystem();

    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, moveFileTryCopy(m_raExtractTmpDir + "/" + m_testExtractedFile, m_destPath + m_testExtractedFile)).Times(1);
    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, m_raTmpFile))
        .WillOnce(Return("shastring"));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);

    nlohmann::json action = getDownloadObject(decompress);
    nlohmann::json response = downloadFileAction.run(action.dump());

    EXPECT_EQ(response["result"], ResponseResult::SUCCESS);
    EXPECT_EQ(response["httpStatus"], HTTP_STATUS_OK);
    EXPECT_FALSE(response.contains("errorType"));
    EXPECT_FALSE(response.contains("errorMessage"));

    EXPECT_TRUE(appenderContains("Extracted 1 file from archive"));
    EXPECT_TRUE(appenderContains("Beginning download to /path/to/download/to/download.txt"));
    EXPECT_TRUE(appenderContains("Downloading directly"));
    EXPECT_TRUE(appenderContains("/path/to/download/to/download.txt downloaded successfully"));
}

TEST_F(DownloadFileTests, SuccessfulDownload_Direct_Decompressed_WithFileNameInTargetPath_WithSubDirectories)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    bool decompress = true;
    Path subDirs = "subDir1/subDir2/";
    setupMockZipUtils();

    addResponseToMockRequester(HTTP_STATUS_OK, ResponseErrorCode::OK);

    addDiskSpaceExpectsToMockFileSystem();
    addListFilesExpectsToMockFileSystem(decompress, false, subDirs);
    addDownloadAndExtractExpectsToMockFileSystem(m_destPath + subDirs + m_testExtractedFile, decompress, subDirs);
    addCleanupChecksToMockFileSystem();

    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, moveFileTryCopy(m_raExtractTmpDir + "/" + subDirs + m_testExtractedFile, m_destPath + subDirs + m_testExtractedFile)).Times(1);
    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, m_raTmpFile))
        .WillOnce(Return("shastring"));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);


    nlohmann::json action = getDownloadObject(decompress, "", 1024, false);
    nlohmann::json response = downloadFileAction.run(action.dump());

    EXPECT_EQ(response["result"], ResponseResult::SUCCESS);
    EXPECT_EQ(response["httpStatus"], HTTP_STATUS_OK);
    EXPECT_FALSE(response.contains("errorType"));
    EXPECT_FALSE(response.contains("errorMessage"));

    EXPECT_TRUE(appenderContains("Extracted 1 file from archive"));
    EXPECT_TRUE(appenderContains("Beginning download to /path/to/download/to/"));
    EXPECT_TRUE(appenderContains("Downloading directly"));
    EXPECT_TRUE(appenderContains("/path/to/download/to/subDir1/subDir2/download.txt downloaded successfully"));
}

TEST_F(DownloadFileTests, SuccessfulDownload_Direct_Decompressed_NoFileNameInTargetPath)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    bool decompress = true;
    setupMockZipUtils();
    const std::string fileName = "anotherdownload.txt";

    addResponseToMockRequester(HTTP_STATUS_OK, ResponseErrorCode::OK);

    addCleanupChecksToMockFileSystem();
    addDiskSpaceExpectsToMockFileSystem();
    addListFilesExpectsToMockFileSystem(); //We want to specify the name of the unzipped file
    addDownloadAndExtractExpectsToMockFileSystem(m_destPath + fileName, decompress);

    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, moveFileTryCopy(m_raExtractTmpDir + "/" + fileName, m_destPath + fileName)).Times(1);
    EXPECT_CALL(*m_mockFileSystem, listAllFilesInDirectoryTree(m_raExtractTmpDir))
        .WillOnce(Return(std::vector<std::string> {m_raExtractTmpDir + "/" + fileName}));

    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, m_raTmpFile))
        .WillOnce(Return("shastring"));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);

    nlohmann::json action = getDownloadObject(decompress, "", 1024, false);
    nlohmann::json response = downloadFileAction.run(action.dump());

    EXPECT_EQ(response["result"], ResponseResult::SUCCESS);
    EXPECT_EQ(response["httpStatus"], HTTP_STATUS_OK);
    EXPECT_FALSE(response.contains("errorType"));
    EXPECT_FALSE(response.contains("errorMessage"));

    EXPECT_TRUE(appenderContains("Beginning download to /path/to/download/to/"));
    EXPECT_TRUE(appenderContains("Downloading directly"));
    EXPECT_TRUE(appenderContains("/path/to/download/to/anotherdownload.txt downloaded successfully"));
}

TEST_F(DownloadFileTests, SuccessfulDownload_Direct_Decompressed_RATmpAndExtractFileExistsAtCleanup)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    setupMockZipUtils();

    bool decompress = true;
    addResponseToMockRequester(HTTP_STATUS_OK, ResponseErrorCode::OK);

    EXPECT_CALL(*m_mockFileSystem, exists(m_raTmpFile)).Times(2).WillRepeatedly(Return(true));
    EXPECT_CALL(*m_mockFileSystem, exists(m_raExtractTmpDir))
        .WillOnce(Return(true))
        .WillOnce(Return(false))
        .WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, makedirs(m_raExtractTmpDir)).Times(1);
    EXPECT_CALL(*m_mockFileSystem, removeFile(m_raTmpFile)).Times(2);
    EXPECT_CALL(*m_mockFileSystem, removeFileOrDirectory(m_raExtractTmpDir)).Times(2);
    EXPECT_CALL(*m_mockFileSystem, exists(m_destPath)).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, exists(m_destPath + m_testExtractedFile)).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, makedirs(m_destPath)).Times(1);

    addDiskSpaceExpectsToMockFileSystem();
    addListFilesExpectsToMockFileSystem(decompress);
    //Test makes sure that remove expects set in below function are called

    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, moveFileTryCopy(m_raExtractTmpDir + "/" + m_testExtractedFile, m_destPath + m_testExtractedFile)).Times(1);
    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, m_raTmpFile))
        .WillOnce(Return("shastring"));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);

    nlohmann::json action = getDownloadObject(decompress);
    nlohmann::json response = downloadFileAction.run(action.dump());
}

TEST_F(DownloadFileTests, SuccessfulDownload_Direct_Decompressed_DownloadToRoot)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    const Path destPath = "/download.txt";
    bool decompress = true;

    std::filesystem::space_info tmpSpaceInfo;
    tmpSpaceInfo.available = 1024 * 1024;

    setupMockZipUtils();
    addResponseToMockRequester(HTTP_STATUS_OK, ResponseErrorCode::OK);

    addListFilesExpectsToMockFileSystem(decompress);

    //Download and extraction calls
    EXPECT_CALL(*m_mockFileSystem, exists(m_raExtractTmpDir)).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, makedirs(m_raExtractTmpDir)).Times(1);
    EXPECT_CALL(*m_mockFileSystem, exists("/download.txt")).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, exists("/")).WillOnce(Return(true));
    addCleanupChecksToMockFileSystem();

    EXPECT_CALL(*m_mockFileSystem, getDiskSpaceInfo(destPath, _)).WillOnce(Return(tmpSpaceInfo));
    EXPECT_CALL(*m_mockFileSystem, getDiskSpaceInfo("/opt", _)).WillOnce(Return(tmpSpaceInfo));
    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, moveFileTryCopy(m_raExtractTmpDir + "/" + m_testExtractedFile, destPath)).Times(1);
    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, m_raTmpFile))
        .WillOnce(Return("shastring"));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);

    nlohmann::json action = getDownloadObject(decompress);
    action["targetPath"] = destPath;
    nlohmann::json response = downloadFileAction.run(action.dump());

    EXPECT_EQ(response["result"], ResponseResult::SUCCESS);
    EXPECT_EQ(response["httpStatus"], HTTP_STATUS_OK);
    EXPECT_FALSE(response.contains("errorType"));
    EXPECT_FALSE(response.contains("errorMessage"));

    EXPECT_TRUE(appenderContains("Beginning download to /download.txt"));
    EXPECT_TRUE(appenderContains("Downloading directly"));
    EXPECT_TRUE(appenderContains("/download.txt downloaded successfully"));
}

//Decompressed End to End Single File Unzip Issues

TEST_F(DownloadFileTests, SuccessfulDownload_Direct_Decompress_BadArchive)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    const std::string expectedErrMsg = "Error unzipping /opt/sophos-spl/plugins/responseactions/tmp/tmp_download.zip due to invalid zip file";

    bool decompress = true;
    setupMockZipUtils(UNZ_BADZIPFILE);

    addResponseToMockRequester(HTTP_STATUS_OK, ResponseErrorCode::OK);

    addDiskSpaceExpectsToMockFileSystem();
    addListFilesExpectsToMockFileSystem();

    EXPECT_CALL(*m_mockFileSystem, exists(m_raExtractTmpDir)).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, makedirs(m_raExtractTmpDir)).Times(1);
    addCleanupChecksToMockFileSystem();

    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, m_raTmpFile))
        .WillOnce(Return("shastring"));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);

    nlohmann::json action = getDownloadObject(decompress);
    nlohmann::json response = downloadFileAction.run(action.dump());

    EXPECT_EQ(response["result"], ResponseResult::ERROR);
    EXPECT_EQ(response["httpStatus"], HTTP_STATUS_OK);
    EXPECT_EQ(response["errorType"], "invalid_archive");
    EXPECT_EQ(response["errorMessage"], expectedErrMsg);

    EXPECT_TRUE(appenderContains("Beginning download to /path/to/download/to/download.txt"));
    EXPECT_TRUE(appenderContains("Downloaded file: /opt/sophos-spl/plugins/responseactions/tmp/tmp_download.zip"));
    EXPECT_TRUE(appenderContains(expectedErrMsg));
}

TEST_F(DownloadFileTests, SuccessfulDownload_Direct_Decompress_ZipUtilsThrows)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    //more helpful logging is done in ziputil
    const std::string expectedErrMsg = "Error unzipping /opt/sophos-spl/plugins/responseactions/tmp/tmp_download.zip due to error no 1";

    bool decompress = true;

    auto mockZip = std::make_unique<NiceMock<MockZipUtils>>();
    EXPECT_CALL(*mockZip, unzip(_, _)).WillOnce(Throw(std::runtime_error("error")));
    Common::ZipUtilities::replaceZipUtils(std::move(mockZip));

    addResponseToMockRequester(HTTP_STATUS_OK, ResponseErrorCode::OK);

    addDiskSpaceExpectsToMockFileSystem();
    addListFilesExpectsToMockFileSystem();

    EXPECT_CALL(*m_mockFileSystem, exists(m_raExtractTmpDir)).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, makedirs(m_raExtractTmpDir)).Times(1);
    addCleanupChecksToMockFileSystem();

    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, m_raTmpFile))
        .WillOnce(Return("shastring"));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);

    nlohmann::json action = getDownloadObject(decompress);
    nlohmann::json response = downloadFileAction.run(action.dump());

    EXPECT_EQ(response["result"], ResponseResult::INTERNAL_ERROR);
    EXPECT_EQ(response["httpStatus"], HTTP_STATUS_OK);
    EXPECT_FALSE(response.contains("errorType"));
    EXPECT_EQ(response["errorMessage"], expectedErrMsg);

    EXPECT_TRUE(appenderContains("Beginning download to /path/to/download/to/download.txt"));
    EXPECT_TRUE(appenderContains("Downloaded file: /opt/sophos-spl/plugins/responseactions/tmp/tmp_download.zip"));
    EXPECT_TRUE(appenderContains(expectedErrMsg));
}

TEST_F(DownloadFileTests, SuccessfulDownload_Direct_Decompress_OtherError)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    const std::string expectedErrMsg = "Error unzipping /opt/sophos-spl/plugins/responseactions/tmp/tmp_download.zip due to error no -104";

    bool decompress = true;
    setupMockZipUtils(UNZ_INTERNALERROR);

    addResponseToMockRequester(HTTP_STATUS_OK, ResponseErrorCode::OK);

    addDiskSpaceExpectsToMockFileSystem();
    addListFilesExpectsToMockFileSystem();

    EXPECT_CALL(*m_mockFileSystem, exists(m_raExtractTmpDir)).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, makedirs(m_raExtractTmpDir)).Times(1);
    addCleanupChecksToMockFileSystem();

    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, m_raTmpFile))
        .WillOnce(Return("shastring"));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);

    nlohmann::json action = getDownloadObject(decompress);
    nlohmann::json response = downloadFileAction.run(action.dump());


    EXPECT_EQ(response["result"], ResponseResult::INTERNAL_ERROR);
    EXPECT_EQ(response["httpStatus"], HTTP_STATUS_OK);
    EXPECT_FALSE(response.contains("errorType"));
    EXPECT_EQ(response["errorMessage"], expectedErrMsg);

    EXPECT_TRUE(appenderContains("Beginning download to /path/to/download/to/download.txt"));
    EXPECT_TRUE(appenderContains("Downloaded file: /opt/sophos-spl/plugins/responseactions/tmp/tmp_download.zip"));
    EXPECT_TRUE(appenderContains(expectedErrMsg));
}

//Decompressed End to End Multiple Files

TEST_F(DownloadFileTests, SuccessfulDownload_Direct_Decompress_ManyFilesTargetPathIsDirectory)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    bool decompress = true;

    setupMockZipUtils();
    addResponseToMockRequester(HTTP_STATUS_OK, ResponseErrorCode::OK);

    addDiskSpaceExpectsToMockFileSystem();
    addListFilesExpectsToMockFileSystem(decompress, true);
    EXPECT_CALL(*m_mockFileSystem, exists(m_raExtractTmpDir)).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, makedirs(m_raExtractTmpDir)).Times(1);
    EXPECT_CALL(*m_mockFileSystem, exists(m_destPath)).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, exists(m_destPath + m_testExtractedFile)).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, exists(m_destPath + m_testExtractedFile + "1")).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, exists(m_destPath + m_testExtractedFile + "2")).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, exists(m_destPath + m_testExtractedFile + "3")).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, makedirs(m_destPath)).Times(1);
    addCleanupChecksToMockFileSystem();

    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, moveFileTryCopy(_, _)).Times(4);
    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, m_raTmpFile))
        .WillOnce(Return("shastring"));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);

    nlohmann::json action = getDownloadObject(decompress, "", 1024, false);
    nlohmann::json response = downloadFileAction.run(action.dump());

    EXPECT_EQ(response["result"], ResponseResult::SUCCESS);
    EXPECT_EQ(response["httpStatus"], HTTP_STATUS_OK);
    EXPECT_FALSE(response.contains("errorType"));
    EXPECT_FALSE(response.contains("errorMessage"));

    EXPECT_TRUE(appenderContains("Beginning download to /path/to/download/to/"));
    EXPECT_TRUE(appenderContains("Downloaded file: /opt/sophos-spl/plugins/responseactions/tmp/tmp_download.zip"));
    EXPECT_TRUE(appenderContains("Extracted 4 files from archive"));
    EXPECT_TRUE(appenderContains("/path/to/download/to/download.txt downloaded successfully"));
    EXPECT_TRUE(appenderContains("/path/to/download/to/download.txt1 downloaded successfully"));
    EXPECT_TRUE(appenderContains("/path/to/download/to/download.txt2 downloaded successfully"));
    EXPECT_TRUE(appenderContains("/path/to/download/to/download.txt3 downloaded successfully"));
}

TEST_F(DownloadFileTests, SuccessfulDownload_Direct_Decompress_ManyFilesTargetPathIsDirectory_WithSubDirectories)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    bool decompress = true;
    Path subDirs = "subDir1/subDir2/";

    setupMockZipUtils();
    addResponseToMockRequester(HTTP_STATUS_OK, ResponseErrorCode::OK);

    addDiskSpaceExpectsToMockFileSystem();
    addListFilesExpectsToMockFileSystem(decompress, true, subDirs);
    EXPECT_CALL(*m_mockFileSystem, exists(m_raExtractTmpDir)).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, makedirs(m_raExtractTmpDir)).Times(1);
    EXPECT_CALL(*m_mockFileSystem, exists(m_destPath)).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, exists(m_destPath + subDirs + m_testExtractedFile)).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, exists(m_destPath + subDirs + m_testExtractedFile + "1")).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, exists(m_destPath + subDirs + m_testExtractedFile + "2")).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, exists(m_destPath + subDirs + m_testExtractedFile + "3")).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, makedirs(m_destPath)).Times(1);
    Path subDirNoSlash = subDirs;
    subDirNoSlash.pop_back();
    EXPECT_CALL(*m_mockFileSystem, makedirs(m_destPath + subDirNoSlash)).Times(AnyNumber());
    addCleanupChecksToMockFileSystem();

    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, moveFileTryCopy(_, _)).Times(4);
    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, m_raTmpFile))
        .WillOnce(Return("shastring"));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);

    nlohmann::json action = getDownloadObject(decompress, "", 1024, false);
    nlohmann::json response = downloadFileAction.run(action.dump());

    EXPECT_EQ(response["result"], ResponseResult::SUCCESS);
    EXPECT_EQ(response["httpStatus"], HTTP_STATUS_OK);
    EXPECT_FALSE(response.contains("errorType"));
    EXPECT_FALSE(response.contains("errorMessage"));

    EXPECT_TRUE(appenderContains("Beginning download to /path/to/download/to/"));
    EXPECT_TRUE(appenderContains("Downloaded file: /opt/sophos-spl/plugins/responseactions/tmp/tmp_download.zip"));
    EXPECT_TRUE(appenderContains("Extracted 4 files from archive"));
    EXPECT_TRUE(appenderContains("/path/to/download/to/subDir1/subDir2/download.txt downloaded successfully"));
    EXPECT_TRUE(appenderContains("/path/to/download/to/subDir1/subDir2/download.txt1 downloaded successfully"));
    EXPECT_TRUE(appenderContains("/path/to/download/to/subDir1/subDir2/download.txt2 downloaded successfully"));
    EXPECT_TRUE(appenderContains("/path/to/download/to/subDir1/subDir2/download.txt3 downloaded successfully"));
}

TEST_F(DownloadFileTests, SuccessfulDownload_Direct_Decompress_ManyFilesTargetPathIsFile)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    std::string expectedMsg = "Ignoring filepath in targetPath field as the archive contains multiple files";
    bool decompress = true;

    setupMockZipUtils();
    addResponseToMockRequester(HTTP_STATUS_OK, ResponseErrorCode::OK);

    addDiskSpaceExpectsToMockFileSystem();
    addListFilesExpectsToMockFileSystem(decompress, true);
    EXPECT_CALL(*m_mockFileSystem, exists(m_raExtractTmpDir)).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, makedirs(m_raExtractTmpDir)).Times(1);
    EXPECT_CALL(*m_mockFileSystem, exists(m_destPath)).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, exists(m_destPath + m_testExtractedFile)).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, exists(m_destPath + m_testExtractedFile + "1")).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, exists(m_destPath + m_testExtractedFile + "2")).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, exists(m_destPath + m_testExtractedFile + "3")).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, makedirs(m_destPath)).Times(1);
    addCleanupChecksToMockFileSystem();

    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, moveFileTryCopy(_, _)).Times(4);
    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, m_raTmpFile))
        .WillOnce(Return("shastring"));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);

    nlohmann::json action = getDownloadObject(decompress);
    nlohmann::json response = downloadFileAction.run(action.dump());

    EXPECT_EQ(response["result"], ResponseResult::SUCCESS);
    EXPECT_EQ(response["httpStatus"], HTTP_STATUS_OK);
    EXPECT_FALSE(response.contains("errorType"));
    EXPECT_EQ(response["errorMessage"], expectedMsg);

    EXPECT_TRUE(appenderContains("Beginning download to /path/to/download/to/"));
    EXPECT_TRUE(appenderContains("Downloaded file: /opt/sophos-spl/plugins/responseactions/tmp/tmp_download.zip"));
    EXPECT_TRUE(appenderContains("Extracted 4 files from archive"));
    EXPECT_TRUE(appenderContains("/path/to/download/to/download.txt downloaded successfully"));
    EXPECT_TRUE(appenderContains("/path/to/download/to/download.txt1 downloaded successfully"));
    EXPECT_TRUE(appenderContains("/path/to/download/to/download.txt2 downloaded successfully"));
    EXPECT_TRUE(appenderContains("/path/to/download/to/download.txt3 downloaded successfully"));
    EXPECT_TRUE(appenderContains(expectedMsg));
}

//Decompress End to End Password

TEST_F(DownloadFileTests, SuccessfulDownload_Direct_Decompressed_Password)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    bool decompress = true;
    setupMockZipUtils(0, m_password);

    addResponseToMockRequester(HTTP_STATUS_OK, ResponseErrorCode::OK);

    addDiskSpaceExpectsToMockFileSystem();
    addListFilesExpectsToMockFileSystem(decompress);
    addDownloadAndExtractExpectsToMockFileSystem(m_destPath + m_testExtractedFile, true);
    addCleanupChecksToMockFileSystem();

    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, moveFileTryCopy(m_raExtractTmpDir + "/" + m_testExtractedFile, m_destPath + m_testExtractedFile)).Times(1);
    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, m_raTmpFile))
        .WillOnce(Return("shastring"));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);

    nlohmann::json action = getDownloadObject(decompress, m_password);
    nlohmann::json response = downloadFileAction.run(action.dump());

    EXPECT_EQ(response["result"], ResponseResult::SUCCESS);
    EXPECT_EQ(response["httpStatus"], HTTP_STATUS_OK);
    EXPECT_FALSE(response.contains("errorType"));
    EXPECT_FALSE(response.contains("errorMessage"));

    EXPECT_TRUE(appenderContains("Beginning download to /path/to/download/to/download.txt"));
    EXPECT_TRUE(appenderContains("Downloading directly"));
    EXPECT_TRUE(appenderContains("/path/to/download/to/download.txt downloaded successfully"));
}

TEST_F(DownloadFileTests, SuccessfulDownload_Direct_Decompress_WrongPassword)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    const std::string expectedErrMsg = "Error unzipping /opt/sophos-spl/plugins/responseactions/tmp/tmp_download.zip due to bad password";

    setupMockZipUtils(UNZ_BADPASSWORD, m_password);

    addResponseToMockRequester(HTTP_STATUS_OK, ResponseErrorCode::OK);

    addDiskSpaceExpectsToMockFileSystem();
    addListFilesExpectsToMockFileSystem();

    EXPECT_CALL(*m_mockFileSystem, exists(m_raExtractTmpDir)).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, makedirs(m_raExtractTmpDir)).Times(1);
    addCleanupChecksToMockFileSystem();

    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, m_raTmpFile))
        .WillOnce(Return("shastring"));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);

    nlohmann::json action = getDownloadObject(true, m_password);
    nlohmann::json response = downloadFileAction.run(action.dump());


    EXPECT_EQ(response["result"], ResponseResult::ERROR);
    EXPECT_EQ(response["httpStatus"], HTTP_STATUS_OK);
    EXPECT_EQ(response["errorType"], "invalid_password");
    EXPECT_EQ(response["errorMessage"], expectedErrMsg);

    EXPECT_TRUE(appenderContains("Beginning download to /path/to/download/to/download.txt"));
    EXPECT_TRUE(appenderContains("Downloaded file: /opt/sophos-spl/plugins/responseactions/tmp/tmp_download.zip"));
    EXPECT_TRUE(appenderContains(expectedErrMsg));
}

TEST_F(DownloadFileTests, SuccessfulDownload_Direct_Decompress_EmptyPassword)
{
    //Empty password should just mean we dont try to unzip using the password
    UsingMemoryAppender memoryAppenderHolder(*this);

    const std::string expectedErrMsg = "Error unzipping /opt/sophos-spl/plugins/responseactions/tmp/tmp_download.zip due to bad password";

    auto mockZip = std::make_unique<NiceMock<MockZipUtils>>();
    EXPECT_CALL(*mockZip, unzip(_, _)).WillOnce(Return(UNZ_OK));
    EXPECT_CALL(*mockZip, unzip(_, _, _, _)).Times(0); //The password unzip call
    Common::ZipUtilities::replaceZipUtils(std::move(mockZip));

    addResponseToMockRequester(HTTP_STATUS_OK, ResponseErrorCode::OK);

    addDiskSpaceExpectsToMockFileSystem();
    addListFilesExpectsToMockFileSystem(true);
    addDownloadAndExtractExpectsToMockFileSystem(m_destPath + m_testExtractedFile, true);
    addCleanupChecksToMockFileSystem(true);

    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, moveFileTryCopy(m_raExtractTmpDir + "/" + m_testExtractedFile, m_destPath + m_testExtractedFile)).Times(1);
    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, m_raTmpFile))
        .WillOnce(Return("shastring"));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);

    nlohmann::json action = getDownloadObject(true, "");
    nlohmann::json response = downloadFileAction.run(action.dump());

    EXPECT_EQ(response["result"], ResponseResult::SUCCESS);
    EXPECT_EQ(response["httpStatus"], HTTP_STATUS_OK);
    EXPECT_FALSE(response.contains("errorType"));
    EXPECT_FALSE(response.contains("errorMessage"));
    EXPECT_TRUE(appenderContains("/path/to/download/to/download.txt downloaded successfully"));
}

TEST_F(DownloadFileTests, SuccessfulDownload_Direct_Decompress_LargePassword)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    const std::string largePassword(30000, 'a');

    setupMockZipUtils(UNZ_OK, largePassword);

    addResponseToMockRequester(HTTP_STATUS_OK, ResponseErrorCode::OK);

    addDiskSpaceExpectsToMockFileSystem();
    addListFilesExpectsToMockFileSystem(true);
    addDownloadAndExtractExpectsToMockFileSystem(m_destPath + m_testExtractedFile, true);
    addCleanupChecksToMockFileSystem(true);

    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    EXPECT_CALL(
        *m_mockFileSystem,
        moveFileTryCopy(m_raExtractTmpDir + "/" + m_testExtractedFile, m_destPath + m_testExtractedFile))
        .Times(1);
    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, m_raTmpFile))
        .WillOnce(Return("shastring"));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);

    nlohmann::json action = getDownloadObject(true, largePassword);
    nlohmann::json response = downloadFileAction.run(action.dump());

    EXPECT_EQ(response["result"], ResponseResult::SUCCESS);
    EXPECT_EQ(response["httpStatus"], HTTP_STATUS_OK);
    EXPECT_FALSE(response.contains("errorType"));
    EXPECT_FALSE(response.contains("errorMessage"));
    EXPECT_TRUE(appenderContains("/path/to/download/to/download.txt downloaded successfully"));
}

//Proxy

TEST_F(DownloadFileTests, SuccessfulDownload_WithProxy_NotDecompressed)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    addResponseToMockRequester(HTTP_STATUS_OK, ResponseErrorCode::OK);

    std::string obfuscatedCreds = "CCD4E57ZjW+t5XPiMSJH1TurG3MfWCN3DpjJRINMwqNaWl+3zzlVIdyVmifCHUwcmaX6+YTSyyBM8SslIIGV5rUw";
    std::string content = R"({"proxy":"localhost","credentials":")" + obfuscatedCreds + R"("})";

    addDiskSpaceExpectsToMockFileSystem();
    addListFilesExpectsToMockFileSystem();
    addDownloadAndExtractExpectsToMockFileSystem(m_destPath + m_testZipFile);
    addCleanupChecksToMockFileSystem();

    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, readFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(content));
    EXPECT_CALL(*m_mockFileSystem, moveFileTryCopy(m_raTmpFile, m_destPath + m_testZipFile)).Times(1);
    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, m_raTmpFile))
        .WillOnce(Return("shastring"));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);

    nlohmann::json action = getDownloadObject();
    nlohmann::json response = downloadFileAction.run(action.dump());
    

    EXPECT_EQ(response["result"], ResponseResult::SUCCESS);
    EXPECT_EQ(response["httpStatus"], HTTP_STATUS_OK);
    EXPECT_FALSE(response.contains("errorType"));
    EXPECT_FALSE(response.contains("errorMessage"));

    EXPECT_TRUE(appenderContains("Beginning download to /path/to/download/to/testdownload.zip"));
    EXPECT_TRUE(appenderContains("Downloading via proxy: localhost"));
    EXPECT_TRUE(appenderContains("/path/to/download/to/testdownload.zip downloaded successfully"));
}

TEST_F(DownloadFileTests, SuccessfulDownload_WithProxy_Decompressed)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    std::string obfuscatedCreds = "CCD4E57ZjW+t5XPiMSJH1TurG3MfWCN3DpjJRINMwqNaWl+3zzlVIdyVmifCHUwcmaX6+YTSyyBM8SslIIGV5rUw";
    std::string content = R"({"proxy":"localhost","credentials":")" + obfuscatedCreds + R"("})";

    bool decompress = true;
    setupMockZipUtils();

    addResponseToMockRequester(HTTP_STATUS_OK, ResponseErrorCode::OK);

    addDiskSpaceExpectsToMockFileSystem();
    addListFilesExpectsToMockFileSystem(decompress);
    addDownloadAndExtractExpectsToMockFileSystem(m_destPath + m_testExtractedFile, decompress);
    addCleanupChecksToMockFileSystem();

    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, readFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(content));
    EXPECT_CALL(*m_mockFileSystem, moveFileTryCopy(m_raExtractTmpDir + "/" + m_testExtractedFile, m_destPath + m_testExtractedFile)).Times(1);
    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, m_raTmpFile))
        .WillOnce(Return("shastring"));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);

    nlohmann::json action = getDownloadObject(decompress);
    nlohmann::json response = downloadFileAction.run(action.dump());

    EXPECT_EQ(response["result"], ResponseResult::SUCCESS);
    EXPECT_EQ(response["httpStatus"], HTTP_STATUS_OK);
    EXPECT_FALSE(response.contains("errorType"));
    EXPECT_FALSE(response.contains("errorMessage"));

    EXPECT_TRUE(appenderContains("Beginning download to /path/to/download/to/download.txt"));
    EXPECT_TRUE(appenderContains("Downloading via proxy: localhost"));
    EXPECT_TRUE(appenderContains("/path/to/download/to/download.txt downloaded successfully"));
}

TEST_F(DownloadFileTests, ProxyFailureFallsbackDirect_NotDecompressed)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    Common::HttpRequests::Response failhttpresponse;
    Common::HttpRequests::Response httpresponse;
    httpresponse.status = Common::HttpRequests::HTTP_STATUS_OK;
    httpresponse.errorCode = Common::HttpRequests::ResponseErrorCode::OK;
    failhttpresponse.status = 400;
    failhttpresponse.errorCode = Common::HttpRequests::ResponseErrorCode::OK;
    EXPECT_CALL(*m_mockHttpRequester, get(_)).Times(2).WillOnce(Return(failhttpresponse)).WillOnce(Return(httpresponse));

    std::string obfuscatedCreds = "CCD4E57ZjW+t5XPiMSJH1TurG3MfWCN3DpjJRINMwqNaWl+3zzlVIdyVmifCHUwcmaX6+YTSyyBM8SslIIGV5rUw";
    std::string content = R"({"proxy":"localhost","credentials":")" + obfuscatedCreds + R"("})";

    addDiskSpaceExpectsToMockFileSystem();
    addListFilesExpectsToMockFileSystem();
    addDownloadAndExtractExpectsToMockFileSystem(m_destPath + m_testZipFile);
    addCleanupChecksToMockFileSystem();

    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, readFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(content));
    EXPECT_CALL(*m_mockFileSystem, moveFileTryCopy(m_raTmpFile, m_destPath + m_testZipFile)).Times(1);
    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, m_raTmpFile))
        .WillOnce(Return("shastring"));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);

    nlohmann::json action = getDownloadObject();
    nlohmann::json response = downloadFileAction.run(action.dump());

    EXPECT_EQ(response["result"], ResponseResult::SUCCESS);
    EXPECT_EQ(response["httpStatus"], HTTP_STATUS_OK);
    EXPECT_FALSE(response.contains("errorType"));
    EXPECT_FALSE(response.contains("errorMessage"));

    EXPECT_TRUE(appenderContains("Downloading via proxy: localhost"));
    EXPECT_TRUE(appenderContains("Failed to download, Error code: 400"));
    EXPECT_TRUE(appenderContains("Connection with proxy failed, going direct"));
    EXPECT_TRUE(appenderContains("/path/to/download/to/testdownload.zip downloaded successfully"));
}

TEST_F(DownloadFileTests, ProxyFailureFallsbackDirect_Decompressed)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    bool decompress = true;
    setupMockZipUtils();

    Common::HttpRequests::Response failhttpresponse;
    Common::HttpRequests::Response httpresponse;
    httpresponse.status = Common::HttpRequests::HTTP_STATUS_OK;
    httpresponse.errorCode = Common::HttpRequests::ResponseErrorCode::OK;
    failhttpresponse.status = 400;
    failhttpresponse.errorCode = Common::HttpRequests::ResponseErrorCode::OK;
    EXPECT_CALL(*m_mockHttpRequester, get(_)).Times(2).WillOnce(Return(failhttpresponse)).WillOnce(Return(httpresponse));

    std::string obfuscatedCreds = "CCD4E57ZjW+t5XPiMSJH1TurG3MfWCN3DpjJRINMwqNaWl+3zzlVIdyVmifCHUwcmaX6+YTSyyBM8SslIIGV5rUw";
    std::string content = R"({"proxy":"localhost","credentials":")" + obfuscatedCreds + R"("})";

    addDiskSpaceExpectsToMockFileSystem();
    addListFilesExpectsToMockFileSystem(decompress);
    addDownloadAndExtractExpectsToMockFileSystem(m_destPath + m_testExtractedFile, decompress);
    addCleanupChecksToMockFileSystem();

    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, readFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(content));
    EXPECT_CALL(*m_mockFileSystem, moveFileTryCopy(m_raExtractTmpDir + "/" + m_testExtractedFile, m_destPath + m_testExtractedFile)).Times(1);
    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, m_raTmpFile))
        .WillOnce(Return("shastring"));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);

    nlohmann::json action = getDownloadObject(decompress);
    nlohmann::json response = downloadFileAction.run(action.dump());

    EXPECT_EQ(response["result"], ResponseResult::SUCCESS);
    EXPECT_EQ(response["httpStatus"], HTTP_STATUS_OK);
    EXPECT_FALSE(response.contains("errorType"));
    EXPECT_FALSE(response.contains("errorMessage"));

    EXPECT_TRUE(appenderContains("Downloading via proxy: localhost"));
    EXPECT_TRUE(appenderContains("Failed to download, Error code: 400"));
    EXPECT_TRUE(appenderContains("Connection with proxy failed, going direct"));
    EXPECT_TRUE(appenderContains("/path/to/download/to/download.txt downloaded successfully"));
}

//Initialchecks failures
TEST_F(DownloadFileTests, DirectLargeExpiration)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    const std::string expectedMsg = "Download file action has expired";
    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);

    std::string action (
        R"({"type": "sophos.mgt.action.DownloadFile"
        ,"timeout": 1000
        ,"sizeBytes": 1000
        ,"url": "https://s3.com/download.zip"
        ,"targetPath": "path"
        ,"sha256": "sha"
        ,"expiration": 18446744073709551616})");

    nlohmann::json response = downloadFileAction.run(action);

    EXPECT_EQ(response["type"], DOWNLOAD_FILE_RESPONSE_TYPE);
    EXPECT_EQ(response["result"], ResponseResult::EXPIRED);
    EXPECT_EQ(response["errorMessage"], expectedMsg);
    EXPECT_FALSE(response.contains("errorType"));
    EXPECT_FALSE(response.contains("httpStatus"));
    EXPECT_FALSE(response.contains("duration"));

    EXPECT_TRUE(appenderContains(expectedMsg));
}

TEST_F(DownloadFileTests, DirectLargeSizeBytes)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);

    std::string action (
        R"({"type" : "sophos.mgt.action.DownloadFile"
        ,"timeout" : 1000
        ,"sizeBytes" : 18446744073709551616
        ,"url" : "https://s3.com/download.zip"
        ,"targetPath" : "/path/to/download/to/"
        ,"sha256" : "sha"
        ,"expiration" : 144444000000004})");

    nlohmann::json response = downloadFileAction.run(action);

    EXPECT_EQ(response["type"], DOWNLOAD_FILE_RESPONSE_TYPE);
    EXPECT_EQ(response["result"], ResponseResult::ERROR);
    EXPECT_EQ(response["errorMessage"], "Error parsing command from Central");
    EXPECT_FALSE(response.contains("errorType"));
    EXPECT_FALSE(response.contains("httpStatus"));
    EXPECT_FALSE(response.contains("duration"));

    EXPECT_TRUE(appenderContains("Invalid command format. sizeBytes field has been evaluated to 0. Very large values can also cause this."));
}

TEST_F(DownloadFileTests, DirectNegativeExpiration)
{
    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);

    nlohmann::json action = getDownloadObject();
    action["expiration"] = -123456;
    nlohmann::json response = downloadFileAction.run(action.dump());

    EXPECT_EQ(response["type"], DOWNLOAD_FILE_RESPONSE_TYPE);
    EXPECT_EQ(response["result"], ResponseResult::ERROR);
    EXPECT_EQ(response["errorMessage"], "Error parsing command from Central");
    EXPECT_FALSE(response.contains("errorType"));
    EXPECT_FALSE(response.contains("httpStatus"));
    EXPECT_FALSE(response.contains("duration"));
}

TEST_F(DownloadFileTests, DirectNegativeSizeBytes)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);

    nlohmann::json action = getDownloadObject();
    action["sizeBytes"] = -123456;
    nlohmann::json response = downloadFileAction.run(action.dump());

    EXPECT_EQ(response["type"], DOWNLOAD_FILE_RESPONSE_TYPE);
    EXPECT_EQ(response["result"], ResponseResult::ERROR);
    EXPECT_EQ(response["errorMessage"], "Error parsing command from Central");
    EXPECT_FALSE(response.contains("errorType"));
    EXPECT_FALSE(response.contains("httpStatus"));
    EXPECT_FALSE(response.contains("duration"));

    EXPECT_TRUE(appenderContains("Invalid command format. Failed to process DownloadInfo from action JSON: sizeBytes is a negative value: -123456"));
}

TEST_F(DownloadFileTests, FileToDownloadIsAboveMaxAllowedFileSize)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    std::string expectedErrMsg = "Downloading file to /path/to/download/to/testdownload.zip failed due to size: 3221225472 is too large";

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);

    nlohmann::json action = getDownloadObject(false, "", 1024UL * 1024 * 1024 * 3);
    nlohmann::json response = downloadFileAction.run(action.dump());

    EXPECT_EQ(response["result"], ResponseResult::ERROR);
    EXPECT_FALSE(response.contains("errorType"));
    EXPECT_FALSE(response.contains("httpStatus"));
    EXPECT_EQ(response["errorMessage"], expectedErrMsg);

    EXPECT_TRUE(appenderContains(expectedErrMsg));
}

TEST_F(DownloadFileTests, EmptyPath)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    const std::string expectedMsg = "Invalid command format. Failed to process DownloadInfo from action JSON: targetPath field is empty";

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);
    nlohmann::json action = getDownloadObject();
    action["targetPath"] = "";
    nlohmann::json response = downloadFileAction.run(action.dump());

    EXPECT_EQ(response["result"], ResponseResult::ERROR);
    EXPECT_EQ(response["errorMessage"], "Error parsing command from Central");
    EXPECT_FALSE(response.contains("errorType"));

    EXPECT_TRUE(appenderContains(expectedMsg));
}

TEST_F(DownloadFileTests, InvalidAbsolutePath)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    const std::string expectedErrMsg = "notapath is not a valid absolute path";

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);
    nlohmann::json action = getDownloadObject();
    action["targetPath"] = "notapath";
    nlohmann::json response = downloadFileAction.run(action.dump());

    EXPECT_EQ(response["result"], ResponseResult::ERROR);
    EXPECT_EQ(response["errorMessage"], expectedErrMsg);
    EXPECT_EQ(response["errorType"], "invalid_path");

    EXPECT_TRUE(appenderContains(expectedErrMsg));
}

TEST_F(DownloadFileTests, NotEnoughSpaceOnRATmpDisk)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    const std::string expectedErrStr = "Not enough space to complete download action: Sophos install disk has 1, destination disk has 1048576";

    addSingleCleanupChecksToMockFileSystem();
    addDiskSpaceExpectsToMockFileSystem(1);
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);

    nlohmann::json action = getDownloadObject();
    nlohmann::json response = downloadFileAction.run(action.dump());


    EXPECT_EQ(response["result"], ResponseResult::ERROR);
    EXPECT_EQ(response["errorMessage"], expectedErrStr);
    EXPECT_EQ(response["errorType"], "not_enough_space");

    EXPECT_TRUE(appenderContains(expectedErrStr));
}

TEST_F(DownloadFileTests, NotEnoughSpaceOnDestDisk)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    const std::string expectedErrStr = "Not enough space to complete download action: Sophos install disk has 1048576, destination disk has 1";

    addSingleCleanupChecksToMockFileSystem();
    addDiskSpaceExpectsToMockFileSystem(1024 * 1024, 1);
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);

    nlohmann::json action = getDownloadObject();
    nlohmann::json response = downloadFileAction.run(action.dump());


    EXPECT_EQ(response["result"], ResponseResult::ERROR);
    EXPECT_EQ(response["errorMessage"], expectedErrStr);
    EXPECT_EQ(response["errorType"], "not_enough_space");

    EXPECT_TRUE(appenderContains(expectedErrStr));
}

TEST_F(DownloadFileTests, HandlesWhenCantAssessDiskSpace_FileSystemException)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    const std::string expectedErrStr = "Cant determine disk space on filesystem: exception";

    addSingleCleanupChecksToMockFileSystem();
    EXPECT_CALL(*m_mockFileSystem, getDiskSpaceInfo(_, _))
        .WillOnce(Throw(Common::FileSystem::IFileSystemException("exception")));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);

    nlohmann::json action = getDownloadObject();
    nlohmann::json response = downloadFileAction.run(action.dump());


    EXPECT_EQ(response["result"], ResponseResult::ERROR);
    EXPECT_EQ(response["errorMessage"], expectedErrStr);
    EXPECT_FALSE(response.contains("errorType"));

    EXPECT_TRUE(appenderContains(expectedErrStr));
}

TEST_F(DownloadFileTests, HandlesWhenCantAssessDiskSpace_Exception)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    const std::string expectedErrStr = "Unknown exception when calculating disk space on filesystem: std::exception";

    addSingleCleanupChecksToMockFileSystem();
    EXPECT_CALL(*m_mockFileSystem, getDiskSpaceInfo(_, _))
        .WillOnce(Throw(std::exception()));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);

    nlohmann::json action = getDownloadObject();
    nlohmann::json response = downloadFileAction.run(action.dump());


    EXPECT_EQ(response["result"], ResponseResult::ERROR);
    EXPECT_EQ(response["errorMessage"], expectedErrStr);
    EXPECT_FALSE(response.contains("errorType"));

    EXPECT_TRUE(appenderContains(expectedErrStr));
}

//More thoroughly checked in actionutils
TEST_F(DownloadFileTests, ErrorParsingJson)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto httpRequester = std::make_shared<StrictMock<MockHTTPRequester>>();
    ResponseActionsImpl::DownloadFileAction downloadFileAction(httpRequester,m_mockSignalHandler, m_mockSysCallWrapper);

    nlohmann::json response = downloadFileAction.run("");

    EXPECT_EQ(response["result"], ResponseResult::ERROR);
    EXPECT_EQ(response["errorMessage"], "Error parsing command from Central");

    EXPECT_TRUE(appenderContains("Invalid command format. Download action JSON is empty"));
}

TEST_F(DownloadFileTests, actionExpired)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto httpRequester = std::make_shared<StrictMock<MockHTTPRequester>>();
    ResponseActionsImpl::DownloadFileAction downloadFileAction(httpRequester,m_mockSignalHandler, m_mockSysCallWrapper);
    nlohmann::json action = getDownloadObject();
    action["expiration"] = 0;
    nlohmann::json response = downloadFileAction.run(action.dump());

    EXPECT_EQ(response["result"],4);
    EXPECT_EQ(response["errorMessage"],"Download file action has expired");

    EXPECT_TRUE(appenderContains("Download file action has expired"));
}

//TargetPath already exists

TEST_F(DownloadFileTests, TargetPathAlreadyExists_NotDecompressed)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    std::string expectedErrMsg = "Path /path/to/download/to/testdownload.zip already exists";

    addResponseToMockRequester(HTTP_STATUS_OK, ResponseErrorCode::OK);

    addDiskSpaceExpectsToMockFileSystem();
    addListFilesExpectsToMockFileSystem();
    EXPECT_CALL(*m_mockFileSystem, makedirs(m_destPath)).Times(1);
    EXPECT_CALL(*m_mockFileSystem, exists(m_destPath)).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, exists(m_destPath + m_testZipFile)).WillOnce(Return(true));
    addCleanupChecksToMockFileSystem();

    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, m_raTmpFile))
        .WillOnce(Return("shastring"));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);

    nlohmann::json action = getDownloadObject();
    nlohmann::json response = downloadFileAction.run(action.dump());

    EXPECT_EQ(response["result"], ResponseResult::ERROR);
    EXPECT_EQ(response["errorMessage"], expectedErrMsg);
    EXPECT_EQ(response["errorType"], "path_exists");

    EXPECT_TRUE(appenderContains(expectedErrMsg));
}

TEST_F(DownloadFileTests, TargetPathAlreadyExists_DecompressedSingleFile)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    std::string expectedErrMsg = "Path /path/to/download/to/download.txt already exists";

    addResponseToMockRequester(HTTP_STATUS_OK, ResponseErrorCode::OK);
    setupMockZipUtils();

    addDiskSpaceExpectsToMockFileSystem();
    addListFilesExpectsToMockFileSystem(true);
    EXPECT_CALL(*m_mockFileSystem, makedirs(m_destPath)).Times(1);
    EXPECT_CALL(*m_mockFileSystem, exists(m_raExtractTmpDir)).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, makedirs(m_raExtractTmpDir)).Times(1);
    EXPECT_CALL(*m_mockFileSystem, exists(m_destPath)).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, exists(m_destPath + m_testExtractedFile)).WillOnce(Return(true));
    addCleanupChecksToMockFileSystem();

    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, m_raTmpFile))
        .WillOnce(Return("shastring"));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);

    nlohmann::json action = getDownloadObject(true);
    nlohmann::json response = downloadFileAction.run(action.dump());


    EXPECT_EQ(response["result"], ResponseResult::ERROR);
    EXPECT_EQ(response["errorMessage"], expectedErrMsg);
    EXPECT_EQ(response["errorType"], "path_exists");

    EXPECT_TRUE(appenderContains(expectedErrMsg));
}

TEST_F(DownloadFileTests, SuccessfulDownload_Direct_Decompress_AbortCopyingIfOneFileExists)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    std::string expectedMsg = "Path /path/to/download/to/download.txt1 already exists";

    bool decompress = true;

    setupMockZipUtils();
    addResponseToMockRequester(HTTP_STATUS_OK, ResponseErrorCode::OK);

    addDiskSpaceExpectsToMockFileSystem();
    addListFilesExpectsToMockFileSystem(decompress, true);
    EXPECT_CALL(*m_mockFileSystem, exists(m_raExtractTmpDir)).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, makedirs(m_raExtractTmpDir)).Times(1);
    EXPECT_CALL(*m_mockFileSystem, exists(m_destPath)).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, exists(m_destPath + m_testExtractedFile)).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, exists(m_destPath + m_testExtractedFile + "1")).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, makedirs(m_destPath)).Times(1);
    addCleanupChecksToMockFileSystem();

    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, m_raTmpFile))
        .WillOnce(Return("shastring"));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);

    nlohmann::json action = getDownloadObject(decompress, "", 1024, false);
    nlohmann::json response = downloadFileAction.run(action.dump());

    EXPECT_EQ(response["result"], ResponseResult::ERROR);
    EXPECT_EQ(response["httpStatus"], HTTP_STATUS_OK);
    EXPECT_EQ(response["errorType"], "path_exists");
    EXPECT_EQ(response["errorMessage"], expectedMsg);

    EXPECT_TRUE(appenderContains("Beginning download to /path/to/download/to/"));
    EXPECT_TRUE(appenderContains("Downloaded file: /opt/sophos-spl/plugins/responseactions/tmp/tmp_download.zip"));
    EXPECT_TRUE(appenderContains("Extracted 4 files from archive"));
    EXPECT_TRUE(appenderContains("A file in the extracted archive already existed on destination path, aborting"));
    EXPECT_TRUE(appenderContains(expectedMsg));

    EXPECT_FALSE(appenderContains("/path/to/download/to/download.txt downloaded successfully"));
    EXPECT_FALSE(appenderContains("/path/to/download/to/download.txt1 downloaded successfully"));
    EXPECT_FALSE(appenderContains("/path/to/download/to/download.txt2 downloaded successfully"));
    EXPECT_FALSE(appenderContains("/path/to/download/to/download.txt3 downloaded successfully"));
}

//Sha256 issues

TEST_F(DownloadFileTests, FileSha256CantBeCalculatedDueToAccess)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    const std::string expectedErrStr = "testdownload.zip cannot be accessed";

    addResponseToMockRequester(HTTP_STATUS_OK, ResponseErrorCode::OK);

    addDiskSpaceExpectsToMockFileSystem();
    addListFilesExpectsToMockFileSystem();
    addCleanupChecksToMockFileSystem();
    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, m_raTmpFile))
        .WillOnce(Throw(Common::FileSystem::IFileSystemException("")));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);
    nlohmann::json action = getDownloadObject();

    nlohmann::json response = downloadFileAction.run(action.dump());
    
    EXPECT_EQ(response["result"],1);
    EXPECT_EQ(response["httpStatus"], HTTP_STATUS_OK);
    EXPECT_EQ(response["errorType"],"access_denied");
    EXPECT_EQ(response["errorMessage"],expectedErrStr);

    EXPECT_TRUE(appenderContains(expectedErrStr));
}

TEST_F(DownloadFileTests, FileSha256CantBeCalculatedDueToOtherReason)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    const std::string expectedErrStr = "Unknown error when calculating digest of testdownload.zip: std::exception";

    addResponseToMockRequester(HTTP_STATUS_OK, ResponseErrorCode::OK);

    addDiskSpaceExpectsToMockFileSystem();
    addListFilesExpectsToMockFileSystem();
    addCleanupChecksToMockFileSystem();
    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, m_raTmpFile))
        .WillOnce(Throw(std::exception()));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);
    nlohmann::json action = getDownloadObject();

    nlohmann::json response = downloadFileAction.run(action.dump());
    
    EXPECT_EQ(response["result"], ResponseResult::ERROR);
    EXPECT_EQ(response["httpStatus"], HTTP_STATUS_OK);
    EXPECT_FALSE(response.contains("errorType"));
    EXPECT_EQ(response["errorMessage"], expectedErrStr);

    EXPECT_TRUE(appenderContains(expectedErrStr));
}


TEST_F(DownloadFileTests, Sha256IsWrong)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    const std::string expectedErrStr = "Calculated Sha256 (adifferentshastring) doesnt match that of file downloaded (shastring)";

    addResponseToMockRequester(HTTP_STATUS_OK, ResponseErrorCode::OK);

    addDiskSpaceExpectsToMockFileSystem();
    addListFilesExpectsToMockFileSystem();
    addCleanupChecksToMockFileSystem();
    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, m_raTmpFile))
        .WillOnce(Return("adifferentshastring"));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);

    nlohmann::json action = getDownloadObject();
    nlohmann::json response = downloadFileAction.run(action.dump());


    EXPECT_EQ(response["result"], ResponseResult::ERROR);
    EXPECT_EQ(response["errorMessage"], expectedErrStr);
    EXPECT_EQ(response["errorType"], "access_denied");

    EXPECT_TRUE(appenderContains(expectedErrStr));
}

TEST_F(DownloadFileTests, Sha256IsHuge)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    const std::string largeSha (30000, 'a');

    addResponseToMockRequester(HTTP_STATUS_OK, ResponseErrorCode::OK);

    addDiskSpaceExpectsToMockFileSystem();
    addDownloadAndExtractExpectsToMockFileSystem(m_destPath + m_testZipFile);
    addListFilesExpectsToMockFileSystem();
    addCleanupChecksToMockFileSystem();
    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, moveFileTryCopy(m_raTmpFile, m_destPath + m_testZipFile)).Times(1);
    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, m_raTmpFile))
        .WillOnce(Return(largeSha));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);

    nlohmann::json action = getDownloadObject();
    action["sha256"] = largeSha;
    nlohmann::json response = downloadFileAction.run(action.dump());

    EXPECT_EQ(response["result"], ResponseResult::SUCCESS);
    EXPECT_EQ(response["httpStatus"], HTTP_STATUS_OK);
    EXPECT_FALSE(response.contains("errorType"));
    EXPECT_FALSE(response.contains("errorMessage"));

    EXPECT_TRUE(appenderContains("Successfully matched sha256 to downloaded file"));
    EXPECT_TRUE(appenderContains("/path/to/download/to/testdownload.zip downloaded successfully"));
}


//Download errors

TEST_F(DownloadFileTests, FailureDueToTimeout)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    const std::string expectedErrStr = "Download timed out";

    addResponseToMockRequester(500, ResponseErrorCode::TIMEOUT);

    addCleanupChecksToMockFileSystem();
    addDiskSpaceExpectsToMockFileSystem();
    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);
    nlohmann::json action = getDownloadObject();

    nlohmann::json response = downloadFileAction.run(action.dump());
    
    EXPECT_EQ(response["result"],2);
    EXPECT_EQ(response["httpStatus"], 500);
    EXPECT_EQ(response["errorType"], "network_error");
    EXPECT_EQ(response["errorMessage"], expectedErrStr);

    EXPECT_TRUE(appenderContains(expectedErrStr));
}


TEST_F(DownloadFileTests, FailureDueToTargetAlreadyExisting)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    const std::string expectedErrStr = "Download failed as target exists already: somethings here already";

    addResponseToMockRequester(500, ResponseErrorCode::DOWNLOAD_TARGET_ALREADY_EXISTS, "somethings here already");

    addCleanupChecksToMockFileSystem();
    addDiskSpaceExpectsToMockFileSystem();
    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);
    nlohmann::json action = getDownloadObject();

    nlohmann::json response = downloadFileAction.run(action.dump());

    EXPECT_EQ(response["result"],1);
    EXPECT_EQ(response["httpStatus"], 500);
    EXPECT_FALSE(response.contains("errorType"));
    EXPECT_EQ(response["errorMessage"], expectedErrStr);

    EXPECT_TRUE(appenderContains(expectedErrStr));
}

TEST_F(DownloadFileTests, FailureDueToNetworkError)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    const std::string expectedErrStr = "Failed to download, Error: I failed";

    addResponseToMockRequester(500, ResponseErrorCode::FAILED, "I failed");

    addCleanupChecksToMockFileSystem();
    addDiskSpaceExpectsToMockFileSystem();
    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);
    nlohmann::json action = getDownloadObject();

    nlohmann::json response = downloadFileAction.run(action.dump());
    
    EXPECT_EQ(response["result"],1);
    EXPECT_EQ(response["httpStatus"], 500);
    EXPECT_EQ(response["errorType"], "network_error");
    EXPECT_EQ(response["errorMessage"], expectedErrStr);

    EXPECT_TRUE(appenderContains(expectedErrStr));
}

TEST_F(DownloadFileTests, FailureDueToServerError)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    const std::string expectedErrStr = "Failed to download, Error code: 500";

    addResponseToMockRequester(500, ResponseErrorCode::OK);

    addCleanupChecksToMockFileSystem();
    addDiskSpaceExpectsToMockFileSystem();
    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);
    nlohmann::json action = getDownloadObject();

    nlohmann::json response = downloadFileAction.run(action.dump());
    
    EXPECT_EQ(response["result"],1);
    EXPECT_EQ(response["httpStatus"], 500);
    EXPECT_EQ(response["errorType"], "network_error");
    EXPECT_EQ(response["errorMessage"], expectedErrStr);

    EXPECT_TRUE(appenderContains(expectedErrStr));
}

TEST_F(DownloadFileTests, FailureDueToServerCertError)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    const std::string expectedErrStr = "Failed to download, Error: SSL issues";

    addResponseToMockRequester(500, ResponseErrorCode::CERTIFICATE_ERROR, "SSL issues");

    addCleanupChecksToMockFileSystem();
    addDiskSpaceExpectsToMockFileSystem();
    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);
    nlohmann::json action = getDownloadObject();

    nlohmann::json response = downloadFileAction.run(action.dump());

    EXPECT_EQ(response["result"],1);
    EXPECT_EQ(response["httpStatus"], 500);
    EXPECT_EQ(response["errorType"], "network_error");
    EXPECT_EQ(response["errorMessage"], expectedErrStr);

    EXPECT_TRUE(appenderContains(expectedErrStr));
}
//FileSystem errors

TEST_F(DownloadFileTests, HandlesWhenCantCreatePathToExtractTo_FileSystemException)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    const bool decompress = true;

    const std::string expectedErrStr = "Unable to create path to extract file to: " + m_raExtractTmpDir + ": exception";

    addResponseToMockRequester(HTTP_STATUS_OK, ResponseErrorCode::OK);

    addDiskSpaceExpectsToMockFileSystem();
    addListFilesExpectsToMockFileSystem();
    EXPECT_CALL(*m_mockFileSystem, exists(m_raExtractTmpDir)).WillOnce(Return(false));
    addCleanupChecksToMockFileSystem(decompress);

    EXPECT_CALL(*m_mockFileSystem, makedirs(m_raExtractTmpDir))
        .WillOnce(Throw(Common::FileSystem::IFileSystemException("exception")));
    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, m_raTmpFile))
        .WillOnce(Return("shastring"));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);

    nlohmann::json action = getDownloadObject(decompress);
    nlohmann::json response = downloadFileAction.run(action.dump());
    

    EXPECT_EQ(response["result"], ResponseResult::ERROR);
    EXPECT_EQ(response["errorMessage"], expectedErrStr);
    EXPECT_EQ(response["errorType"], "access_denied");
    EXPECT_EQ(response["httpStatus"], HTTP_STATUS_OK);

    EXPECT_TRUE(appenderContains(expectedErrStr));
}

TEST_F(DownloadFileTests, HandlesWhenCantCreatePathToExtractTo_Exception)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    const bool decompress = true;

    const std::string expectedErrStr = "Unknown error creating path to extract file to: " + m_raExtractTmpDir + ": std::exception";

    addResponseToMockRequester(HTTP_STATUS_OK, ResponseErrorCode::OK);

    addDiskSpaceExpectsToMockFileSystem();
    addListFilesExpectsToMockFileSystem();
    EXPECT_CALL(*m_mockFileSystem, exists(m_raExtractTmpDir)).WillOnce(Return(false));
    addCleanupChecksToMockFileSystem(decompress);

    EXPECT_CALL(*m_mockFileSystem, makedirs(m_raExtractTmpDir))
        .WillOnce(Throw(std::exception()));
    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, m_raTmpFile))
        .WillOnce(Return("shastring"));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);

    nlohmann::json action = getDownloadObject(decompress);
    nlohmann::json response = downloadFileAction.run(action.dump());
    

    EXPECT_EQ(response["result"], ResponseResult::ERROR);
    EXPECT_EQ(response["errorMessage"], expectedErrStr);
    EXPECT_FALSE(response.contains("errorType"));
    EXPECT_EQ(response["httpStatus"], HTTP_STATUS_OK);

    EXPECT_TRUE(appenderContains(expectedErrStr));
}

TEST_F(DownloadFileTests, HandlesWhenCantCreateDestinationDirectory_FileSystemException)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    const std::string expectedErrStr = "Unable to make directory " + m_destPath + " : exception";

    addResponseToMockRequester(HTTP_STATUS_OK, ResponseErrorCode::OK);

    addDiskSpaceExpectsToMockFileSystem();
    addListFilesExpectsToMockFileSystem();
    addCleanupChecksToMockFileSystem();

    EXPECT_CALL(*m_mockFileSystem, exists(m_destPath)).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, makedirs(m_destPath))
        .WillOnce(Throw(Common::FileSystem::IFileSystemException("exception")));
    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, m_raTmpFile))
        .WillOnce(Return("shastring"));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);

    nlohmann::json action = getDownloadObject();
    nlohmann::json response = downloadFileAction.run(action.dump());
    

    EXPECT_EQ(response["result"], ResponseResult::ERROR);
    EXPECT_EQ(response["errorMessage"], expectedErrStr);
    EXPECT_EQ(response["errorType"], "access_denied");
    EXPECT_EQ(response["httpStatus"], HTTP_STATUS_OK);

    EXPECT_TRUE(appenderContains(expectedErrStr));
}

TEST_F(DownloadFileTests, HandlesWhenCantCreateDestinationDirectory_Exception)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    const std::string expectedErrStr = "Unknown error when making directory " + m_destPath + " : std::exception";

    addResponseToMockRequester(HTTP_STATUS_OK, ResponseErrorCode::OK);

    addDiskSpaceExpectsToMockFileSystem();
    addListFilesExpectsToMockFileSystem();
    addCleanupChecksToMockFileSystem();

    EXPECT_CALL(*m_mockFileSystem, exists(m_destPath)).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, makedirs(m_destPath))
        .WillOnce(Throw(std::exception()));
    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, m_raTmpFile))
        .WillOnce(Return("shastring"));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);

    nlohmann::json action = getDownloadObject();
    nlohmann::json response = downloadFileAction.run(action.dump());
    

    EXPECT_EQ(response["result"], ResponseResult::ERROR);
    EXPECT_EQ(response["errorMessage"], expectedErrStr);
    EXPECT_FALSE(response.contains("errorType"));
    EXPECT_EQ(response["httpStatus"], HTTP_STATUS_OK);

    EXPECT_TRUE(appenderContains(expectedErrStr));
}

TEST_F(DownloadFileTests, HandlesWhenCantCopyFileToFinalDestination_FileSystemException_DestFileDoesntExist)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    const std::string expectedErrStr = "Unable to move " + m_raTmpFile + " to " + m_destPath + m_testZipFile;

    addResponseToMockRequester(HTTP_STATUS_OK, ResponseErrorCode::OK);

    EXPECT_CALL(*m_mockFileSystem, exists(m_destPath + m_testZipFile)).WillOnce(Return(false));
    addDiskSpaceExpectsToMockFileSystem();
    addListFilesExpectsToMockFileSystem();
    addDownloadAndExtractExpectsToMockFileSystem(m_destPath + m_testZipFile);
    addCleanupChecksToMockFileSystem();

    EXPECT_CALL(*m_mockFileSystem, moveFileTryCopy(_, _)).WillOnce(Throw(Common::FileSystem::IFileSystemException(expectedErrStr)));
    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, m_raTmpFile))
        .WillOnce(Return("shastring"));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);

    nlohmann::json action = getDownloadObject();
    nlohmann::json response = downloadFileAction.run(action.dump());

    EXPECT_EQ(response["result"], ResponseResult::ERROR);
    EXPECT_EQ(response["errorMessage"], expectedErrStr);
    EXPECT_EQ(response["errorType"], "access_denied");
    EXPECT_EQ(response["httpStatus"], HTTP_STATUS_OK);

    EXPECT_TRUE(appenderContains(expectedErrStr));
}

TEST_F(DownloadFileTests, HandlesWhenCantCopyFileToFinalDestination_FileSystemException_DestFileDoesExist)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    const std::string expectedErrStr = "Unable to move " + m_raTmpFile + " to " + m_destPath + m_testZipFile;

    addResponseToMockRequester(HTTP_STATUS_OK, ResponseErrorCode::OK);

    EXPECT_CALL(*m_mockFileSystem, exists(m_destPath + m_testZipFile)).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, removeFileOrDirectory(m_destPath + m_testZipFile)).Times(1);
    addDiskSpaceExpectsToMockFileSystem();
    addListFilesExpectsToMockFileSystem();
    addDownloadAndExtractExpectsToMockFileSystem(m_destPath + m_testZipFile);
    addCleanupChecksToMockFileSystem();

    EXPECT_CALL(*m_mockFileSystem, moveFileTryCopy(_, _)).WillOnce(Throw(Common::FileSystem::IFileSystemException(expectedErrStr)));
    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, m_raTmpFile))
        .WillOnce(Return("shastring"));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);

    nlohmann::json action = getDownloadObject();
    nlohmann::json response = downloadFileAction.run(action.dump());

    EXPECT_EQ(response["result"], ResponseResult::ERROR);
    EXPECT_EQ(response["errorMessage"], expectedErrStr);
    EXPECT_EQ(response["errorType"], "not_enough_space");
    EXPECT_EQ(response["httpStatus"], HTTP_STATUS_OK);

    EXPECT_TRUE(appenderContains(expectedErrStr));
}

TEST_F(DownloadFileTests, HandlesWhenCantCopyFileToFinalDestination_FileSystemException_PreviousFilesCopied)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    const std::string expectedErrStr = "Unable to move " + m_raExtractTmpDir + m_testExtractedFile + "3 to " + m_destPath + m_testExtractedFile + "3";
    bool decompress = true;

    setupMockZipUtils();
    addResponseToMockRequester(HTTP_STATUS_OK, ResponseErrorCode::OK);
    addDiskSpaceExpectsToMockFileSystem();
    addListFilesExpectsToMockFileSystem(decompress, true);

    EXPECT_CALL(*m_mockFileSystem, exists(m_raExtractTmpDir)).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, makedirs(m_raExtractTmpDir)).Times(1);
    EXPECT_CALL(*m_mockFileSystem, exists(m_destPath)).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, exists(m_destPath + m_testExtractedFile)).Times(2).WillRepeatedly(Return(false));
    EXPECT_CALL(*m_mockFileSystem, exists(m_destPath + m_testExtractedFile + "1")).Times(2).WillRepeatedly(Return(false));
    EXPECT_CALL(*m_mockFileSystem, exists(m_destPath + m_testExtractedFile + "2")).Times(2).WillRepeatedly(Return(false));
    EXPECT_CALL(*m_mockFileSystem, exists(m_destPath + m_testExtractedFile + "3")).Times(3).WillRepeatedly(Return(false));
    EXPECT_CALL(*m_mockFileSystem, makedirs(m_destPath)).Times(1);
    addCleanupChecksToMockFileSystem();

    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, moveFileTryCopy(_, _)).WillOnce(Throw(Common::FileSystem::IFileSystemException(expectedErrStr)));
    EXPECT_CALL(*m_mockFileSystem, moveFileTryCopy(_, _)).Times(3).RetiresOnSaturation();
    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, m_raTmpFile))
        .WillOnce(Return("shastring"));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);

    nlohmann::json action = getDownloadObject(decompress, "", 1024, false);
    nlohmann::json response = downloadFileAction.run(action.dump());

    EXPECT_EQ(response["result"], ResponseResult::ERROR);
    EXPECT_EQ(response["errorMessage"], expectedErrStr);
    EXPECT_EQ(response["errorType"], "access_denied");
    EXPECT_EQ(response["httpStatus"], HTTP_STATUS_OK);

    EXPECT_TRUE(appenderContains("Beginning download to /path/to/download/to/"));
    EXPECT_TRUE(appenderContains("Downloaded file: /opt/sophos-spl/plugins/responseactions/tmp/tmp_download.zip"));
    EXPECT_TRUE(appenderContains("Extracted 4 files from archive"));
    EXPECT_TRUE(appenderContains("/path/to/download/to/download.txt downloaded successfully"));
    EXPECT_TRUE(appenderContains("/path/to/download/to/download.txt1 downloaded successfully"));
    EXPECT_TRUE(appenderContains("/path/to/download/to/download.txt2 downloaded successfully"));
    EXPECT_TRUE(appenderContains("Removing file /path/to/download/to/download.txt as move file failed"));
    EXPECT_TRUE(appenderContains("Removing file /path/to/download/to/download.txt1 as move file failed"));
    EXPECT_TRUE(appenderContains("Removing file /path/to/download/to/download.txt2 as move file failed"));
    EXPECT_TRUE(appenderContains("Removing file /path/to/download/to/download.txt3 as move file failed"));
    EXPECT_TRUE(appenderContains(expectedErrStr));
}

TEST_F(DownloadFileTests, HandlesWhenCantCopyFileToFinalDestination_Exception)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    const std::string expectedErrStr = "Unknown error when moving file " + m_raTmpFile + " to " + m_destPath + m_testZipFile + ": std::exception";

    addResponseToMockRequester(HTTP_STATUS_OK, ResponseErrorCode::OK);

    EXPECT_CALL(*m_mockFileSystem, exists(m_destPath + m_testZipFile)).WillOnce(Return(false));
    addDiskSpaceExpectsToMockFileSystem();
    addListFilesExpectsToMockFileSystem();
    addDownloadAndExtractExpectsToMockFileSystem(m_destPath + m_testZipFile);
    addCleanupChecksToMockFileSystem();

    EXPECT_CALL(*m_mockFileSystem, moveFileTryCopy(_,_)).WillOnce(Throw(std::exception()));
    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, m_raTmpFile))
        .WillOnce(Return("shastring"));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);

    nlohmann::json action = getDownloadObject();
    nlohmann::json response = downloadFileAction.run(action.dump());


    EXPECT_EQ(response["result"], ResponseResult::ERROR);
    EXPECT_EQ(response["errorMessage"], expectedErrStr);
    EXPECT_FALSE(response.contains("errorType"));
    EXPECT_EQ(response["httpStatus"], HTTP_STATUS_OK);

    EXPECT_TRUE(appenderContains(expectedErrStr));
}

//Unexpected downloads

TEST_F(DownloadFileTests, SuccessfulDownload_Direct_Decompress_NoFilesInDownloadFolder)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    const std::string expectedErrMsg = "Expected one file in " + m_raTmpDir + " but there are none";
    bool decompress = true;

    addResponseToMockRequester(HTTP_STATUS_OK, ResponseErrorCode::OK);


    addDiskSpaceExpectsToMockFileSystem();
    addCleanupChecksToMockFileSystem();

    EXPECT_CALL(*m_mockFileSystem, listFiles(m_raTmpDir)).WillOnce(Return(std::vector<std::string> {}));
    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);

    nlohmann::json action = getDownloadObject(decompress);
    nlohmann::json response = downloadFileAction.run(action.dump());


    EXPECT_EQ(response["result"], ResponseResult::ERROR);
    EXPECT_EQ(response["httpStatus"], HTTP_STATUS_OK);
    EXPECT_FALSE(response.contains("errorType"));
    EXPECT_EQ(response["errorMessage"], expectedErrMsg);

    EXPECT_TRUE(appenderContains("Beginning download to /path/to/download/to/download.txt"));
    EXPECT_TRUE(appenderContains(expectedErrMsg));
}

TEST_F(DownloadFileTests, SuccessfulDownload_Direct_Decompress_ToManyFilesInDownloadFolder)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    const std::string expectedErrMsg = "Expected one file in " + m_raTmpDir + " but there are 2";
    bool decompress = true;

    addResponseToMockRequester(HTTP_STATUS_OK, ResponseErrorCode::OK);


    addDiskSpaceExpectsToMockFileSystem();
    addCleanupChecksToMockFileSystem();

    EXPECT_CALL(*m_mockFileSystem, listFiles(m_raTmpDir)).WillOnce(Return(std::vector<std::string> {"one", "two"}));
    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));

    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);

    nlohmann::json action = getDownloadObject(decompress);
    nlohmann::json response = downloadFileAction.run(action.dump());
    

    EXPECT_EQ(response["result"], ResponseResult::ERROR);
    EXPECT_EQ(response["httpStatus"], HTTP_STATUS_OK);
    EXPECT_FALSE(response.contains("errorType"));
    EXPECT_EQ(response["errorMessage"], expectedErrMsg);

    EXPECT_TRUE(appenderContains("Beginning download to /path/to/download/to/download.txt"));
    EXPECT_TRUE(appenderContains(expectedErrMsg));
}

TEST_F(DownloadFileTests, SuccessfulDownload_Direct_Decompress_NoFilesUnzipped)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    const std::string expectedErrMsg = "Unzip successful but no file found in " + m_raExtractTmpDir;

    bool decompress = true;

    setupMockZipUtils();
    addResponseToMockRequester(HTTP_STATUS_OK, ResponseErrorCode::OK);

    // MockFileSystem
    addDiskSpaceExpectsToMockFileSystem();
    addListFilesExpectsToMockFileSystem();
    EXPECT_CALL(*m_mockFileSystem, exists(m_raExtractTmpDir)).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, makedirs(m_raExtractTmpDir)).Times(1);
    addCleanupChecksToMockFileSystem(decompress);

    EXPECT_CALL(*m_mockFileSystem, listAllFilesInDirectoryTree(m_raExtractTmpDir))
        .WillOnce(Return(std::vector<std::string>{}));
    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, m_raTmpFile))
        .WillOnce(Return("shastring"));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);

    nlohmann::json action = getDownloadObject(decompress);
    nlohmann::json response = downloadFileAction.run(action.dump());

    EXPECT_EQ(response["result"], ResponseResult::ERROR);
    EXPECT_EQ(response["httpStatus"], HTTP_STATUS_OK);
    EXPECT_FALSE(response.contains("errorType"));
    EXPECT_EQ(response["errorMessage"], expectedErrMsg);

    EXPECT_TRUE(appenderContains("Beginning download to /path/to/download/to/download.txt"));
    EXPECT_TRUE(appenderContains("Downloaded file: /opt/sophos-spl/plugins/responseactions/tmp/tmp_download.zip"));
    EXPECT_TRUE(appenderContains(expectedErrMsg));
}


//assessSpaceInfo
TEST_F(DownloadFileTests, DiskSpaceCheckReturningErrorCodeReturnsFalseOnRATmpDir)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    const std::string expectedErrStr = "Cant determine disk space on filesystem: exception";

    std::filesystem::space_info spaceInfo;
    spaceInfo.available = 1024 * 1024;
    std::error_code errCode = std::make_error_code(std::errc(ENODEV));

    EXPECT_CALL(*m_mockFileSystem, getDiskSpaceInfo(_, _))
        .WillOnce(testing::DoAll(testing::SetArgReferee<1>(errCode),Return(spaceInfo)));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);

    ResponseActionsImpl::DownloadInfo info;
    EXPECT_EQ(downloadFileAction.assessSpaceInfo(info), false);

    EXPECT_TRUE(appenderContains("Getting space info for " + m_raTmpDir));
    EXPECT_TRUE(appenderContains("Error calculating disk space for /opt/sophos-spl/plugins/responseactions/tmp: 19 message:No such device"));
}

TEST_F(DownloadFileTests, DiskSpaceCheckReturningErrorCodeReturnsFalseOnDestDir)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    const std::string expectedErrStr = "Cant determine disk space on filesystem: exception";

    std::filesystem::space_info spaceInfo;
    spaceInfo.available = 1024 * 1024;
    std::error_code errCode = std::make_error_code(std::errc(EBUSY));

    EXPECT_CALL(*m_mockFileSystem, getDiskSpaceInfo(_, _))
        .WillOnce(Return(spaceInfo))
        .WillOnce(testing::DoAll(testing::SetArgReferee<1>(errCode),Return(spaceInfo)));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);

    ResponseActionsImpl::DownloadInfo info;
    info.targetPath = "/das/destination";
    EXPECT_EQ(downloadFileAction.assessSpaceInfo(info), false);

    EXPECT_TRUE(appenderContains("Getting space info for " + m_raTmpDir));
    EXPECT_TRUE(appenderContains("Error calculating disk space for /das/destination: 16 message:Device or resource busy"));
}

TEST_F(DownloadFileTests, terminateSignal)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, _, _, _)).WillOnce(pollReturnsWithRevents(0, POLLIN));

    addResponseToMockRequester(26, ResponseErrorCode::OK);
    EXPECT_CALL(*m_mockHttpRequester, sendTerminate());
    addCleanupChecksToMockFileSystem();
    addDiskSpaceExpectsToMockFileSystem();
    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);
    nlohmann::json action = getDownloadObject();

    nlohmann::json response = downloadFileAction.run(action.dump());

    EXPECT_EQ(response["result"],1);
    EXPECT_EQ(response["httpStatus"], 26);

}

TEST_F(DownloadFileTests, timeoutSignal)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, _, _, _)).WillOnce(pollReturnsWithRevents(1, POLLIN));

    addResponseToMockRequester(26, ResponseErrorCode::OK);
    EXPECT_CALL(*m_mockHttpRequester, sendTerminate());
    addCleanupChecksToMockFileSystem();
    addDiskSpaceExpectsToMockFileSystem();
    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);
    nlohmann::json action = getDownloadObject();

    nlohmann::json response = downloadFileAction.run(action.dump());

    EXPECT_EQ(response["result"],ResponseResult::TIMEOUT);
    EXPECT_EQ(response["httpStatus"], 26);

}

TEST_F(DownloadFileTests, PpollErrorContinueWhenEINTR)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    EXPECT_CALL(*m_mockSysCallWrapper, ppoll(_, _, _, _))
            .WillOnce(SetErrnoAndReturn(EINTR, -1))
            .WillOnce(pollReturnsWithRevents(1, POLLIN));

    addResponseToMockRequester(26, ResponseErrorCode::OK);
    EXPECT_CALL(*m_mockHttpRequester, sendTerminate());

    addCleanupChecksToMockFileSystem();
    addDiskSpaceExpectsToMockFileSystem();
    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester,m_mockSignalHandler, m_mockSysCallWrapper);
    nlohmann::json action = getDownloadObject();

    nlohmann::json response = downloadFileAction.run(action.dump());

    EXPECT_EQ(response["result"],ResponseResult::TIMEOUT);
    EXPECT_EQ(response["httpStatus"], 26);

}