// Copyright 2023 Sophos Limited. All rights reserved.

#include "modules/ResponseActions/ResponseActionsImpl/DownloadFileAction.h"

#include "modules/Common/FileSystem/IFileTooLargeException.h"
#include "modules/Common/ZipUtilities/ZipUtils.h"

#include "tests/Common/Helpers/FileSystemReplaceAndRestore.h"
#include "tests/Common/Helpers/MemoryAppender.h"
#include "tests/Common/Helpers/MockFileSystem.h"
#include "tests/Common/Helpers/MockHttpRequester.h"
#include "tests/Common/Helpers/MockZipUtils.h"
#include "tests/Common/Helpers/TempDir.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <minizip/mz_compat.h>

#include <filesystem>

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

        EXPECT_CALL(*m_mockFileSystem, getDiskSpaceInfo("/path")).WillOnce(Return(tmpSpaceInfo));
        EXPECT_CALL(*m_mockFileSystem, getDiskSpaceInfo(m_raTmpDir)).WillOnce(Return(raSpaceInfo));
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

    void addListFilesExpectsToMockFileSystem(const bool decompress = false, const bool multipleFiles = false)
    {
        std::vector<std::string> zippedList { m_raTmpFile };
        EXPECT_CALL(*m_mockFileSystem, listFiles(m_raTmpDir)).WillOnce(Return(zippedList));

        if (decompress)
        {
            std::vector<std::string> unzippedList { m_raExtractTmpDir + "/" + m_testExtractedFile };
            if (multipleFiles)
            {
                for (int i = 1; i != 4; i++)
                {
                    auto tmpString = m_raExtractTmpDir + "/" + m_testExtractedFile + (std::to_string(i));
                    unzippedList.push_back(tmpString);
                }
            }

            EXPECT_CALL(*m_mockFileSystem, listAllFilesInDirectoryTree(m_raExtractTmpDir)).WillOnce(Return(unzippedList));
        }
    }

    void addDownloadAndExtractExpectsToMockFileSystem(const std::string& targetPath, const bool decompress = false)
    {
        std::string filename = m_testZipFile;

        if (decompress)
        {
            EXPECT_CALL(*m_mockFileSystem, exists(m_raExtractTmpDir)).WillOnce(Return(false));
            EXPECT_CALL(*m_mockFileSystem, makedirs(m_raExtractTmpDir)).Times(1);
        }
        
        EXPECT_CALL(*m_mockFileSystem, exists(m_destPath)).WillOnce(Return(false));
        EXPECT_CALL(*m_mockFileSystem, exists(targetPath)).WillOnce(Return(false));
        EXPECT_CALL(*m_mockFileSystem, makedirs(m_destPath)).Times(1);
    }

    void addCleanupChecksToMockFileSystem(const bool downloadExists = false, const bool extractExists = false)
    {
        EXPECT_CALL(*m_mockFileSystem, exists(m_raTmpFile)).WillOnce(Return(downloadExists));
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
};

TEST_F(DownloadFileTests, SuccessfulDownload_Direct_NotDecompressed)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    addResponseToMockRequester(HTTP_STATUS_OK, ResponseErrorCode::OK);

    //MockFileSystem
    addDiskSpaceExpectsToMockFileSystem();
    addListFilesExpectsToMockFileSystem();
    addDownloadAndExtractExpectsToMockFileSystem(m_destPath + m_testZipFile);
    addCleanupChecksToMockFileSystem();

    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, moveFileTryCopy(m_raTmpFile, m_destPath + m_testZipFile)).Times(1);
    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, m_raTmpFile))
        .WillOnce(Return("shastring"));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester);

    nlohmann::json action = getDownloadObject();
    nlohmann::json response = downloadFileAction.run(action.dump());
    

    EXPECT_EQ(response["type"], "sophos.mgt.response.DownloadFile");
    EXPECT_EQ(response["result"], 0);
    EXPECT_EQ(response["httpStatus"], HTTP_STATUS_OK);
    EXPECT_FALSE(response.contains("errorType"));
    EXPECT_FALSE(response.contains("errorMessage"));
    EXPECT_TRUE(response.contains("duration"));

    EXPECT_TRUE(appenderContains("Beginning download to /path/to/download/to/testdownload.zip"));
    EXPECT_TRUE(appenderContains("Downloading directly"));
    EXPECT_TRUE(appenderContains("/path/to/download/to/testdownload.zip downloaded successfully"));
}

TEST_F(DownloadFileTests, SuccessfulDownload_Direct_NotDecompressed_NoFileNameInTargetPath)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    addResponseToMockRequester(HTTP_STATUS_OK, ResponseErrorCode::OK);

    //MockFileSystem
    addDiskSpaceExpectsToMockFileSystem();
    addListFilesExpectsToMockFileSystem();
    addDownloadAndExtractExpectsToMockFileSystem(m_destPath + m_defaultZipName);
    addCleanupChecksToMockFileSystem();

    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, moveFileTryCopy(m_raTmpFile, m_destPath + m_defaultZipName)).Times(1);
    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, m_raTmpFile))
        .WillOnce(Return("shastring"));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester);

    nlohmann::json action = getDownloadObject(false, "", 1024, false);
    nlohmann::json response = downloadFileAction.run(action.dump());

    EXPECT_EQ(response["type"], "sophos.mgt.response.DownloadFile");
    EXPECT_EQ(response["result"], 0);
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

    //MockFileSystem
    addDiskSpaceExpectsToMockFileSystem();
    addListFilesExpectsToMockFileSystem();
    addDownloadAndExtractExpectsToMockFileSystem(m_destPath + m_testZipFile);
    addCleanupChecksToMockFileSystem(true, true);

    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, moveFileTryCopy(m_raTmpFile, m_destPath + m_testZipFile)).Times(1);
    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, m_raTmpFile))
        .WillOnce(Return("shastring"));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester);

    nlohmann::json action = getDownloadObject();
    nlohmann::json response = downloadFileAction.run(action.dump());
    
}

TEST_F(DownloadFileTests, SuccessfulDownload_Direct_Decompressed_WithFileNameInTargetPath)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    bool decompress = true;
    setupMockZipUtils();

    addResponseToMockRequester(HTTP_STATUS_OK, ResponseErrorCode::OK);

    //MockFileSystem
    addDiskSpaceExpectsToMockFileSystem();
    addListFilesExpectsToMockFileSystem(decompress);
    addDownloadAndExtractExpectsToMockFileSystem(m_destPath + m_testExtractedFile, decompress);
    addCleanupChecksToMockFileSystem();

    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, moveFileTryCopy(m_raExtractTmpDir + "/" + m_testExtractedFile, m_destPath + m_testExtractedFile)).Times(1);
    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, m_raTmpFile))
        .WillOnce(Return("shastring"));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester);

    nlohmann::json action = getDownloadObject(decompress);
    nlohmann::json response = downloadFileAction.run(action.dump());
    

    EXPECT_EQ(response["result"], 0);
    EXPECT_EQ(response["httpStatus"], HTTP_STATUS_OK);
    EXPECT_FALSE(response.contains("errorType"));
    EXPECT_FALSE(response.contains("errorMessage"));

    EXPECT_TRUE(appenderContains("Beginning download to /path/to/download/to/download.txt"));
    EXPECT_TRUE(appenderContains("Downloading directly"));
    EXPECT_TRUE(appenderContains("/path/to/download/to/download.txt downloaded successfully"));
}

TEST_F(DownloadFileTests, SuccessfulDownload_Direct_Decompressed_NoFileNameInTargetPath)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    bool decompress = true;
    setupMockZipUtils();
    const std::string fileName = "anotherdownload.txt";

    addResponseToMockRequester(HTTP_STATUS_OK, ResponseErrorCode::OK);

    //MockFileSystem
    addDiskSpaceExpectsToMockFileSystem();
    addListFilesExpectsToMockFileSystem(); //We want to specify the name of the unzipped file
    addDownloadAndExtractExpectsToMockFileSystem(m_destPath + fileName, decompress);
    addCleanupChecksToMockFileSystem();

    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, moveFileTryCopy(m_raExtractTmpDir + "/" + fileName, m_destPath + fileName)).Times(1);
    EXPECT_CALL(*m_mockFileSystem, listAllFilesInDirectoryTree(m_raExtractTmpDir))
        .WillOnce(Return(std::vector<std::string> {m_raExtractTmpDir + "/" + fileName}));

    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, m_raTmpFile))
        .WillOnce(Return("shastring"));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester);

    nlohmann::json action = getDownloadObject(decompress, "", 1024, false);
    nlohmann::json response = downloadFileAction.run(action.dump());


    EXPECT_EQ(response["result"], 0);
    EXPECT_EQ(response["httpStatus"], HTTP_STATUS_OK);
    EXPECT_FALSE(response.contains("errorType"));
    EXPECT_FALSE(response.contains("errorMessage"));

    EXPECT_TRUE(appenderContains("Beginning download to /path/to/download/to/"));
    EXPECT_TRUE(appenderContains("Downloading directly"));
    EXPECT_TRUE(appenderContains("/path/to/download/to/anotherdownload.txt downloaded successfully"));
}

TEST_F(DownloadFileTests, SuccessfulDownload_Direct_Decompressed_TmpFilesLeft)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    bool decompress = true;
    setupMockZipUtils();

    addResponseToMockRequester(HTTP_STATUS_OK, ResponseErrorCode::OK);

    //MockFileSystem
    addDiskSpaceExpectsToMockFileSystem();
    addListFilesExpectsToMockFileSystem(decompress);
    addDownloadAndExtractExpectsToMockFileSystem(m_destPath + m_testExtractedFile, decompress);
    addCleanupChecksToMockFileSystem();

    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, moveFileTryCopy(m_raExtractTmpDir + "/" + m_testExtractedFile, m_destPath + m_testExtractedFile)).Times(1);

    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, m_raTmpFile))
        .WillOnce(Return("shastring"));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester);

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

     //MockFileSystem
    addListFilesExpectsToMockFileSystem(decompress);

    //Download and extraction calls
    EXPECT_CALL(*m_mockFileSystem, exists(m_raExtractTmpDir)).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, makedirs(m_raExtractTmpDir)).Times(1);
    EXPECT_CALL(*m_mockFileSystem, exists("/download.txt")).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, exists("/")).WillOnce(Return(true));
    addCleanupChecksToMockFileSystem();

    EXPECT_CALL(*m_mockFileSystem, getDiskSpaceInfo(destPath)).WillOnce(Return(tmpSpaceInfo));
    EXPECT_CALL(*m_mockFileSystem, getDiskSpaceInfo(m_raTmpDir)).WillOnce(Return(tmpSpaceInfo));
    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, moveFileTryCopy(m_raExtractTmpDir + "/" + m_testExtractedFile, destPath)).Times(1);
    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, m_raTmpFile))
        .WillOnce(Return("shastring"));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester);

    nlohmann::json action = getDownloadObject(decompress);
    action["targetPath"] = destPath;
    nlohmann::json response = downloadFileAction.run(action.dump());
    

    EXPECT_EQ(response["result"], 0);
    EXPECT_EQ(response["httpStatus"], HTTP_STATUS_OK);
    EXPECT_FALSE(response.contains("errorType"));
    EXPECT_FALSE(response.contains("errorMessage"));

    EXPECT_TRUE(appenderContains("Beginning download to /download.txt"));
    EXPECT_TRUE(appenderContains("Downloading directly"));
    EXPECT_TRUE(appenderContains("/download.txt downloaded successfully"));
}


TEST_F(DownloadFileTests, SuccessfulDownload_Direct_Decompress_ManyFilesTargetPathIsDirectory)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    bool decompress = true;

    setupMockZipUtils();
    addResponseToMockRequester(HTTP_STATUS_OK, ResponseErrorCode::OK);

    //MockFileSystem
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

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester);

    nlohmann::json action = getDownloadObject(decompress, "", 1024, false);
    nlohmann::json response = downloadFileAction.run(action.dump());

    EXPECT_EQ(response["result"], 0);
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

TEST_F(DownloadFileTests, SuccessfulDownload_Direct_Decompress_ManyFilesTargetPathIsFile)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    std::string expectedMsg = "Ignoring filepath in targetPath field as the archive contains multiple files";
    bool decompress = true;

    setupMockZipUtils();
    addResponseToMockRequester(HTTP_STATUS_OK, ResponseErrorCode::OK);

    //MockFileSystem
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

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester);

    nlohmann::json action = getDownloadObject(decompress);
    nlohmann::json response = downloadFileAction.run(action.dump());

    EXPECT_EQ(response["result"], 0);
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

TEST_F(DownloadFileTests, SuccessfulDownload_Direct_Decompress_AbortCopyingIfOneFileExists)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    std::string expectedMsg = "Path /path/to/download/to/download.txt1 already exists";

    bool decompress = true;

    setupMockZipUtils();
    addResponseToMockRequester(HTTP_STATUS_OK, ResponseErrorCode::OK);

    //MockFileSystem
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

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester);

    nlohmann::json action = getDownloadObject(decompress, "", 1024, false);
    nlohmann::json response = downloadFileAction.run(action.dump());

    EXPECT_EQ(response["result"], 1);
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

TEST_F(DownloadFileTests, SuccessfulDownload_Direct_Decompressed_Password)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    bool decompress = true;
    setupMockZipUtils(0, m_password);

    addResponseToMockRequester(HTTP_STATUS_OK, ResponseErrorCode::OK);

    //MockFileSystem
    addDiskSpaceExpectsToMockFileSystem();
    addListFilesExpectsToMockFileSystem(decompress);
    addDownloadAndExtractExpectsToMockFileSystem(m_destPath + m_testExtractedFile, true);
    addCleanupChecksToMockFileSystem();

    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, moveFileTryCopy(m_raExtractTmpDir + "/" + m_testExtractedFile, m_destPath + m_testExtractedFile)).Times(1);
    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, m_raTmpFile))
        .WillOnce(Return("shastring"));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester);

    nlohmann::json action = getDownloadObject(decompress, m_password);
    nlohmann::json response = downloadFileAction.run(action.dump());
    

    EXPECT_EQ(response["result"], 0);
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

    //MockFileSystem
    addDiskSpaceExpectsToMockFileSystem();
    addListFilesExpectsToMockFileSystem();

    EXPECT_CALL(*m_mockFileSystem, exists(m_raExtractTmpDir)).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, makedirs(m_raExtractTmpDir)).Times(1);
    addCleanupChecksToMockFileSystem();

    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, m_raTmpFile))
        .WillOnce(Return("shastring"));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester);

    nlohmann::json action = getDownloadObject(true, m_password);
    nlohmann::json response = downloadFileAction.run(action.dump());
    

    EXPECT_EQ(response["result"], 1);
    EXPECT_EQ(response["httpStatus"], HTTP_STATUS_OK);
    EXPECT_EQ(response["errorType"], "invalid_password");
    EXPECT_EQ(response["errorMessage"], expectedErrMsg);

    EXPECT_TRUE(appenderContains("Beginning download to /path/to/download/to/download.txt"));
    EXPECT_TRUE(appenderContains("Downloaded file: /opt/sophos-spl/plugins/responseactions/tmp/tmp_download.zip"));
    EXPECT_TRUE(appenderContains(expectedErrMsg));
}

TEST_F(DownloadFileTests, SuccessfulDownload_Direct_Decompress_BadArchive)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    const std::string expectedErrMsg = "Error unzipping /opt/sophos-spl/plugins/responseactions/tmp/tmp_download.zip due to bad archive";

    bool decompress = true;
    setupMockZipUtils(UNZ_BADZIPFILE);

    addResponseToMockRequester(HTTP_STATUS_OK, ResponseErrorCode::OK);

    //MockFileSystem
    addDiskSpaceExpectsToMockFileSystem();
    addListFilesExpectsToMockFileSystem();

    EXPECT_CALL(*m_mockFileSystem, exists(m_raExtractTmpDir)).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, makedirs(m_raExtractTmpDir)).Times(1);
    addCleanupChecksToMockFileSystem();

    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, m_raTmpFile))
        .WillOnce(Return("shastring"));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester);

    nlohmann::json action = getDownloadObject(decompress);
    nlohmann::json response = downloadFileAction.run(action.dump());
    

    EXPECT_EQ(response["result"], 1);
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

    //MockFileSystem
    addDiskSpaceExpectsToMockFileSystem();
    addListFilesExpectsToMockFileSystem();

    EXPECT_CALL(*m_mockFileSystem, exists(m_raExtractTmpDir)).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, makedirs(m_raExtractTmpDir)).Times(1);
    addCleanupChecksToMockFileSystem();

    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, m_raTmpFile))
        .WillOnce(Return("shastring"));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester);

    nlohmann::json action = getDownloadObject(decompress);
    nlohmann::json response = downloadFileAction.run(action.dump());
    

    EXPECT_EQ(response["result"], 3);
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

    //MockFileSystem
    addDiskSpaceExpectsToMockFileSystem();
    addListFilesExpectsToMockFileSystem();

    EXPECT_CALL(*m_mockFileSystem, exists(m_raExtractTmpDir)).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, makedirs(m_raExtractTmpDir)).Times(1);
    addCleanupChecksToMockFileSystem();

    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, m_raTmpFile))
        .WillOnce(Return("shastring"));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester);

    nlohmann::json action = getDownloadObject(decompress);
    nlohmann::json response = downloadFileAction.run(action.dump());
    

    EXPECT_EQ(response["result"], 3);
    EXPECT_EQ(response["httpStatus"], HTTP_STATUS_OK);
    EXPECT_FALSE(response.contains("errorType"));
    EXPECT_EQ(response["errorMessage"], expectedErrMsg);

    EXPECT_TRUE(appenderContains("Beginning download to /path/to/download/to/download.txt"));
    EXPECT_TRUE(appenderContains("Downloaded file: /opt/sophos-spl/plugins/responseactions/tmp/tmp_download.zip"));
    EXPECT_TRUE(appenderContains(expectedErrMsg));
}


TEST_F(DownloadFileTests, SuccessfulDownload_WithProxy_NotDecompressed)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    addResponseToMockRequester(HTTP_STATUS_OK, ResponseErrorCode::OK);

    std::string obfuscatedCreds = "CCD4E57ZjW+t5XPiMSJH1TurG3MfWCN3DpjJRINMwqNaWl+3zzlVIdyVmifCHUwcmaX6+YTSyyBM8SslIIGV5rUw";
    std::string content = R"({"proxy":"localhost","credentials":")" + obfuscatedCreds + R"("})";

    //MockFileSystem
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

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester);

    nlohmann::json action = getDownloadObject();
    nlohmann::json response = downloadFileAction.run(action.dump());
    

    EXPECT_EQ(response["result"], 0);
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

    //MockFileSystem
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

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester);

    nlohmann::json action = getDownloadObject(decompress);
    nlohmann::json response = downloadFileAction.run(action.dump());
    

    EXPECT_EQ(response["result"], 0);
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

    //MockFileSystem
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

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester);

    nlohmann::json action = getDownloadObject();
    nlohmann::json response = downloadFileAction.run(action.dump());
    

    EXPECT_EQ(response["result"], 0);
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

    //MockFileSystem
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

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester);

    nlohmann::json action = getDownloadObject(decompress);
    nlohmann::json response = downloadFileAction.run(action.dump());
    

    EXPECT_EQ(response["result"], 0);
    EXPECT_EQ(response["httpStatus"], HTTP_STATUS_OK);
    EXPECT_FALSE(response.contains("errorType"));
    EXPECT_FALSE(response.contains("errorMessage"));

    EXPECT_TRUE(appenderContains("Downloading via proxy: localhost"));
    EXPECT_TRUE(appenderContains("Failed to download, Error code: 400"));
    EXPECT_TRUE(appenderContains("Connection with proxy failed, going direct"));
    EXPECT_TRUE(appenderContains("/path/to/download/to/download.txt downloaded successfully"));
}

//Not sure if this behaviour is prevented in Central so we handle it here to
TEST_F(DownloadFileTests, FileToDownloadIsAboveMaxAllowedFileSize)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    std::string expectedErrMsg = "Downloading file to /path/to/download/to/testdownload.zip failed due to size: 3221225472 is too large";

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester);

    nlohmann::json action = getDownloadObject(false, "", 1024UL * 1024 * 1024 * 3);
    nlohmann::json response = downloadFileAction.run(action.dump());
    

    EXPECT_EQ(response["result"], 1);
    EXPECT_FALSE(response.contains("errorType"));
    EXPECT_FALSE(response.contains("httpStatus"));
    EXPECT_EQ(response["errorMessage"], expectedErrMsg);

    EXPECT_TRUE(appenderContains(expectedErrMsg));
}


TEST_F(DownloadFileTests, TargetPathAlreadyExists_NotDecompressed)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    std::string expectedErrMsg = "Path /path/to/download/to/testdownload.zip already exists";

    addResponseToMockRequester(HTTP_STATUS_OK, ResponseErrorCode::OK);

    //MockFileSystem
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

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester);

    nlohmann::json action = getDownloadObject();
    nlohmann::json response = downloadFileAction.run(action.dump());

    EXPECT_EQ(response["result"], 1);
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

    //MockFileSystem
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

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester);

    nlohmann::json action = getDownloadObject(true);
    nlohmann::json response = downloadFileAction.run(action.dump());


    EXPECT_EQ(response["result"], 1);
    EXPECT_EQ(response["errorMessage"], expectedErrMsg);
    EXPECT_EQ(response["errorType"], "path_exists");

    EXPECT_TRUE(appenderContains(expectedErrMsg));
}

TEST_F(DownloadFileTests, TargetPathAlreadyExists_NotDecompressedAlreadyExists)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    std::string expectedErrMsg = "Path /path/to/download/to/testdownload.zip already exists";

    addResponseToMockRequester(HTTP_STATUS_OK, ResponseErrorCode::OK);

    //MockFileSystem
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

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester);

    nlohmann::json action = getDownloadObject();
    nlohmann::json response = downloadFileAction.run(action.dump());


    EXPECT_EQ(response["result"], 1);
    EXPECT_EQ(response["errorMessage"], expectedErrMsg);
    EXPECT_EQ(response["errorType"], "path_exists");

    EXPECT_TRUE(appenderContains(expectedErrMsg));
}

TEST_F(DownloadFileTests, InvalidAbsolutePath)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    const std::string expectedErrMsg = "notapath is not a valid absolute path";

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester);
    nlohmann::json action = getDownloadObject();
    action["targetPath"] = "notapath";
    nlohmann::json response = downloadFileAction.run(action.dump());
    

    EXPECT_EQ(response["result"], 1);
    EXPECT_EQ(response["errorMessage"], expectedErrMsg);
    EXPECT_EQ(response["errorType"], "invalid_path");

    EXPECT_TRUE(appenderContains(expectedErrMsg));
}

//More thoroughly checked in actionutils
TEST_F(DownloadFileTests, ErrorParsingJson)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto httpRequester = std::make_shared<StrictMock<MockHTTPRequester>>();
    ResponseActionsImpl::DownloadFileAction downloadFileAction(httpRequester);

    nlohmann::json response = downloadFileAction.run("");
    
    EXPECT_EQ(response["result"], 1);
    EXPECT_EQ(response["errorMessage"], "Error parsing command from Central");

    EXPECT_TRUE(appenderContains("json.exception.parse_error.101"));
}

TEST_F(DownloadFileTests, actionExpired)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto httpRequester = std::make_shared<StrictMock<MockHTTPRequester>>();
    ResponseActionsImpl::DownloadFileAction downloadFileAction(httpRequester);
    nlohmann::json action = getDownloadObject();
    action["expiration"] = 0;
    nlohmann::json response = downloadFileAction.run(action.dump());
    
    EXPECT_EQ(response["result"],4);
    EXPECT_EQ(response["errorMessage"],"Download file action has expired");

    EXPECT_TRUE(appenderContains("Download file action has expired"));
}

TEST_F(DownloadFileTests, FileSha256CantBeCalculatedDueToAccess)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    const std::string expectedErrStr = "testdownload.zip cannot be accessed";

    addResponseToMockRequester(HTTP_STATUS_OK, ResponseErrorCode::OK);

     //MockFileSystem
    addDiskSpaceExpectsToMockFileSystem();
    addListFilesExpectsToMockFileSystem();
    addCleanupChecksToMockFileSystem();
    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, m_raTmpFile))
        .WillOnce(Throw(Common::FileSystem::IFileSystemException("")));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester);
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

    // MockFileSystem
    addDiskSpaceExpectsToMockFileSystem();
    addListFilesExpectsToMockFileSystem();
    addCleanupChecksToMockFileSystem();
    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, m_raTmpFile))
        .WillOnce(Throw(std::exception()));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester);
    nlohmann::json action = getDownloadObject();

    nlohmann::json response = downloadFileAction.run(action.dump());
    
    EXPECT_EQ(response["result"], 1);
    EXPECT_EQ(response["httpStatus"], HTTP_STATUS_OK);
    EXPECT_FALSE(response.contains("errorType"));
    EXPECT_EQ(response["errorMessage"], expectedErrStr);

    EXPECT_TRUE(appenderContains(expectedErrStr));
}

TEST_F(DownloadFileTests, FailureDueToTimeout)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    const std::string expectedErrStr = "Download timed out";

    addResponseToMockRequester(500, ResponseErrorCode::TIMEOUT);

    //MockFileSystem
    addDiskSpaceExpectsToMockFileSystem();
    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester);
    nlohmann::json action = getDownloadObject();

    nlohmann::json response = downloadFileAction.run(action.dump());
    
    EXPECT_EQ(response["result"],2);
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

    //MockFileSystem
    addDiskSpaceExpectsToMockFileSystem();
    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester);
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

    //MockFileSystem
    addDiskSpaceExpectsToMockFileSystem();
    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester);
    nlohmann::json action = getDownloadObject();

    nlohmann::json response = downloadFileAction.run(action.dump());
    
    EXPECT_EQ(response["result"],1);
    EXPECT_EQ(response["httpStatus"], 500);
    EXPECT_EQ(response["errorType"], "network_error");
    EXPECT_EQ(response["errorMessage"], expectedErrStr);

    EXPECT_TRUE(appenderContains(expectedErrStr));
}

TEST_F(DownloadFileTests, NotEnoughSpaceOnRATmpDisk)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    const std::string expectedErrStr = "Not enough space to complete download action: Sophos install disk has 1, destination disk has 1048576";

    //MockFileSystem
    addDiskSpaceExpectsToMockFileSystem(1);
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester);

    nlohmann::json action = getDownloadObject();
    nlohmann::json response = downloadFileAction.run(action.dump());
    

    EXPECT_EQ(response["result"], 1);
    EXPECT_EQ(response["errorMessage"], expectedErrStr);
    EXPECT_EQ(response["errorType"], "not_enough_space");

    EXPECT_TRUE(appenderContains(expectedErrStr));
}

TEST_F(DownloadFileTests, NotEnoughSpaceOnDestDisk)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    const std::string expectedErrStr = "Not enough space to complete download action: Sophos install disk has 1048576, destination disk has 1";

    //MockFileSystem
    addDiskSpaceExpectsToMockFileSystem(1024 * 1024, 1);
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester);

    nlohmann::json action = getDownloadObject();
    nlohmann::json response = downloadFileAction.run(action.dump());
    

    EXPECT_EQ(response["result"], 1);
    EXPECT_EQ(response["errorMessage"], expectedErrStr);
    EXPECT_EQ(response["errorType"], "not_enough_space");

    EXPECT_TRUE(appenderContains(expectedErrStr));
}

TEST_F(DownloadFileTests, HandlesWhenCantAssessDiskSpace_FileSystemException)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    const std::string expectedErrStr = "Cant determine disk space on filesystem: exception";

    //MockFileSystem
    EXPECT_CALL(*m_mockFileSystem, getDiskSpaceInfo("/opt/sophos-spl/plugins/responseactions/tmp"))
        .WillOnce(Throw(Common::FileSystem::IFileSystemException("exception")));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester);

    nlohmann::json action = getDownloadObject();
    nlohmann::json response = downloadFileAction.run(action.dump());
    

    EXPECT_EQ(response["result"], 1);
    EXPECT_EQ(response["errorMessage"], expectedErrStr);
    EXPECT_FALSE(response.contains("errorType"));

    EXPECT_TRUE(appenderContains(expectedErrStr));
}

TEST_F(DownloadFileTests, HandlesWhenCantAssessDiskSpace_Exception)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    const std::string expectedErrStr = "Unknown exception when calculating disk space on filesystem: std::exception";

    //MockFileSystem
    EXPECT_CALL(*m_mockFileSystem, getDiskSpaceInfo("/opt/sophos-spl/plugins/responseactions/tmp"))
        .WillOnce(Throw(std::exception()));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester);

    nlohmann::json action = getDownloadObject();
    nlohmann::json response = downloadFileAction.run(action.dump());
    

    EXPECT_EQ(response["result"], 1);
    EXPECT_EQ(response["errorMessage"], expectedErrStr);
    EXPECT_FALSE(response.contains("errorType"));

    EXPECT_TRUE(appenderContains(expectedErrStr));
}


TEST_F(DownloadFileTests, HandlesWhenCantCreatePathToExtractTo_FileSystemException)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    const bool decompress = true;

    const std::string expectedErrStr = "Unable to create path to extract file to: " + m_raExtractTmpDir + ": exception";

    addResponseToMockRequester(HTTP_STATUS_OK, ResponseErrorCode::OK);

    //MockFileSystem
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

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester);

    nlohmann::json action = getDownloadObject(decompress);
    nlohmann::json response = downloadFileAction.run(action.dump());
    

    EXPECT_EQ(response["result"], 1);
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

    //MockFileSystem
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

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester);

    nlohmann::json action = getDownloadObject(decompress);
    nlohmann::json response = downloadFileAction.run(action.dump());
    

    EXPECT_EQ(response["result"], 1);
    EXPECT_EQ(response["errorMessage"], expectedErrStr);
    EXPECT_FALSE(response.contains("errorType"));
    EXPECT_EQ(response["httpStatus"], HTTP_STATUS_OK);

    EXPECT_TRUE(appenderContains(expectedErrStr));
}

TEST_F(DownloadFileTests, HandlesWhenCantCreateOrCopyFileToFinalDestination_FileSystemException)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    const std::string expectedErrStr = "Unable to make directory " + m_destPath + " : exception";

    addResponseToMockRequester(HTTP_STATUS_OK, ResponseErrorCode::OK);

    //MockFileSystem
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

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester);

    nlohmann::json action = getDownloadObject();
    nlohmann::json response = downloadFileAction.run(action.dump());
    

    EXPECT_EQ(response["result"], 1);
    EXPECT_EQ(response["errorMessage"], expectedErrStr);
    EXPECT_EQ(response["errorType"], "access_denied");
    EXPECT_EQ(response["httpStatus"], HTTP_STATUS_OK);

    EXPECT_TRUE(appenderContains(expectedErrStr));
}

TEST_F(DownloadFileTests, HandlesWhenCantCreateOrCopyFileToFinalDestination_Exception)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    const std::string expectedErrStr = "Unknown error when making directory " + m_destPath + " : std::exception";

    addResponseToMockRequester(HTTP_STATUS_OK, ResponseErrorCode::OK);

    //MockFileSystem
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

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester);

    nlohmann::json action = getDownloadObject();
    nlohmann::json response = downloadFileAction.run(action.dump());
    

    EXPECT_EQ(response["result"], 1);
    EXPECT_EQ(response["errorMessage"], expectedErrStr);
    EXPECT_FALSE(response.contains("errorType"));
    EXPECT_EQ(response["httpStatus"], HTTP_STATUS_OK);

    EXPECT_TRUE(appenderContains(expectedErrStr));
}

TEST_F(DownloadFileTests, SuccessfulDownload_Direct_Decompress_NoFilesInDownloadFolder)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    const std::string expectedErrMsg = "Expected one file in " + m_raTmpDir + " but there are none";
    bool decompress = true;

    addResponseToMockRequester(HTTP_STATUS_OK, ResponseErrorCode::OK);

    //MockFileSystem
    addDiskSpaceExpectsToMockFileSystem();
    addCleanupChecksToMockFileSystem();

    EXPECT_CALL(*m_mockFileSystem, listFiles(m_raTmpDir)).WillOnce(Return(std::vector<std::string> {}));
    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester);

    nlohmann::json action = getDownloadObject(decompress);
    nlohmann::json response = downloadFileAction.run(action.dump());
    

    EXPECT_EQ(response["result"], 1);
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

    //MockFileSystem
    addDiskSpaceExpectsToMockFileSystem();
    addCleanupChecksToMockFileSystem();

    EXPECT_CALL(*m_mockFileSystem, listFiles(m_raTmpDir)).WillOnce(Return(std::vector<std::string> {"one", "two"}));
    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));

    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester);

    nlohmann::json action = getDownloadObject(decompress);
    nlohmann::json response = downloadFileAction.run(action.dump());
    

    EXPECT_EQ(response["result"], 1);
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

    //MockFileSystem
    addDiskSpaceExpectsToMockFileSystem();
    addListFilesExpectsToMockFileSystem();
    EXPECT_CALL(*m_mockFileSystem, exists(m_raExtractTmpDir)).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, makedirs(m_raExtractTmpDir)).Times(1);
    addCleanupChecksToMockFileSystem(decompress);

    EXPECT_CALL(*m_mockFileSystem, listAllFilesInDirectoryTree(m_raExtractTmpDir)).WillOnce(Return(std::vector<std::string> {}));
    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, m_raTmpFile))
        .WillOnce(Return("shastring"));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester);

    nlohmann::json action = getDownloadObject(decompress);
    nlohmann::json response = downloadFileAction.run(action.dump());
    

    EXPECT_EQ(response["result"], 1);
    EXPECT_EQ(response["httpStatus"], HTTP_STATUS_OK);
    EXPECT_FALSE(response.contains("errorType"));
    EXPECT_EQ(response["errorMessage"], expectedErrMsg);

    EXPECT_TRUE(appenderContains("Beginning download to /path/to/download/to/download.txt"));
    EXPECT_TRUE(appenderContains("Downloaded file: /opt/sophos-spl/plugins/responseactions/tmp/tmp_download.zip"));
    EXPECT_TRUE(appenderContains(expectedErrMsg));
}

TEST_F(DownloadFileTests, Sha256IsWrong)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    const std::string expectedErrStr = "Calculated Sha256 (adifferentshastring) doesnt match that of file downloaded (shastring)";

    addResponseToMockRequester(HTTP_STATUS_OK, ResponseErrorCode::OK);

    //MockFileSystem
    addDiskSpaceExpectsToMockFileSystem();
    addListFilesExpectsToMockFileSystem();
    addCleanupChecksToMockFileSystem();
    EXPECT_CALL(*m_mockFileSystem, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillOnce(Return(false));
    EXPECT_CALL(*m_mockFileSystem, calculateDigest(Common::SslImpl::Digest::sha256, m_raTmpFile))
        .WillOnce(Return("adifferentshastring"));
    Tests::replaceFileSystem(std::move(m_mockFileSystem));

    ResponseActionsImpl::DownloadFileAction downloadFileAction(m_mockHttpRequester);

    nlohmann::json action = getDownloadObject();
    nlohmann::json response = downloadFileAction.run(action.dump());
    

    EXPECT_EQ(response["result"], 1);
    EXPECT_EQ(response["errorMessage"], expectedErrStr);
    EXPECT_EQ(response["errorType"], "access_denied");

    EXPECT_TRUE(appenderContains(expectedErrStr));
}
