// Copyright 2022-2023 Sophos Limited. All rights reserved.

#define AUTO_FD_IMPLICIT_INT

#include "MockISafeStoreWrapper.h"
#include "MockSafeStoreResources.h"
#include "SafeStoreMemoryAppenderUsingTests.h"

#include "common/ApplicationPaths.h"
#include "safestore/QuarantineManager/IQuarantineManager.h"
#include "safestore/QuarantineManager/QuarantineManagerImpl.h"
#include "safestore/SafeStoreWrapper/ISafeStoreWrapper.h"
#include "scan_messages/QuarantineResponse.h"

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
#include "Common/FileSystem/IFileSystem.h"
#include "Common/FileSystem/IFileSystemException.h"
#include "Common/Helpers/FileSystemReplaceAndRestore.h"
#include "Common/Helpers/MockFilePermissions.h"
#include "Common/Helpers/MockFileSystem.h"
#include "Common/Helpers/MockSysCalls.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;
using namespace safestore::QuarantineManager;

class QuarantineManagerTests : public SafeStoreMemoryAppenderUsingTests
{
protected:
    void SetUp() override
    {
        auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
        appConfig.setData("SOPHOS_INSTALL", "/tmp");
        appConfig.setData("PLUGIN_INSTALL", "/tmp/av");
        m_mockSafeStoreWrapper = std::make_unique<StrictMock<MockISafeStoreWrapper>>();
    }

    void addCommonPersistValueExpects(StrictMock<MockFileSystem>& filesystemMock)
    {
        EXPECT_CALL(filesystemMock, exists("/tmp/av/var/persist-safeStoreDbErrorThreshold")).WillOnce(Return(false));
        EXPECT_CALL(filesystemMock, writeFile("/tmp/av/var/persist-safeStoreDbErrorThreshold", "10"));
    }

    std::unique_ptr<QuarantineManagerImpl> createQuarantineManager()
    {
        return std::make_unique<QuarantineManagerImpl>(
            std::move(m_mockSafeStoreWrapper), std::move(m_mockSysCallWrapper), mockSafeStoreResources_);
    }

    std::unique_ptr<StrictMock<MockISafeStoreWrapper>> m_mockSafeStoreWrapper;
    std::shared_ptr<StrictMock<MockSystemCallWrapper>> m_mockSysCallWrapper =
        std::make_shared<StrictMock<MockSystemCallWrapper>>();
    MockSafeStoreResources mockSafeStoreResources_;

    // Common test constants
    inline static const std::string m_dir = "/dir/";
    inline static const std::string m_file = "file";
    inline static const std::string m_threatID = "01234567-89ab-cdef-0123-456789abcdef";
    inline static const std::string m_threatType = "threatType";
    inline static const std::string m_threatName = "threatName";
    inline static const std::string threatSha256_ = "threatSHA256";
    inline static const std::string threats_ = R"([{"name":"threatName","sha256":"threatSHA256","type":"threatType"}])";
    inline static const std::string m_SHA256 = "SHA256abcdef";
    inline static const std::string m_correlationId = "fedcba98-7654-3210-fedc-ba9876543210";
    const std::string m_path = "/dir/file";
};

TEST_F(QuarantineManagerTests, initDbWithExistingPassword)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };

    addCommonPersistValueExpects(*filesystemMock);

    EXPECT_CALL(*filesystemMock, removeFile(Plugin::getSafeStoreDormantFlagPath(), true)).WillOnce(Return());
    EXPECT_CALL(*filesystemMock, isFile(Plugin::getSafeStorePasswordFilePath())).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(Plugin::getSafeStorePasswordFilePath())).WillOnce(Return("a password"));
    EXPECT_CALL(*filesystemMock, isFile(Plugin::getSafeStoreConfigPath())).WillOnce(Return(false));
    EXPECT_CALL(*filesystemMock, removeFilesInDirectory("/tmp/av/var/tempUnpack")).Times(1);

    EXPECT_CALL(
        *m_mockSafeStoreWrapper,
        initialise(Plugin::getSafeStoreDbDirPath(), Plugin::getSafeStoreDbFileName(), "a password"))
        .WillOnce(Return(InitReturnCode::OK));
    auto quarantineManager = createQuarantineManager();

    EXPECT_NO_THROW(quarantineManager->initialise());
}

TEST_F(QuarantineManagerTests, initDbAndGeneratePassword)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };
    addCommonPersistValueExpects(*filesystemMock);
    EXPECT_CALL(*filesystemMock, removeFile(Plugin::getSafeStoreDormantFlagPath(), true)).WillOnce(Return());
    EXPECT_CALL(*filesystemMock, isFile(Plugin::getSafeStorePasswordFilePath())).WillOnce(Return(false));
    EXPECT_CALL(*filesystemMock, readFile(Plugin::getSafeStorePasswordFilePath())).Times(0);
    EXPECT_CALL(*filesystemMock, isDirectory(Plugin::getSafeStoreDbDirPath())).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, writeFile(Plugin::getSafeStorePasswordFilePath(), _)).Times(1);
    EXPECT_CALL(*filesystemMock, isFile(Plugin::getSafeStoreConfigPath())).WillOnce(Return(false));
    EXPECT_CALL(*filesystemMock, removeFilesInDirectory("/tmp/av/var/tempUnpack")).Times(1);

    EXPECT_CALL(
        *m_mockSafeStoreWrapper, initialise(Plugin::getSafeStoreDbDirPath(), Plugin::getSafeStoreDbFileName(), _))
        .WillOnce(Return(InitReturnCode::OK));
    auto quarantineManager = createQuarantineManager();

    EXPECT_NO_THROW(quarantineManager->initialise());
}

TEST_F(QuarantineManagerTests, uninitialisedDbIsInitialisedAfterSuccessfulInitCall)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };
    addCommonPersistValueExpects(*filesystemMock);
    EXPECT_CALL(*filesystemMock, removeFile(Plugin::getSafeStoreDormantFlagPath(), true)).WillOnce(Return());
    EXPECT_CALL(*filesystemMock, isFile(Plugin::getSafeStorePasswordFilePath())).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(Plugin::getSafeStorePasswordFilePath())).WillOnce(Return("a password"));
    EXPECT_CALL(*filesystemMock, isFile(Plugin::getSafeStoreConfigPath())).WillOnce(Return(false));
    EXPECT_CALL(*filesystemMock, removeFilesInDirectory("/tmp/av/var/tempUnpack")).Times(1);

    EXPECT_CALL(
        *m_mockSafeStoreWrapper,
        initialise(Plugin::getSafeStoreDbDirPath(), Plugin::getSafeStoreDbFileName(), "a password"))
        .WillOnce(Return(InitReturnCode::OK));
    auto quarantineManager = createQuarantineManager();

    ASSERT_EQ(quarantineManager->getState(), QuarantineManagerState::STARTUP);
    EXPECT_NO_THROW(quarantineManager->initialise());
    ASSERT_EQ(quarantineManager->getState(), QuarantineManagerState::INITIALISED);
}

TEST_F(QuarantineManagerTests, uninitialisedDbStateIsStillUnitialisedAfterFailedInitCall)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };
    addCommonPersistValueExpects(*filesystemMock);
    EXPECT_CALL(*filesystemMock, isFile(Plugin::getSafeStorePasswordFilePath())).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(Plugin::getSafeStorePasswordFilePath())).WillOnce(Return("a password"));
    EXPECT_CALL(
        *filesystemMock,
        writeFileAtomically(Plugin::getSafeStoreDormantFlagPath(), "SafeStore database uninitialised", "/tmp/tmp"));
    EXPECT_CALL(*filesystemMock, removeFilesInDirectory("/tmp/av/var/tempUnpack")).Times(1);

    EXPECT_CALL(
        *m_mockSafeStoreWrapper,
        initialise(Plugin::getSafeStoreDbDirPath(), Plugin::getSafeStoreDbFileName(), "a password"))
        .WillOnce(Return(InitReturnCode::DB_OPEN_FAILED));
    auto quarantineManager = createQuarantineManager();

    ASSERT_EQ(quarantineManager->getState(), QuarantineManagerState::STARTUP);
    EXPECT_NO_THROW(quarantineManager->initialise());
    ASSERT_EQ(quarantineManager->getState(), QuarantineManagerState::UNINITIALISED);
}

TEST_F(QuarantineManagerTests, initFailsOnDBErrorAndDbIsMarkedCorrupt)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };
    addCommonPersistValueExpects(*filesystemMock);
    EXPECT_CALL(*filesystemMock, isFile(Plugin::getSafeStorePasswordFilePath())).WillRepeatedly(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(Plugin::getSafeStorePasswordFilePath())).WillRepeatedly(Return("a password"));
    EXPECT_CALL(*filesystemMock, removeFilesInDirectory("/tmp/av/var/tempUnpack")).Times(10);

    EXPECT_CALL(
        *m_mockSafeStoreWrapper,
        initialise(Plugin::getSafeStoreDbDirPath(), Plugin::getSafeStoreDbFileName(), "a password"))
        .WillRepeatedly(Return(InitReturnCode::DB_ERROR));
    auto quarantineManager = createQuarantineManager();

    EXPECT_CALL(
        *filesystemMock,
        writeFileAtomically(Plugin::getSafeStoreDormantFlagPath(), "SafeStore database uninitialised", "/tmp/tmp"));
    EXPECT_CALL(
        *filesystemMock,
        writeFileAtomically(Plugin::getSafeStoreDormantFlagPath(), "SafeStore database corrupt", "/tmp/tmp"));
    ASSERT_EQ(quarantineManager->getState(), QuarantineManagerState::STARTUP);

    for (int i = 0; i < 10; ++i)
    {
        EXPECT_NO_THROW(quarantineManager->initialise());
    }
    ASSERT_EQ(quarantineManager->getState(), QuarantineManagerState::CORRUPT);
}

TEST_F(QuarantineManagerTests, initFailsOnDBOpenFailureAndDbIsMarkedCorrupt)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };
    addCommonPersistValueExpects(*filesystemMock);
    EXPECT_CALL(
        *filesystemMock,
        writeFileAtomically(Plugin::getSafeStoreDormantFlagPath(), "SafeStore database uninitialised", "/tmp/tmp"));
    EXPECT_CALL(
        *filesystemMock,
        writeFileAtomically(Plugin::getSafeStoreDormantFlagPath(), "SafeStore database corrupt", "/tmp/tmp"));
    EXPECT_CALL(*filesystemMock, isFile(Plugin::getSafeStorePasswordFilePath())).WillRepeatedly(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(Plugin::getSafeStorePasswordFilePath())).WillRepeatedly(Return("a password"));
    EXPECT_CALL(*filesystemMock, removeFilesInDirectory("/tmp/av/var/tempUnpack")).Times(10);

    EXPECT_CALL(
        *m_mockSafeStoreWrapper,
        initialise(Plugin::getSafeStoreDbDirPath(), Plugin::getSafeStoreDbFileName(), "a password"))
        .WillRepeatedly(Return(InitReturnCode::DB_OPEN_FAILED));
    auto quarantineManager = createQuarantineManager();

    ASSERT_EQ(quarantineManager->getState(), QuarantineManagerState::STARTUP);

    for (int i = 0; i < 10; ++i)
    {
        EXPECT_NO_THROW(quarantineManager->initialise());
    }
    ASSERT_EQ(quarantineManager->getState(), QuarantineManagerState::CORRUPT);
}

TEST_F(QuarantineManagerTests, quarantineFile)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };
    addCommonPersistValueExpects(*filesystemMock);
    EXPECT_CALL(*filesystemMock, removeFile(Plugin::getSafeStoreDormantFlagPath(), true)).WillOnce(Return()); // 1
    EXPECT_CALL(*filesystemMock, isFile(Plugin::getSafeStorePasswordFilePath())).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(Plugin::getSafeStorePasswordFilePath())).WillOnce(Return("a password"));
    EXPECT_CALL(*filesystemMock, isFile(Plugin::getSafeStoreConfigPath())).WillOnce(Return(false));

    EXPECT_CALL(*filesystemMock, compareFileDescriptors(_, _)).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, unlinkFileUsingDirectoryFD(_, "file"));
    EXPECT_CALL(*filesystemMock, getFileInfoDescriptor(_)).WillOnce(Return(100));
    EXPECT_CALL(*filesystemMock, getFileInfoDescriptorFromDirectoryFD(_, _)).WillOnce(Return(120));
    EXPECT_CALL(*filesystemMock, removeFilesInDirectory("/tmp/av/var/tempUnpack")).Times(1);

    EXPECT_CALL(
        *m_mockSafeStoreWrapper,
        initialise(Plugin::getSafeStoreDbDirPath(), Plugin::getSafeStoreDbFileName(), "a password"))
        .WillOnce(Return(InitReturnCode::OK));

    auto mockGetIdMethods = std::make_shared<StrictMock<MockISafeStoreGetIdMethods>>();
    auto mockReleaseMethods = std::make_shared<StrictMock<MockISafeStoreReleaseMethods>>();

    void* rawHandle1 = reinterpret_cast<SafeStoreObjectHandle>(1111);
    auto objectHandle1 = std::make_unique<ObjectHandleHolder>(mockGetIdMethods, mockReleaseMethods);
    *objectHandle1->getRawHandlePtr() = rawHandle1;
    auto handleAsArg1 = Property(&ObjectHandleHolder::getRawHandle, rawHandle1); // Matches argument with raw handle
    EXPECT_CALL(*mockReleaseMethods, releaseObjectHandle(rawHandle1));

    void* rawHandle2 = reinterpret_cast<SafeStoreObjectHandle>(2222);
    ObjectHandleHolder objectHandle2{ mockGetIdMethods, mockReleaseMethods };
    *objectHandle2.getRawHandlePtr() = rawHandle2;
    auto handleAsArg2 = Property(&ObjectHandleHolder::getRawHandle, rawHandle2); // Matches argument with raw handle
    EXPECT_CALL(*mockReleaseMethods, releaseObjectHandle(rawHandle2));
    EXPECT_CALL(*m_mockSafeStoreWrapper, createObjectHandleHolder()).WillOnce(Return(ByMove(std::move(objectHandle1))));

    EXPECT_CALL(*m_mockSafeStoreWrapper, saveFile(m_dir, m_file, m_threatID, m_threatName, handleAsArg1))
        .WillOnce(Return(SaveFileReturnCode::OK));
    EXPECT_CALL(*m_mockSafeStoreWrapper, setObjectCustomDataString(handleAsArg1, "SHA256", m_SHA256))
        .WillOnce(Return(true));
    EXPECT_CALL(*m_mockSafeStoreWrapper, setObjectCustomDataString(handleAsArg1, "threats", threats_))
        .WillOnce(Return(true));
    EXPECT_CALL(*m_mockSafeStoreWrapper, setObjectCustomDataString(handleAsArg1, "correlationId", m_correlationId))
        .WillOnce(Return(true));
    EXPECT_CALL(*m_mockSafeStoreWrapper, finaliseObject(handleAsArg1)).WillOnce(Return(true));

    std::vector<ObjectHandleHolder> searchResults;
    searchResults.push_back(std::move(objectHandle2));
    EXPECT_CALL(*m_mockSafeStoreWrapper, find(SafeStoreFilter{ m_threatID, {}, {}, {}, ObjectType::FILE, {}, {}, {} }))
        .WillOnce(Return(ByMove(std::move(searchResults))));
    EXPECT_CALL(*filesystemMock, readlink).Times(1).WillOnce(Return(m_path));

    auto quarantineManager = createQuarantineManager();

    EXPECT_NO_THROW(quarantineManager->initialise());
    ASSERT_EQ(quarantineManager->getState(), QuarantineManagerState::INITIALISED);
    datatypes::AutoFd fdHolder;
    EXPECT_EQ(
        common::CentralEnums::QuarantineResult::SUCCESS,
        quarantineManager->quarantineFile(
            m_dir + m_file,
            m_threatID,
            m_threatType,
            m_threatName,
            threatSha256_,
            m_SHA256,
            m_correlationId,
            std::move(fdHolder)));
}

TEST_F(QuarantineManagerTests, quarantineFileLogsWhenSaveFileFails)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };
    addCommonPersistValueExpects(*filesystemMock);
    EXPECT_CALL(*filesystemMock, removeFile(Plugin::getSafeStoreDormantFlagPath(), true)).WillOnce(Return()); // 1
    EXPECT_CALL(*filesystemMock, isFile(Plugin::getSafeStorePasswordFilePath())).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(Plugin::getSafeStorePasswordFilePath())).WillOnce(Return("a password"));
    EXPECT_CALL(*filesystemMock, isFile(Plugin::getSafeStoreConfigPath())).WillOnce(Return(false));
    EXPECT_CALL(*filesystemMock, removeFilesInDirectory("/tmp/av/var/tempUnpack")).Times(1);

    EXPECT_CALL(
        *m_mockSafeStoreWrapper,
        initialise(Plugin::getSafeStoreDbDirPath(), Plugin::getSafeStoreDbFileName(), "a password"))
        .WillOnce(Return(InitReturnCode::OK));

    auto mockGetIdMethods = std::make_shared<StrictMock<MockISafeStoreGetIdMethods>>();
    auto mockReleaseMethods = std::make_shared<StrictMock<MockISafeStoreReleaseMethods>>();

    void* rawHandle1 = reinterpret_cast<SafeStoreObjectHandle>(1111);
    auto objectHandle1 = std::make_unique<ObjectHandleHolder>(mockGetIdMethods, mockReleaseMethods);
    *objectHandle1->getRawHandlePtr() = rawHandle1;
    auto handleAsArg1 = Property(&ObjectHandleHolder::getRawHandle, rawHandle1); // Matches argument with raw handle
    EXPECT_CALL(*mockReleaseMethods, releaseObjectHandle(rawHandle1));

    EXPECT_CALL(*m_mockSafeStoreWrapper, createObjectHandleHolder()).WillOnce(Return(ByMove(std::move(objectHandle1))));
    EXPECT_CALL(*m_mockSafeStoreWrapper, saveFile(m_dir, m_file, m_threatID, m_threatName, handleAsArg1))
        .WillOnce(Return(SaveFileReturnCode::INVALID_ARG));

    EXPECT_CALL(*filesystemMock, readlink).Times(1).WillOnce(Return(m_path));
    EXPECT_CALL(*filesystemMock, getFileInfoDescriptor(_)).WillOnce(Return(100));

    auto quarantineManager = createQuarantineManager();

    EXPECT_NO_THROW(quarantineManager->initialise());
    ASSERT_EQ(quarantineManager->getState(), QuarantineManagerState::INITIALISED);
    datatypes::AutoFd fdHolder;
    EXPECT_EQ(
        common::CentralEnums::QuarantineResult::FAILED_TO_DELETE_FILE,
        quarantineManager->quarantineFile(
            m_dir + m_file,
            m_threatID,
            m_threatType,
            m_threatName,
            threatSha256_,
            m_SHA256,
            m_correlationId,
            std::move(fdHolder)));

    EXPECT_TRUE(appenderContains(
        "Failed to quarantine " + m_dir + m_file +
        " due to: " + GL_SAVE_FILE_RETURN_CODES.at(SaveFileReturnCode::INVALID_ARG)));
}

TEST_F(QuarantineManagerTests, quarantineFileFailsWhenFileDescriptorsDoNotMatch)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };
    addCommonPersistValueExpects(*filesystemMock);
    EXPECT_CALL(*filesystemMock, removeFile(Plugin::getSafeStoreDormantFlagPath(), true)).WillOnce(Return());
    EXPECT_CALL(*filesystemMock, isFile(Plugin::getSafeStorePasswordFilePath())).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(Plugin::getSafeStorePasswordFilePath())).WillOnce(Return("a password"));
    EXPECT_CALL(*filesystemMock, isFile(Plugin::getSafeStoreConfigPath())).WillOnce(Return(false));
    EXPECT_CALL(*filesystemMock, removeFilesInDirectory("/tmp/av/var/tempUnpack")).Times(1);

    EXPECT_CALL(*filesystemMock, compareFileDescriptors(_, _)).WillOnce(Return(false));
    EXPECT_CALL(*filesystemMock, getFileInfoDescriptor(_)).WillOnce(Return(100));
    EXPECT_CALL(*filesystemMock, getFileInfoDescriptorFromDirectoryFD(_, _)).WillOnce(Return(120));

    EXPECT_CALL(
        *m_mockSafeStoreWrapper,
        initialise(Plugin::getSafeStoreDbDirPath(), Plugin::getSafeStoreDbFileName(), "a password"))
        .WillOnce(Return(InitReturnCode::OK));
    auto mockGetIdMethods = std::make_shared<StrictMock<MockISafeStoreGetIdMethods>>();
    auto mockReleaseMethods = std::make_shared<StrictMock<MockISafeStoreReleaseMethods>>();
    EXPECT_CALL(*mockReleaseMethods, releaseObjectHandle(_));
    EXPECT_CALL(*m_mockSafeStoreWrapper, createObjectHandleHolder())
        .WillOnce(Return(ByMove(std::make_unique<ObjectHandleHolder>(mockGetIdMethods, mockReleaseMethods))));

    EXPECT_CALL(*m_mockSafeStoreWrapper, saveFile(m_dir, m_file, m_threatID, m_threatName, _))
        .WillOnce(Return(SaveFileReturnCode::OK));

    EXPECT_CALL(*filesystemMock, readlink).Times(1).WillOnce(Return(m_path));

    auto quarantineManager = createQuarantineManager();

    EXPECT_NO_THROW(quarantineManager->initialise());
    ASSERT_EQ(quarantineManager->getState(), QuarantineManagerState::INITIALISED);
    datatypes::AutoFd fdHolder;
    EXPECT_EQ(
        common::CentralEnums::QuarantineResult::FAILED_TO_DELETE_FILE,
        quarantineManager->quarantineFile(
            m_dir + m_file,
            m_threatID,
            m_threatType,
            m_threatName,
            threatSha256_,
            m_SHA256,
            m_correlationId,
            std::move(fdHolder)));
}

TEST_F(QuarantineManagerTests, quarantineFileFailsWhenUnlinkFails)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };
    addCommonPersistValueExpects(*filesystemMock);
    EXPECT_CALL(*filesystemMock, removeFile(Plugin::getSafeStoreDormantFlagPath(), true)).WillOnce(Return());
    EXPECT_CALL(*filesystemMock, isFile(Plugin::getSafeStorePasswordFilePath())).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(Plugin::getSafeStorePasswordFilePath())).WillOnce(Return("a password"));
    EXPECT_CALL(*filesystemMock, isFile(Plugin::getSafeStoreConfigPath())).WillOnce(Return(false));
    EXPECT_CALL(*filesystemMock, removeFilesInDirectory("/tmp/av/var/tempUnpack")).Times(1);

    EXPECT_CALL(*filesystemMock, compareFileDescriptors(_, _)).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, getFileInfoDescriptor(_)).WillOnce(Return(100));
    EXPECT_CALL(*filesystemMock, getFileInfoDescriptorFromDirectoryFD(_, _)).WillOnce(Return(120));
    EXPECT_CALL(*filesystemMock, unlinkFileUsingDirectoryFD(_, "file")).WillOnce(Throw(IFileSystemException("")));
    EXPECT_CALL(
        *m_mockSafeStoreWrapper,
        initialise(Plugin::getSafeStoreDbDirPath(), Plugin::getSafeStoreDbFileName(), "a password"))
        .WillOnce(Return(InitReturnCode::OK));
    auto mockGetIdMethods = std::make_shared<StrictMock<MockISafeStoreGetIdMethods>>();
    auto mockReleaseMethods = std::make_shared<StrictMock<MockISafeStoreReleaseMethods>>();
    EXPECT_CALL(*mockReleaseMethods, releaseObjectHandle(_));
    EXPECT_CALL(*m_mockSafeStoreWrapper, createObjectHandleHolder())
        .WillOnce(Return(ByMove(std::make_unique<ObjectHandleHolder>(mockGetIdMethods, mockReleaseMethods))));

    EXPECT_CALL(*m_mockSafeStoreWrapper, saveFile(m_dir, m_file, m_threatID, m_threatName, _))
        .WillOnce(Return(SaveFileReturnCode::OK));

    EXPECT_CALL(*filesystemMock, readlink).Times(1).WillOnce(Return(m_path));

    auto quarantineManager = createQuarantineManager();

    EXPECT_NO_THROW(quarantineManager->initialise());
    ASSERT_EQ(quarantineManager->getState(), QuarantineManagerState::INITIALISED);
    datatypes::AutoFd fdHolder;
    EXPECT_EQ(
        common::CentralEnums::QuarantineResult::FAILED_TO_DELETE_FILE,
        quarantineManager->quarantineFile(
            m_dir + m_file,
            m_threatID,
            m_threatType,
            m_threatName,
            threatSha256_,
            m_SHA256,
            m_correlationId,
            std::move(fdHolder)));
}

TEST_F(QuarantineManagerTests, quarantineFileFailsWhenThreatDirectoryDoesNotExist)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };
    addCommonPersistValueExpects(*filesystemMock);
    EXPECT_CALL(*filesystemMock, removeFile(Plugin::getSafeStoreDormantFlagPath(), true)).WillOnce(Return());
    EXPECT_CALL(*filesystemMock, isFile(Plugin::getSafeStorePasswordFilePath())).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(Plugin::getSafeStorePasswordFilePath())).WillOnce(Return("a password"));
    EXPECT_CALL(*filesystemMock, isFile(Plugin::getSafeStoreConfigPath())).WillOnce(Return(false));
    EXPECT_CALL(*filesystemMock, removeFilesInDirectory("/tmp/av/var/tempUnpack")).Times(1);

    EXPECT_CALL(*filesystemMock, getFileInfoDescriptor(_)).WillOnce(Return(-1));

    EXPECT_CALL(
        *m_mockSafeStoreWrapper,
        initialise(Plugin::getSafeStoreDbDirPath(), Plugin::getSafeStoreDbFileName(), "a password"))
        .WillOnce(Return(InitReturnCode::OK));

    auto quarantineManager = createQuarantineManager();

    EXPECT_NO_THROW(quarantineManager->initialise());
    ASSERT_EQ(quarantineManager->getState(), QuarantineManagerState::INITIALISED);
    datatypes::AutoFd fdHolder;
    EXPECT_EQ(
        common::CentralEnums::QuarantineResult::NOT_FOUND,
        quarantineManager->quarantineFile(
            m_dir + m_file,
            m_threatID,
            m_threatType,
            m_threatName,
            threatSha256_,
            m_SHA256,
            m_correlationId,
            std::move(fdHolder)));
}

TEST_F(QuarantineManagerTests, quarantineFileFailsWhenthreatDoesNotExist)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };
    addCommonPersistValueExpects(*filesystemMock);
    EXPECT_CALL(*filesystemMock, removeFile(Plugin::getSafeStoreDormantFlagPath(), true)).WillOnce(Return());
    EXPECT_CALL(*filesystemMock, isFile(Plugin::getSafeStorePasswordFilePath())).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(Plugin::getSafeStorePasswordFilePath())).WillOnce(Return("a password"));
    EXPECT_CALL(*filesystemMock, isFile(Plugin::getSafeStoreConfigPath())).WillOnce(Return(false));
    EXPECT_CALL(*filesystemMock, removeFilesInDirectory("/tmp/av/var/tempUnpack")).Times(1);

    EXPECT_CALL(*filesystemMock, getFileInfoDescriptor(_)).WillOnce(Return(100));
    EXPECT_CALL(*filesystemMock, getFileInfoDescriptorFromDirectoryFD(_, _)).WillOnce(Return(-1));

    EXPECT_CALL(
        *m_mockSafeStoreWrapper,
        initialise(Plugin::getSafeStoreDbDirPath(), Plugin::getSafeStoreDbFileName(), "a password"))
        .WillOnce(Return(InitReturnCode::OK));
    auto mockGetIdMethods = std::make_shared<StrictMock<MockISafeStoreGetIdMethods>>();
    auto mockReleaseMethods = std::make_shared<StrictMock<MockISafeStoreReleaseMethods>>();
    EXPECT_CALL(*mockReleaseMethods, releaseObjectHandle(_));
    EXPECT_CALL(*m_mockSafeStoreWrapper, createObjectHandleHolder())
        .WillOnce(Return(ByMove(std::make_unique<ObjectHandleHolder>(mockGetIdMethods, mockReleaseMethods))));

    EXPECT_CALL(*m_mockSafeStoreWrapper, saveFile(m_dir, m_file, m_threatID, m_threatName, _))
        .WillOnce(Return(SaveFileReturnCode::OK));

    EXPECT_CALL(*filesystemMock, readlink).Times(1).WillOnce(Return(m_path));

    auto quarantineManager = createQuarantineManager();

    EXPECT_NO_THROW(quarantineManager->initialise());
    ASSERT_EQ(quarantineManager->getState(), QuarantineManagerState::INITIALISED);
    datatypes::AutoFd fdHolder;
    EXPECT_EQ(
        common::CentralEnums::QuarantineResult::NOT_FOUND,
        quarantineManager->quarantineFile(
            m_dir + m_file,
            m_threatID,
            m_threatType,
            m_threatName,
            threatSha256_,
            m_SHA256,
            m_correlationId,
            std::move(fdHolder)));
}

TEST_F(QuarantineManagerTests, quarantineFileFailsAndDbIsMarkedCorrupt)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };

    // Force the m_dbErrorCountThreshold variable to be set to 1 for tests so that we only have to see one error
    // before the database is reported to be corrupt.
    EXPECT_CALL(*filesystemMock, removeFile(Plugin::getSafeStoreDormantFlagPath(), true)).WillOnce(Return());
    EXPECT_CALL(*filesystemMock, exists("/tmp/av/var/persist-safeStoreDbErrorThreshold")).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, writeFile("/tmp/av/var/persist-safeStoreDbErrorThreshold", "1"));
    EXPECT_CALL(*filesystemMock, readFile("/tmp/av/var/persist-safeStoreDbErrorThreshold")).WillOnce(Return("1"));
    EXPECT_CALL(*filesystemMock, removeFilesInDirectory("/tmp/av/var/tempUnpack")).Times(1);

    EXPECT_CALL(*filesystemMock, isFile(Plugin::getSafeStoreConfigPath())).WillOnce(Return(false));

    EXPECT_CALL(*filesystemMock, isFile(Plugin::getSafeStorePasswordFilePath())).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(Plugin::getSafeStorePasswordFilePath())).WillOnce(Return("a password"));

    EXPECT_CALL(
        *m_mockSafeStoreWrapper,
        initialise(Plugin::getSafeStoreDbDirPath(), Plugin::getSafeStoreDbFileName(), "a password"))
        .WillRepeatedly(Return(InitReturnCode::OK));

    EXPECT_CALL(*m_mockSafeStoreWrapper, saveFile(m_dir, m_file, m_threatID, m_threatName, _))
        .WillRepeatedly(Return(SaveFileReturnCode::DB_ERROR));
    EXPECT_CALL(
        *filesystemMock,
        writeFileAtomically(Plugin::getSafeStoreDormantFlagPath(), "SafeStore database corrupt", "/tmp/tmp"));
    EXPECT_CALL(*m_mockSafeStoreWrapper, setObjectCustomDataString(_, "SHA256", m_SHA256)).WillRepeatedly(Return(true));
    EXPECT_CALL(*m_mockSafeStoreWrapper, setObjectCustomDataString(_, "correlationId", m_correlationId))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*m_mockSafeStoreWrapper, finaliseObject(_)).WillRepeatedly(Return(true));

    auto mockGetIdMethods = std::make_shared<StrictMock<MockISafeStoreGetIdMethods>>();
    auto mockReleaseMethods = std::make_shared<StrictMock<MockISafeStoreReleaseMethods>>();
    EXPECT_CALL(*mockReleaseMethods, releaseObjectHandle(_));

    EXPECT_CALL(*m_mockSafeStoreWrapper, createObjectHandleHolder())
        .WillOnce(Return(ByMove(std::make_unique<ObjectHandleHolder>(mockGetIdMethods, mockReleaseMethods))));

    EXPECT_CALL(*filesystemMock, readlink).Times(1).WillOnce(Return(m_path));
    EXPECT_CALL(*filesystemMock, getFileInfoDescriptor(_)).WillOnce(Return(100));

    auto quarantineManager = createQuarantineManager();

    EXPECT_NO_THROW(quarantineManager->initialise());
    ASSERT_EQ(quarantineManager->getState(), QuarantineManagerState::INITIALISED);
    datatypes::AutoFd fdHolder;
    EXPECT_EQ(
        common::CentralEnums::QuarantineResult::FAILED_TO_DELETE_FILE,
        quarantineManager->quarantineFile(
            m_dir + m_file,
            m_threatID,
            m_threatType,
            m_threatName,
            threatSha256_,
            m_SHA256,
            m_correlationId,
            std::move(fdHolder)));

    EXPECT_EQ(quarantineManager->getState(), QuarantineManagerState::CORRUPT);
}

TEST_F(QuarantineManagerTests, quarantineFileFailsToFinaliseFile)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };
    addCommonPersistValueExpects(*filesystemMock);
    EXPECT_CALL(*filesystemMock, removeFile(Plugin::getSafeStoreDormantFlagPath(), true)).WillOnce(Return());
    EXPECT_CALL(*filesystemMock, isFile(Plugin::getSafeStorePasswordFilePath())).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(Plugin::getSafeStorePasswordFilePath())).WillOnce(Return("a password"));
    EXPECT_CALL(*filesystemMock, isFile(Plugin::getSafeStoreConfigPath())).WillOnce(Return(false));
    EXPECT_CALL(*filesystemMock, removeFilesInDirectory("/tmp/av/var/tempUnpack")).Times(1);

    EXPECT_CALL(*filesystemMock, compareFileDescriptors(_, _)).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, unlinkFileUsingDirectoryFD(_, _));
    EXPECT_CALL(*filesystemMock, getFileInfoDescriptor(_)).WillOnce(Return(100));
    EXPECT_CALL(*filesystemMock, getFileInfoDescriptorFromDirectoryFD(_, _)).WillOnce(Return(120));

    EXPECT_CALL(
        *m_mockSafeStoreWrapper,
        initialise(Plugin::getSafeStoreDbDirPath(), Plugin::getSafeStoreDbFileName(), "a password"))
        .WillOnce(Return(InitReturnCode::OK));

    auto mockGetIdMethods = std::make_shared<StrictMock<MockISafeStoreGetIdMethods>>();
    auto mockReleaseMethods = std::make_shared<StrictMock<MockISafeStoreReleaseMethods>>();

    EXPECT_CALL(*mockReleaseMethods, releaseObjectHandle(_));
    EXPECT_CALL(*m_mockSafeStoreWrapper, createObjectHandleHolder())
        .WillOnce(Return(ByMove(std::make_unique<ObjectHandleHolder>(mockGetIdMethods, mockReleaseMethods))));

    EXPECT_CALL(*m_mockSafeStoreWrapper, saveFile(m_dir, m_file, m_threatID, m_threatName, _))
        .WillOnce(Return(SaveFileReturnCode::OK));
    EXPECT_CALL(*m_mockSafeStoreWrapper, setObjectCustomDataString(_, "SHA256", m_SHA256)).WillOnce(Return(true));
    EXPECT_CALL(*m_mockSafeStoreWrapper, setObjectCustomDataString(_, "threats", threats_)).WillOnce(Return(true));
    EXPECT_CALL(*m_mockSafeStoreWrapper, setObjectCustomDataString(_, "correlationId", m_correlationId))
        .WillOnce(Return(true));
    EXPECT_CALL(*m_mockSafeStoreWrapper, finaliseObject(_)).WillOnce(Return(false));

    std::vector<ObjectHandleHolder> searchResults;
    EXPECT_CALL(*m_mockSafeStoreWrapper, find).WillOnce(Return(ByMove(std::move(searchResults))));

    EXPECT_CALL(*filesystemMock, readlink).Times(1).WillOnce(Return(m_path));

    auto quarantineManager = createQuarantineManager();

    EXPECT_NO_THROW(quarantineManager->initialise());
    ASSERT_EQ(quarantineManager->getState(), QuarantineManagerState::INITIALISED);
    datatypes::AutoFd fdHolder;
    EXPECT_EQ(
        common::CentralEnums::QuarantineResult::SUCCESS,
        quarantineManager->quarantineFile(
            m_dir + m_file,
            m_threatID,
            m_threatType,
            m_threatName,
            threatSha256_,
            m_SHA256,
            m_correlationId,
            std::move(fdHolder)));
}

TEST_F(QuarantineManagerTests, fileQuarantinesAndRemovesPreviouslySavedObjectsWithSameThreatId)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };
    addCommonPersistValueExpects(*filesystemMock);
    EXPECT_CALL(*filesystemMock, removeFile(Plugin::getSafeStoreDormantFlagPath(), true)).WillOnce(Return());
    EXPECT_CALL(*filesystemMock, isFile(Plugin::getSafeStorePasswordFilePath())).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(Plugin::getSafeStorePasswordFilePath())).WillOnce(Return("a password"));
    EXPECT_CALL(*filesystemMock, isFile(Plugin::getSafeStoreConfigPath())).WillOnce(Return(false));
    EXPECT_CALL(*filesystemMock, compareFileDescriptors(_, _)).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, unlinkFileUsingDirectoryFD(_, "file"));
    EXPECT_CALL(*filesystemMock, getFileInfoDescriptor(_)).WillOnce(Return(100));
    EXPECT_CALL(*filesystemMock, getFileInfoDescriptorFromDirectoryFD(_, _)).WillOnce(Return(120));
    EXPECT_CALL(*filesystemMock, removeFilesInDirectory("/tmp/av/var/tempUnpack")).Times(1);

    EXPECT_CALL(*m_mockSafeStoreWrapper, initialise("/tmp/av/var/safestore_db", "safestore.db", "a password"))
        .WillOnce(Return(InitReturnCode::OK));

    auto mockGetIdMethods = std::make_shared<StrictMock<MockISafeStoreGetIdMethods>>();
    auto mockReleaseMethods = std::make_shared<StrictMock<MockISafeStoreReleaseMethods>>();

    // Newly quarantined object
    const std::string object1_id = "11111111-1111-1111-1111-111111111111";
    // Handle 1
    void* object1_1_rawHandle = reinterpret_cast<SafeStoreObjectHandle>(1111);
    auto object1_1_handle = std::make_unique<ObjectHandleHolder>(mockGetIdMethods, mockReleaseMethods);
    *object1_1_handle->getRawHandlePtr() = object1_1_rawHandle;
    auto object1_1_handleArg = Property(&ObjectHandleHolder::getRawHandle, object1_1_rawHandle);
    EXPECT_CALL(*mockReleaseMethods, releaseObjectHandle(object1_1_rawHandle));
    // Handle 2
    void* object1_2_rawHandle = reinterpret_cast<SafeStoreObjectHandle>(2222);
    ObjectHandleHolder object1_2_handle{ mockGetIdMethods, mockReleaseMethods };
    *object1_2_handle.getRawHandlePtr() = object1_2_rawHandle;
    auto object1_2_handleArg = Property(&ObjectHandleHolder::getRawHandle, object1_2_rawHandle);
    EXPECT_CALL(*mockReleaseMethods, releaseObjectHandle(object1_2_rawHandle));

    // Previously quarantined objects
    const std::string object2_id = "22222222-2222-2222-2222-222222222222";
    void* object2_rawHandle = reinterpret_cast<SafeStoreObjectHandle>(3333);
    ObjectHandleHolder object2_handle{ mockGetIdMethods, mockReleaseMethods };
    *object2_handle.getRawHandlePtr() = object2_rawHandle;
    auto object2_handleArg = Property(&ObjectHandleHolder::getRawHandle, object2_rawHandle);
    EXPECT_CALL(*mockReleaseMethods, releaseObjectHandle(object2_rawHandle));

    const std::string object3_id = "33333333-3333-3333-3333-333333333333";
    void* object3_rawHandle = reinterpret_cast<SafeStoreObjectHandle>(4444);
    ObjectHandleHolder object3_handle{ mockGetIdMethods, mockReleaseMethods };
    *object3_handle.getRawHandlePtr() = object3_rawHandle;
    auto object3_handleArg = Property(&ObjectHandleHolder::getRawHandle, object3_rawHandle);
    EXPECT_CALL(*mockReleaseMethods, releaseObjectHandle(object3_rawHandle));

    EXPECT_CALL(*m_mockSafeStoreWrapper, createObjectHandleHolder())
        .WillOnce(Return(ByMove(std::move(object1_1_handle))));

    EXPECT_CALL(*m_mockSafeStoreWrapper, saveFile(m_dir, m_file, m_threatID, m_threatName, object1_1_handleArg))
        .WillOnce(Return(SaveFileReturnCode::OK));
    EXPECT_CALL(*m_mockSafeStoreWrapper, setObjectCustomDataString(object1_1_handleArg, "SHA256", m_SHA256))
        .WillOnce(Return(true));
    EXPECT_CALL(*m_mockSafeStoreWrapper, setObjectCustomDataString(object1_1_handleArg, "threats", threats_))
        .WillOnce(Return(true));
    EXPECT_CALL(
        *m_mockSafeStoreWrapper, setObjectCustomDataString(object1_1_handleArg, "correlationId", m_correlationId))
        .WillOnce(Return(true));
    EXPECT_CALL(*m_mockSafeStoreWrapper, finaliseObject(object1_1_handleArg)).WillOnce(Return(true));

    std::vector<ObjectHandleHolder> searchResults;
    searchResults.push_back(std::move(object1_2_handle));
    searchResults.push_back(std::move(object2_handle));
    searchResults.push_back(std::move(object3_handle));
    EXPECT_CALL(*m_mockSafeStoreWrapper, find(SafeStoreFilter{ m_threatID, {}, {}, {}, ObjectType::FILE, {}, {}, {} }))
        .WillOnce(Return(ByMove(std::move(searchResults))));

    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectId(object1_1_handleArg)).WillOnce(Return(object1_id));

    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectId(object1_2_handleArg)).WillOnce(Return(object1_id));

    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectId(object2_handleArg)).WillOnce(Return(object2_id));
    EXPECT_CALL(*m_mockSafeStoreWrapper, deleteObjectById(object2_id)).WillOnce(Return(true));

    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectId(object3_handleArg)).WillOnce(Return(object3_id));
    EXPECT_CALL(*m_mockSafeStoreWrapper, deleteObjectById(object3_id)).WillOnce(Return(true));

    EXPECT_CALL(*filesystemMock, readlink).Times(1).WillOnce(Return(m_path));

    auto quarantineManager = createQuarantineManager();

    EXPECT_NO_THROW(quarantineManager->initialise());
    ASSERT_EQ(quarantineManager->getState(), QuarantineManagerState::INITIALISED);
    datatypes::AutoFd fdHolder;
    EXPECT_EQ(
        common::CentralEnums::QuarantineResult::SUCCESS,
        quarantineManager->quarantineFile(
            m_dir + m_file,
            m_threatID,
            m_threatType,
            m_threatName,
            threatSha256_,
            m_SHA256,
            m_correlationId,
            std::move(fdHolder)));
}

TEST_F(QuarantineManagerTests, fileQuarantinesButFailsToRemovePreviouslySavedObjectWithSameThreatId)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };
    addCommonPersistValueExpects(*filesystemMock);
    EXPECT_CALL(*filesystemMock, removeFile(Plugin::getSafeStoreDormantFlagPath(), true)).WillOnce(Return());
    EXPECT_CALL(*filesystemMock, isFile(Plugin::getSafeStorePasswordFilePath())).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(Plugin::getSafeStorePasswordFilePath())).WillOnce(Return("a password"));
    EXPECT_CALL(*filesystemMock, isFile(Plugin::getSafeStoreConfigPath())).WillOnce(Return(false));
    EXPECT_CALL(*filesystemMock, compareFileDescriptors(_, _)).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, unlinkFileUsingDirectoryFD(_, "file"));
    EXPECT_CALL(*filesystemMock, getFileInfoDescriptor(_)).WillOnce(Return(100));
    EXPECT_CALL(*filesystemMock, getFileInfoDescriptorFromDirectoryFD(_, _)).WillOnce(Return(120));
    EXPECT_CALL(*filesystemMock, removeFilesInDirectory("/tmp/av/var/tempUnpack")).Times(1);

    EXPECT_CALL(*m_mockSafeStoreWrapper, initialise("/tmp/av/var/safestore_db", "safestore.db", "a password"))
        .WillOnce(Return(InitReturnCode::OK));

    auto mockGetIdMethods = std::make_shared<StrictMock<MockISafeStoreGetIdMethods>>();
    auto mockReleaseMethods = std::make_shared<StrictMock<MockISafeStoreReleaseMethods>>();

    // Newly quarantined object
    const std::string object1_id = "11111111-1111-1111-1111-111111111111";
    // Handle 1
    void* object1_1_rawHandle = reinterpret_cast<SafeStoreObjectHandle>(1111);
    auto object1_1_handle = std::make_unique<ObjectHandleHolder>(mockGetIdMethods, mockReleaseMethods);
    *object1_1_handle->getRawHandlePtr() = object1_1_rawHandle;
    auto object1_1_handleArg = Property(&ObjectHandleHolder::getRawHandle, object1_1_rawHandle);
    EXPECT_CALL(*mockReleaseMethods, releaseObjectHandle(object1_1_rawHandle));
    // Handle 2
    void* object1_2_rawHandle = reinterpret_cast<SafeStoreObjectHandle>(2222);
    ObjectHandleHolder object1_2_handle{ mockGetIdMethods, mockReleaseMethods };
    *object1_2_handle.getRawHandlePtr() = object1_2_rawHandle;
    auto object1_2_handleArg = Property(&ObjectHandleHolder::getRawHandle, object1_2_rawHandle);
    EXPECT_CALL(*mockReleaseMethods, releaseObjectHandle(object1_2_rawHandle));

    // Previously quarantined objects
    const std::string object2_id = "22222222-2222-2222-2222-222222222222";
    void* object2_rawHandle = reinterpret_cast<SafeStoreObjectHandle>(3333);
    ObjectHandleHolder object2_handle{ mockGetIdMethods, mockReleaseMethods };
    *object2_handle.getRawHandlePtr() = object2_rawHandle;
    auto object2_handleArg = Property(&ObjectHandleHolder::getRawHandle, object2_rawHandle);
    EXPECT_CALL(*mockReleaseMethods, releaseObjectHandle(object2_rawHandle));

    const std::string object3_id = "33333333-3333-3333-3333-333333333333";
    void* object3_rawHandle = reinterpret_cast<SafeStoreObjectHandle>(4444);
    ObjectHandleHolder object3_handle{ mockGetIdMethods, mockReleaseMethods };
    *object3_handle.getRawHandlePtr() = object3_rawHandle;
    auto object3_handleArg = Property(&ObjectHandleHolder::getRawHandle, object3_rawHandle);
    EXPECT_CALL(*mockReleaseMethods, releaseObjectHandle(object3_rawHandle));

    EXPECT_CALL(*m_mockSafeStoreWrapper, createObjectHandleHolder())
        .WillOnce(Return(ByMove(std::move(object1_1_handle))));

    EXPECT_CALL(*m_mockSafeStoreWrapper, saveFile(m_dir, m_file, m_threatID, m_threatName, object1_1_handleArg))
        .WillOnce(Return(SaveFileReturnCode::OK));
    EXPECT_CALL(*m_mockSafeStoreWrapper, setObjectCustomDataString(object1_1_handleArg, "SHA256", m_SHA256))
        .WillOnce(Return(true));
    EXPECT_CALL(*m_mockSafeStoreWrapper, setObjectCustomDataString(object1_1_handleArg, "threats", threats_))
        .WillOnce(Return(true));
    EXPECT_CALL(
        *m_mockSafeStoreWrapper, setObjectCustomDataString(object1_1_handleArg, "correlationId", m_correlationId))
        .WillOnce(Return(true));
    EXPECT_CALL(*m_mockSafeStoreWrapper, finaliseObject(object1_1_handleArg)).WillOnce(Return(true));

    std::vector<ObjectHandleHolder> searchResults;
    searchResults.push_back(std::move(object1_2_handle));
    searchResults.push_back(std::move(object2_handle));
    searchResults.push_back(std::move(object3_handle));
    EXPECT_CALL(*m_mockSafeStoreWrapper, find(SafeStoreFilter{ m_threatID, {}, {}, {}, ObjectType::FILE, {}, {}, {} }))
        .WillOnce(Return(ByMove(std::move(searchResults))));

    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectId(object1_1_handleArg)).WillOnce(Return(object1_id));

    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectId(object1_2_handleArg)).WillOnce(Return(object1_id));

    // This one fails to be deleted
    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectId(object2_handleArg)).WillOnce(Return(object2_id));
    EXPECT_CALL(*m_mockSafeStoreWrapper, deleteObjectById(object2_id)).WillOnce(Return(false));

    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectId(object3_handleArg)).WillOnce(Return(object3_id));
    EXPECT_CALL(*m_mockSafeStoreWrapper, deleteObjectById(object3_id)).WillOnce(Return(true));

    EXPECT_CALL(*filesystemMock, readlink).Times(1).WillOnce(Return(m_path));

    auto quarantineManager = createQuarantineManager();

    EXPECT_NO_THROW(quarantineManager->initialise());
    ASSERT_EQ(quarantineManager->getState(), QuarantineManagerState::INITIALISED);
    datatypes::AutoFd fdHolder;
    EXPECT_EQ(
        common::CentralEnums::QuarantineResult::SUCCESS,
        quarantineManager->quarantineFile(
            m_dir + m_file,
            m_threatID,
            m_threatType,
            m_threatName,
            threatSha256_,
            m_SHA256,
            m_correlationId,
            std::move(fdHolder)));
}

TEST_F(QuarantineManagerTests, tryToQuarantineFileWhenThreatIdIsIncorrectSize)
{
    std::string threatId = "Tfe8974b97b4b7a6a33b4c52acb4ffba0c11ebbf208a519245791ad32a96227d8";

    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };
    addCommonPersistValueExpects(*filesystemMock);
    EXPECT_CALL(*filesystemMock, removeFile(Plugin::getSafeStoreDormantFlagPath(), true)).WillOnce(Return());
    EXPECT_CALL(*filesystemMock, isFile(Plugin::getSafeStorePasswordFilePath())).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(Plugin::getSafeStorePasswordFilePath())).WillOnce(Return("a password"));
    EXPECT_CALL(*filesystemMock, isFile(Plugin::getSafeStoreConfigPath())).WillOnce(Return(false));
    EXPECT_CALL(*filesystemMock, removeFilesInDirectory("/tmp/av/var/tempUnpack")).Times(1);

    EXPECT_CALL(
        *m_mockSafeStoreWrapper,
        initialise(Plugin::getSafeStoreDbDirPath(), Plugin::getSafeStoreDbFileName(), "a password"))
        .WillOnce(Return(InitReturnCode::OK));

    auto quarantineManager = createQuarantineManager();
    EXPECT_NO_THROW(quarantineManager->initialise());
    ASSERT_EQ(quarantineManager->getState(), QuarantineManagerState::INITIALISED);
    datatypes::AutoFd fdHolder;
    EXPECT_EQ(
        common::CentralEnums::QuarantineResult::FAILED_TO_DELETE_FILE,
        quarantineManager->quarantineFile(
            m_dir + m_file,
            threatId,
            m_threatType,
            m_threatName,
            threatSha256_,
            m_SHA256,
            m_correlationId,
            std::move(fdHolder)));
}

TEST_F(QuarantineManagerTests, tryToQuarantineFileWhenUninitialised)
{
    EXPECT_CALL(*m_mockSafeStoreWrapper, saveFile(m_dir, m_file, m_threatID, m_threatName, _)).Times(0);

    auto quarantineManager = createQuarantineManager();

    ASSERT_EQ(quarantineManager->getState(), QuarantineManagerState::STARTUP);
    datatypes::AutoFd fdHolder;
    common::CentralEnums::QuarantineResult result;
    EXPECT_NO_THROW(
        result = quarantineManager->quarantineFile(
            m_dir + m_file,
            m_threatID,
            m_threatType,
            m_threatName,
            threatSha256_,
            m_SHA256,
            m_correlationId,
            std::move(fdHolder)));
    EXPECT_EQ(result, common::CentralEnums::QuarantineResult::FAILED_TO_DELETE_FILE);
}

TEST_F(QuarantineManagerTests, deleteDatabaseCalledOnInitialisedDb)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };
    addCommonPersistValueExpects(*filesystemMock);
    EXPECT_CALL(*filesystemMock, removeFile(Plugin::getSafeStoreDormantFlagPath(), true)).WillOnce(Return());
    EXPECT_CALL(*filesystemMock, isFile(Plugin::getSafeStorePasswordFilePath())).WillOnce(Return(false));
    EXPECT_CALL(*filesystemMock, readFile(Plugin::getSafeStorePasswordFilePath())).Times(0);
    EXPECT_CALL(*filesystemMock, isDirectory(Plugin::getSafeStoreDbDirPath())).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, writeFile(Plugin::getSafeStorePasswordFilePath(), _)).Times(1);
    EXPECT_CALL(*filesystemMock, exists(Plugin::getSafeStoreDbDirPath())).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, removeFileOrDirectory(Plugin::getSafeStoreDbDirPath())).Times(1);
    EXPECT_CALL(*filesystemMock, makedirs(Plugin::getSafeStoreDbDirPath())).Times(1);
    EXPECT_CALL(*filesystemMock, isFile(Plugin::getSafeStoreConfigPath())).WillOnce(Return(false));
    EXPECT_CALL(*filesystemMock, removeFilesInDirectory("/tmp/av/var/tempUnpack")).Times(1);

    auto* filePermissionsMock = new StrictMock<MockFilePermissions>();
    Tests::ScopedReplaceFilePermissions scopedReplaceFilePermissions{
        std::unique_ptr<Common::FileSystem::IFilePermissions>(filePermissionsMock)
    };

    EXPECT_CALL(*filePermissionsMock, chown(Plugin::getSafeStoreDbDirPath(), "root", "root")).WillOnce(Return());
    EXPECT_CALL(*filePermissionsMock, chmod(Plugin::getSafeStoreDbDirPath(), S_IRUSR | S_IWUSR | S_IXUSR))
        .WillOnce(Return());

    EXPECT_CALL(
        *filesystemMock,
        writeFileAtomically(Plugin::getSafeStoreDormantFlagPath(), "SafeStore database uninitialised", "/tmp/tmp"));
    EXPECT_CALL(
        *m_mockSafeStoreWrapper, initialise(Plugin::getSafeStoreDbDirPath(), Plugin::getSafeStoreDbFileName(), _))
        .WillOnce(Return(InitReturnCode::OK));
    auto quarantineManager = createQuarantineManager();

    EXPECT_NO_THROW(quarantineManager->initialise());
    ASSERT_EQ(quarantineManager->getState(), QuarantineManagerState::INITIALISED);
    EXPECT_NO_THROW(quarantineManager->deleteDatabase());
}

TEST_F(QuarantineManagerTests, deleteDatabaseCalledOnUninitialisedDbThatDoesNotExist)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };
    addCommonPersistValueExpects(*filesystemMock);
    EXPECT_CALL(*filesystemMock, exists(Plugin::getSafeStoreDbDirPath())).WillOnce(Return(false));

    auto quarantineManager = createQuarantineManager();

    ASSERT_EQ(quarantineManager->getState(), QuarantineManagerState::STARTUP);
    EXPECT_NO_THROW(quarantineManager->deleteDatabase());
}

TEST_F(QuarantineManagerTests, deleteDatabaseCalledOnUninitialisedDbThatDoesExist)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };
    addCommonPersistValueExpects(*filesystemMock);
    EXPECT_CALL(
        *filesystemMock,
        writeFileAtomically(Plugin::getSafeStoreDormantFlagPath(), "SafeStore database uninitialised", "/tmp/tmp"));
    EXPECT_CALL(*filesystemMock, exists(Plugin::getSafeStoreDbDirPath())).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, removeFileOrDirectory(Plugin::getSafeStoreDbDirPath())).Times(1);
    EXPECT_CALL(*filesystemMock, makedirs(Plugin::getSafeStoreDbDirPath())).Times(1);

    auto* filePermissionsMock = new StrictMock<MockFilePermissions>();
    Tests::ScopedReplaceFilePermissions scopedReplaceFilePermissions{
        std::unique_ptr<Common::FileSystem::IFilePermissions>(filePermissionsMock)
    };

    EXPECT_CALL(*filePermissionsMock, chown(Plugin::getSafeStoreDbDirPath(), "root", "root")).WillOnce(Return());
    EXPECT_CALL(*filePermissionsMock, chmod(Plugin::getSafeStoreDbDirPath(), S_IRUSR | S_IWUSR | S_IXUSR))
        .WillOnce(Return());

    auto quarantineManager = createQuarantineManager();

    ASSERT_EQ(quarantineManager->getState(), QuarantineManagerState::STARTUP);
    EXPECT_NO_THROW(quarantineManager->deleteDatabase());
}

TEST_F(QuarantineManagerTests, deleteDatabaseDoesNotThrowOnFailure)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };
    addCommonPersistValueExpects(*filesystemMock);
    EXPECT_CALL(*filesystemMock, exists(Plugin::getSafeStoreDbDirPath())).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, removeFileOrDirectory(Plugin::getSafeStoreDbDirPath()))
        .WillOnce(Throw(Common::FileSystem::IFileSystemException("File does not exist.")));

    auto quarantineManager = createQuarantineManager();

    bool result = true;
    EXPECT_NO_THROW(result = quarantineManager->deleteDatabase());
    ASSERT_FALSE(result);
}

TEST_F(QuarantineManagerTests, extractQuarantinedFile)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };
    auto* filePermissionsMock = new StrictMock<MockFilePermissions>();
    Tests::ScopedReplaceFilePermissions scopedReplaceFilePermissions{
        std::unique_ptr<Common::FileSystem::IFilePermissions>(filePermissionsMock)
    };

    addCommonPersistValueExpects(*filesystemMock);

    EXPECT_CALL(*filePermissionsMock, chmod("/tmp/av/var/tempUnpack", _)).WillOnce(Return());
    EXPECT_CALL(*filePermissionsMock, chown("/tmp/av/var/tempUnpack", "root", "root")).WillOnce(Return());
    EXPECT_CALL(*filesystemMock, removeFileOrDirectory("/tmp/av/var/tempUnpack")).WillOnce(Return());
    EXPECT_CALL(*filesystemMock, makedirs("/tmp/av/var/tempUnpack")).WillOnce(Return());

    std::vector<std::string> unpackedFiles = { "file" };
    EXPECT_CALL(*filesystemMock, listAllFilesInDirectoryTree("/tmp/av/var/tempUnpack")).WillOnce(Return(unpackedFiles));

    EXPECT_CALL(*filePermissionsMock, chmod("file", _)).WillOnce(Return());
    EXPECT_CALL(*filePermissionsMock, chown("file", "root", "root")).WillOnce(Return());
    EXPECT_CALL(*filesystemMock, removeFile("file")).WillOnce(Return());

    auto mockGetIdMethods = std::make_shared<StrictMock<MockISafeStoreGetIdMethods>>();
    auto mockReleaseMethods = std::make_shared<StrictMock<MockISafeStoreReleaseMethods>>();
    EXPECT_CALL(*mockReleaseMethods, releaseObjectHandle(_)).Times(1);

    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectId(_)).Times(1);
    EXPECT_CALL(*m_mockSafeStoreWrapper, restoreObjectByIdToLocation(_, _)).WillOnce(Return(true));

    EXPECT_CALL(*m_mockSysCallWrapper, _open(_, _, _)).WillOnce(Return(100));

    auto quarantineManager = createQuarantineManager();

    ObjectHandleHolder holder = safestore::SafeStoreWrapper::ObjectHandleHolder(mockGetIdMethods, mockReleaseMethods);
    auto actualFile = quarantineManager->extractQuarantinedFile(std::move(holder));
    EXPECT_TRUE(actualFile.has_value());
    EXPECT_EQ(100, actualFile.value().first);
}

TEST_F(QuarantineManagerTests, extractQuarantinedFileHandlesFailedToRemoveFileFollowedByAFailToRemoveUnpackDir)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };
    auto* filePermissionsMock = new StrictMock<MockFilePermissions>();
    Tests::ScopedReplaceFilePermissions scopedReplaceFilePermissions{
        std::unique_ptr<Common::FileSystem::IFilePermissions>(filePermissionsMock)
    };

    addCommonPersistValueExpects(*filesystemMock);

    EXPECT_CALL(*filePermissionsMock, chmod("/tmp/av/var/tempUnpack", _)).WillOnce(Return());
    EXPECT_CALL(*filePermissionsMock, chown("/tmp/av/var/tempUnpack", "root", "root")).WillOnce(Return());
    EXPECT_CALL(*filesystemMock, removeFileOrDirectory("/tmp/av/var/tempUnpack"))
        .WillOnce(Throw(Common::FileSystem::IFileSystemException("exception")));
    EXPECT_CALL(*filesystemMock, makedirs("/tmp/av/var/tempUnpack")).WillOnce(Return());

    std::vector<std::string> unpackedFiles = { "file" };
    EXPECT_CALL(*filesystemMock, listAllFilesInDirectoryTree("/tmp/av/var/tempUnpack")).WillOnce(Return(unpackedFiles));

    EXPECT_CALL(*filePermissionsMock, chmod("file", _)).WillOnce(Return());
    EXPECT_CALL(*filePermissionsMock, chown("file", "root", "root")).WillOnce(Return());
    EXPECT_CALL(*filesystemMock, removeFile("file"))
        .WillOnce(Throw(Common::FileSystem::IFileSystemException("exception")));

    auto mockGetIdMethods = std::make_shared<StrictMock<MockISafeStoreGetIdMethods>>();
    auto mockReleaseMethods = std::make_shared<StrictMock<MockISafeStoreReleaseMethods>>();
    EXPECT_CALL(*mockReleaseMethods, releaseObjectHandle(_)).Times(1);

    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectId(_)).Times(1);
    EXPECT_CALL(*m_mockSafeStoreWrapper, restoreObjectByIdToLocation(_, _)).WillOnce(Return(true));

    EXPECT_CALL(*m_mockSysCallWrapper, _open(_, _, _)).WillOnce(Return(100));

    auto quarantineManager = createQuarantineManager();
    
    ObjectHandleHolder holder = safestore::SafeStoreWrapper::ObjectHandleHolder(mockGetIdMethods, mockReleaseMethods);
    auto actualFile = quarantineManager->extractQuarantinedFile(std::move(holder));
    EXPECT_TRUE(actualFile.has_value());
    EXPECT_TRUE(appenderContains("Failed to clean up threat with error: exception"));
    EXPECT_TRUE(appenderContains("Failed to clean up staging location for rescan with error:"));
    EXPECT_EQ(100, actualFile.value().first);
}

TEST_F(QuarantineManagerTests, extractQuarantinedFileHandlesAFailToRemoveUnpackDir)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };
    auto* filePermissionsMock = new StrictMock<MockFilePermissions>();
    Tests::ScopedReplaceFilePermissions scopedReplaceFilePermissions{
        std::unique_ptr<Common::FileSystem::IFilePermissions>(filePermissionsMock)
    };

    addCommonPersistValueExpects(*filesystemMock);

    EXPECT_CALL(*filePermissionsMock, chmod("/tmp/av/var/tempUnpack", _)).WillOnce(Return());
    EXPECT_CALL(*filePermissionsMock, chown("/tmp/av/var/tempUnpack", "root", "root")).WillOnce(Return());
    EXPECT_CALL(*filesystemMock, removeFileOrDirectory("/tmp/av/var/tempUnpack"))
        .WillOnce(Throw(Common::FileSystem::IFileSystemException("exception")));
    EXPECT_CALL(*filesystemMock, makedirs("/tmp/av/var/tempUnpack")).WillOnce(Return());

    std::vector<std::string> unpackedFiles = { "file" };
    EXPECT_CALL(*filesystemMock, listAllFilesInDirectoryTree("/tmp/av/var/tempUnpack")).WillOnce(Return(unpackedFiles));

    EXPECT_CALL(*filePermissionsMock, chmod("file", _)).WillOnce(Return());
    EXPECT_CALL(*filePermissionsMock, chown("file", "root", "root")).WillOnce(Return());
    EXPECT_CALL(*filesystemMock, removeFile("file")).WillOnce(Return());

    auto mockGetIdMethods = std::make_shared<StrictMock<MockISafeStoreGetIdMethods>>();
    auto mockReleaseMethods = std::make_shared<StrictMock<MockISafeStoreReleaseMethods>>();
    EXPECT_CALL(*mockReleaseMethods, releaseObjectHandle(_)).Times(1);

    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectId(_)).Times(1);
    EXPECT_CALL(*m_mockSafeStoreWrapper, restoreObjectByIdToLocation(_, _)).WillOnce(Return(true));

    EXPECT_CALL(*m_mockSysCallWrapper, _open(_, _, _)).WillOnce(Return(100));

    auto quarantineManager = createQuarantineManager();

    ObjectHandleHolder holder = safestore::SafeStoreWrapper::ObjectHandleHolder(mockGetIdMethods, mockReleaseMethods);
    auto actualFile = quarantineManager->extractQuarantinedFile(std::move(holder));
    EXPECT_TRUE(actualFile.has_value());
    EXPECT_TRUE(appenderContains("Failed to clean up staging location for rescan with error:"));
    EXPECT_EQ(100, actualFile.value().first);
}

TEST_F(QuarantineManagerTests, extractQuarantinedFileAbortsWhenThereIsMoreThanOneFileInUnpackDir)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };
    auto* filePermissionsMock = new StrictMock<MockFilePermissions>();
    Tests::ScopedReplaceFilePermissions scopedReplaceFilePermissions{
        std::unique_ptr<Common::FileSystem::IFilePermissions>(filePermissionsMock)
    };

    addCommonPersistValueExpects(*filesystemMock);

    EXPECT_CALL(*filePermissionsMock, chmod("/tmp/av/var/tempUnpack", _)).Times(1);
    EXPECT_CALL(*filePermissionsMock, chown("/tmp/av/var/tempUnpack", "root", "root")).Times(1);
    EXPECT_CALL(*filesystemMock, removeFileOrDirectory("/tmp/av/var/tempUnpack")).Times(1);
    EXPECT_CALL(*filesystemMock, makedirs("/tmp/av/var/tempUnpack")).Times(1);

    std::vector<std::string> unpackedFiles = { "file", "file2" };
    EXPECT_CALL(*filesystemMock, listAllFilesInDirectoryTree("/tmp/av/var/tempUnpack")).WillOnce(Return(unpackedFiles));

    auto mockGetIdMethods = std::make_shared<StrictMock<MockISafeStoreGetIdMethods>>();
    auto mockReleaseMethods = std::make_shared<StrictMock<MockISafeStoreReleaseMethods>>();
    EXPECT_CALL(*mockReleaseMethods, releaseObjectHandle(_)).Times(1);

    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectId(_)).Times(1);
    EXPECT_CALL(*m_mockSafeStoreWrapper, restoreObjectByIdToLocation(_, _)).WillOnce(Return(true));

    auto quarantineManager = createQuarantineManager();

    ObjectHandleHolder holder = safestore::SafeStoreWrapper::ObjectHandleHolder(mockGetIdMethods, mockReleaseMethods);
    auto actualFile = quarantineManager->extractQuarantinedFile(std::move(holder));
    EXPECT_FALSE(actualFile.has_value());
    EXPECT_TRUE(appenderContains("Failed to clean up previous unpacked file"));
}

TEST_F(QuarantineManagerTests, extractQuarantinedFileHandlesFailedRestore)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };
    auto* filePermissionsMock = new StrictMock<MockFilePermissions>();
    Tests::ScopedReplaceFilePermissions scopedReplaceFilePermissions{
        std::unique_ptr<Common::FileSystem::IFilePermissions>(filePermissionsMock)
    };

    addCommonPersistValueExpects(*filesystemMock);

    EXPECT_CALL(*filePermissionsMock, chmod("/tmp/av/var/tempUnpack", _)).Times(1);
    EXPECT_CALL(*filePermissionsMock, chown("/tmp/av/var/tempUnpack", "root", "root")).Times(1);
    EXPECT_CALL(*filesystemMock, removeFileOrDirectory("/tmp/av/var/tempUnpack")).Times(1);
    EXPECT_CALL(*filesystemMock, makedirs("/tmp/av/var/tempUnpack")).Times(1);

    std::vector<std::string> unpackedFiles = { "file" };
    EXPECT_CALL(*filesystemMock, listAllFilesInDirectoryTree("/tmp/av/var/tempUnpack")).WillOnce(Return(unpackedFiles));

    EXPECT_CALL(*filesystemMock, removeFile("file")).Times(1);
    auto mockGetIdMethods = std::make_shared<StrictMock<MockISafeStoreGetIdMethods>>();
    auto mockReleaseMethods = std::make_shared<StrictMock<MockISafeStoreReleaseMethods>>();
    EXPECT_CALL(*mockReleaseMethods, releaseObjectHandle(_)).Times(1);

    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectId(_)).Times(1);
    EXPECT_CALL(*m_mockSafeStoreWrapper, restoreObjectByIdToLocation(_, _)).WillOnce(Return(false));

    auto quarantineManager = createQuarantineManager();

    ObjectHandleHolder holder = safestore::SafeStoreWrapper::ObjectHandleHolder(mockGetIdMethods, mockReleaseMethods);
    auto actualFile = quarantineManager->extractQuarantinedFile(std::move(holder));
    EXPECT_FALSE(actualFile.has_value());
    EXPECT_TRUE(appenderContains("Failed to restore threat for rescan"));
}

TEST_F(QuarantineManagerTests, extractQuarantinedFileAbortWhenFailingToChmodFile)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };
    auto* filePermissionsMock = new StrictMock<MockFilePermissions>();
    Tests::ScopedReplaceFilePermissions scopedReplaceFilePermissions{
        std::unique_ptr<Common::FileSystem::IFilePermissions>(filePermissionsMock)
    };

    addCommonPersistValueExpects(*filesystemMock);

    EXPECT_CALL(*filePermissionsMock, chmod("/tmp/av/var/tempUnpack", _)).Times(1);
    EXPECT_CALL(*filePermissionsMock, chown("/tmp/av/var/tempUnpack", "root", "root")).Times(1);
    EXPECT_CALL(*filesystemMock, removeFileOrDirectory("/tmp/av/var/tempUnpack")).Times(1);
    EXPECT_CALL(*filesystemMock, makedirs("/tmp/av/var/tempUnpack")).Times(1);

    std::vector<std::string> unpackedFiles = { "file" };
    EXPECT_CALL(*filesystemMock, listAllFilesInDirectoryTree("/tmp/av/var/tempUnpack")).WillOnce(Return(unpackedFiles));

    EXPECT_CALL(*filePermissionsMock, chmod("file", _))
        .WillOnce(Throw(Common::FileSystem::IFileSystemException("exception")));
    EXPECT_CALL(*filePermissionsMock, chown("file", "root", "root")).Times(1);

    auto mockGetIdMethods = std::make_shared<StrictMock<MockISafeStoreGetIdMethods>>();
    auto mockReleaseMethods = std::make_shared<StrictMock<MockISafeStoreReleaseMethods>>();
    EXPECT_CALL(*mockReleaseMethods, releaseObjectHandle(_)).Times(1);

    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectId(_)).Times(1);
    EXPECT_CALL(*m_mockSafeStoreWrapper, restoreObjectByIdToLocation(_, _)).WillOnce(Return(true));

    auto quarantineManager = createQuarantineManager();

    ObjectHandleHolder holder = safestore::SafeStoreWrapper::ObjectHandleHolder(mockGetIdMethods, mockReleaseMethods);
    auto actualFile = quarantineManager->extractQuarantinedFile(std::move(holder));
    EXPECT_FALSE(actualFile.has_value());
    EXPECT_TRUE(appenderContains("Failed to set correct permissions exception aborting rescan"));
}

TEST_F(QuarantineManagerTests, configParsingCanParseValidConfig)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };
    addCommonPersistValueExpects(*filesystemMock);

    std::string jsonContents =
        R"({ "MaxObjectSize" : 32000, "MaxSafeStoreSize" : 144000, "MaxRegObjectCount" : 50, "AutoPurge" : 0, "MaxStoredObjectCount" : 100 })";

    EXPECT_CALL(*filesystemMock, isFile(Plugin::getSafeStoreConfigPath())).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(Plugin::getSafeStoreConfigPath())).WillOnce(Return(jsonContents));

    EXPECT_CALL(*m_mockSafeStoreWrapper, setConfigIntValue(ConfigOption::MAX_SAFESTORE_SIZE, 144000))
        .WillOnce(Return(true));
    EXPECT_CALL(*m_mockSafeStoreWrapper, setConfigIntValue(ConfigOption::MAX_OBJECT_SIZE, 32000))
        .WillOnce(Return(true));
    EXPECT_CALL(*m_mockSafeStoreWrapper, setConfigIntValue(ConfigOption::MAX_REG_OBJECT_COUNT, 50))
        .WillOnce(Return(true));
    EXPECT_CALL(*m_mockSafeStoreWrapper, setConfigIntValue(ConfigOption::AUTO_PURGE, 0)).WillOnce(Return(true));
    EXPECT_CALL(*m_mockSafeStoreWrapper, setConfigIntValue(ConfigOption::MAX_STORED_OBJECT_COUNT, 100))
        .WillOnce(Return(true));

    auto quarantineManager = createQuarantineManager();
    EXPECT_NO_THROW(quarantineManager->parseConfig());
}

TEST_F(QuarantineManagerTests, configParsingIgnoresInvalidConfigValues)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };
    addCommonPersistValueExpects(*filesystemMock);

    std::string jsonContents =
        R"({ "MaxObjectSize" : -32000, "MaxSafeStoreSize" : "Big", "MaxRegObjectCount" : [ 50, 100 ], "AutoPurge" : true, "MaxStoredObjectCount" : null, "NotAValidKey" : 10 })";

    EXPECT_CALL(*filesystemMock, isFile(Plugin::getSafeStoreConfigPath())).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(Plugin::getSafeStoreConfigPath())).WillOnce(Return(jsonContents));

    auto quarantineManager = createQuarantineManager();
    EXPECT_NO_THROW(quarantineManager->parseConfig());
}

TEST_F(QuarantineManagerTests, configParsingHandlesMalformedJson)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };
    addCommonPersistValueExpects(*filesystemMock);

    std::string jsonContents = R"(Not valid json)";

    EXPECT_CALL(*filesystemMock, isFile(Plugin::getSafeStoreConfigPath())).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(Plugin::getSafeStoreConfigPath())).WillOnce(Return(jsonContents));

    auto quarantineManager = createQuarantineManager();
    EXPECT_NO_THROW(quarantineManager->parseConfig());
    EXPECT_TRUE(appenderContains("Failed to parse SafeStore config json: "));
}

TEST_F(QuarantineManagerTests, configParsingHandlesBadFileRead)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };
    addCommonPersistValueExpects(*filesystemMock);

    EXPECT_CALL(*filesystemMock, isFile(Plugin::getSafeStoreConfigPath())).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(Plugin::getSafeStoreConfigPath()))
        .WillOnce(Throw(IFileSystemException("test error")));

    auto quarantineManager = createQuarantineManager();
    EXPECT_NO_THROW(quarantineManager->parseConfig());
    EXPECT_TRUE(appenderContains("Failed to read SafeStore config json: test error"));
}

class TestQuarantineManagerDefaultMock : public SafeStoreMemoryAppenderUsingTests
{
protected:
    TestQuarantineManagerDefaultMock()
    {
        auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
        appConfig.setData("SOPHOS_INSTALL", "/tmp");
        appConfig.setData("PLUGIN_INSTALL", "/tmp/av");

        ON_CALL(*m_mockFileSystem, getFileInfoDescriptor(m_dir)).WillByDefault(Return(100));
        ON_CALL(*m_mockFileSystem, getFileInfoDescriptorFromDirectoryFD(100, m_file)).WillByDefault(Return(200));
        ON_CALL(*m_mockFileSystem, readlink("/proc/self/fd/200")).WillByDefault(Return(m_path));
        ON_CALL(*m_mockFileSystem, compareFileDescriptors(200, 200)).WillByDefault(Return(true));

        ON_CALL(*m_mockSafeStoreWrapper, initialise).WillByDefault(Return(InitReturnCode::OK));
        ON_CALL(*m_mockSafeStoreWrapper, createObjectHandleHolder)
            .WillByDefault(InvokeWithoutArgs(
                [&]() { return std::make_unique<ObjectHandleHolder>(m_mockGetIdMethods, m_mockReleaseMethods); }));
        ON_CALL(*m_mockSafeStoreWrapper, getObjectHandle).WillByDefault(Return(true));
        ON_CALL(*m_mockSafeStoreWrapper, finaliseObject).WillByDefault(Return(true));
        ON_CALL(*m_mockSafeStoreWrapper, getObjectName).WillByDefault(Return(m_file));
        ON_CALL(*m_mockSafeStoreWrapper, getObjectLocation).WillByDefault(Return(m_dir));

        EXPECT_CALL(*m_mockSafeStoreWrapper, setObjectCustomDataString).Times(AnyNumber()).WillRepeatedly(Return(true));
    }

    QuarantineManagerImpl createQuarantineManager()
    {
        return { std::move(m_mockSafeStoreWrapper), std::move(m_mockSysCallWrapper), mockSafeStoreResources_ };
    }

    std::unique_ptr<MockISafeStoreWrapper> m_mockSafeStoreWrapper = std::make_unique<MockISafeStoreWrapper>();
    std::unique_ptr<MockFileSystem> m_mockFileSystem = std::make_unique<MockFileSystem>();
    std::shared_ptr<MockISafeStoreGetIdMethods> m_mockGetIdMethods = std::make_shared<MockISafeStoreGetIdMethods>();
    std::shared_ptr<MockISafeStoreReleaseMethods> m_mockReleaseMethods =
        std::make_shared<MockISafeStoreReleaseMethods>();
    std::shared_ptr<MockSystemCallWrapper> m_mockSysCallWrapper = std::make_shared<MockSystemCallWrapper>();
    MockSafeStoreResources mockSafeStoreResources_;

    // Common test constants
    const std::string m_dir = "/dir/";
    const std::string m_file = "file";
    const std::string m_path = "/dir/file";
    const std::string m_threatID = "01234567-89ab-cdef-0123-456789abcdef";
    const std::string m_threatType = "threatType";
    const std::string m_threatName = "threatName";
    const std::string threatSha256_ = "threatSHA256";
    const std::string m_SHA256 = "SHA256abcdef";
    const std::string m_correlationId = "fedcba98-7654-3210-fedc-ba9876543210";
    datatypes::AutoFd autoFd{ 200 };
};

TEST_F(TestQuarantineManagerDefaultMock, QuarantineFileFailsWhenSettingCorrelationIdCustomStringFails)
{
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(m_mockFileSystem) };

    EXPECT_CALL(*m_mockSafeStoreWrapper, setObjectCustomDataString(_, "correlationId", _))
        .Times(1)
        .WillOnce(Return(false));

    UsingMemoryAppender memoryAppenderHolder{ *this };

    auto quarantineManager = createQuarantineManager();
    quarantineManager.initialise();
    const auto result = quarantineManager.quarantineFile(
        m_path, m_threatID, m_threatType, m_threatName, threatSha256_, m_SHA256, m_correlationId, std::move(autoFd));
    EXPECT_EQ(result, common::CentralEnums::QuarantineResult::FAILED_TO_DELETE_FILE);

    EXPECT_TRUE(appenderContains("Failed to store correlation ID"));
}

TEST_F(TestQuarantineManagerDefaultMock, QuarantineFileFailsWhenSettingCorrelationIdCustomStringThrows)
{
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(m_mockFileSystem) };

    EXPECT_CALL(*m_mockSafeStoreWrapper, setObjectCustomDataString(_, "correlationId", _))
        .Times(1)
        .WillOnce(Throw(std::runtime_error("failure")));

    UsingMemoryAppender memoryAppenderHolder{ *this };

    auto quarantineManager = createQuarantineManager();
    quarantineManager.initialise();
    const auto result = quarantineManager.quarantineFile(
        m_path, m_threatID, m_threatType, m_threatName, threatSha256_, m_SHA256, m_correlationId, std::move(autoFd));
    EXPECT_EQ(result, common::CentralEnums::QuarantineResult::FAILED_TO_DELETE_FILE);

    EXPECT_TRUE(appenderContains("Failed to store correlation ID"));
}

TEST_F(TestQuarantineManagerDefaultMock, QuarantineFileFailsWhenPassedInvalidCorrelationId)
{
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(m_mockFileSystem) };

    UsingMemoryAppender memoryAppenderHolder{ *this };

    auto quarantineManager = createQuarantineManager();
    quarantineManager.initialise();
    const auto result = quarantineManager.quarantineFile(
        m_path, m_threatID, m_threatType, m_threatName, threatSha256_, m_SHA256, "invalid uuid", std::move(autoFd));
    EXPECT_EQ(result, common::CentralEnums::QuarantineResult::FAILED_TO_DELETE_FILE);

    EXPECT_TRUE(appenderContains("Invalid correlation ID"));
}

TEST_F(TestQuarantineManagerDefaultMock, RestoreFailsWhenGettingCorrelationIdReturnsEmpty)
{
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(m_mockFileSystem) };

    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectCustomDataString(_, "correlationId")).WillOnce(Return(""));

    UsingMemoryAppender memoryAppenderHolder{ *this };

    auto quarantineManager = createQuarantineManager();
    quarantineManager.initialise();
    const auto result = quarantineManager.restoreFile("00000000-0000-0000-0000-000000000000");
    EXPECT_FALSE(result.has_value());

    EXPECT_TRUE(appenderContains("couldn't get correlation ID"));
}

TEST_F(TestQuarantineManagerDefaultMock, RestoreFailsWhenGettingCorrelationIdReturnsInvalidUuid)
{
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(m_mockFileSystem) };

    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectCustomDataString(_, "correlationId"))
        .WillOnce(Return("invalid uuid"));

    UsingMemoryAppender memoryAppenderHolder{ *this };

    auto quarantineManager = createQuarantineManager();
    quarantineManager.initialise();
    const auto result = quarantineManager.restoreFile("00000000-0000-0000-0000-000000000000");
    EXPECT_FALSE(result.has_value());

    EXPECT_TRUE(appenderContains("couldn't get correlation ID"));
}

TEST_F(TestQuarantineManagerDefaultMock, RestoreFailsWhenGettingCorrelationIdThrows)
{
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(m_mockFileSystem) };

    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectCustomDataString(_, "correlationId"))
        .WillOnce(Throw(std::runtime_error("failure")));

    UsingMemoryAppender memoryAppenderHolder{ *this };

    auto quarantineManager = createQuarantineManager();
    quarantineManager.initialise();
    const auto result = quarantineManager.restoreFile("00000000-0000-0000-0000-000000000000");
    EXPECT_FALSE(result.has_value());

    EXPECT_TRUE(appenderContains("couldn't get correlation ID"));
}

TEST_F(TestQuarantineManagerDefaultMock, RestoreSucceedsOnRootFile)
{
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(m_mockFileSystem) };

    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectCustomDataString(_, "correlationId"))
        .WillOnce(Return(m_correlationId));
    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectLocation(_)).WillOnce(Return("/"));
    EXPECT_CALL(*m_mockSafeStoreWrapper, restoreObjectById(_)).WillOnce(Return(true));
    EXPECT_CALL(*m_mockSafeStoreWrapper, deleteObjectById(_)).WillOnce(Return(true));

    UsingMemoryAppender memoryAppenderHolder{ *this };

    auto quarantineManager = createQuarantineManager();
    quarantineManager.initialise();
    const auto result = quarantineManager.restoreFile("00000000-0000-0000-0000-000000000000");
    EXPECT_TRUE(result.has_value());

    EXPECT_TRUE(appenderContains("Restored file to disk: /file"));
}

TEST_F(TestQuarantineManagerDefaultMock, QuarantineFileSetsThreatsInSafeStoreObjectFromArguments)
{
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(m_mockFileSystem) };

    EXPECT_CALL(
        *m_mockSafeStoreWrapper,
        setObjectCustomDataString(
            _, "threats", R"([{"name":"threatName","sha256":"threatSHA256","type":"threatType"}])"))
        .Times(1)
        .WillOnce(Return(true));
    auto quarantineManager = createQuarantineManager();

    quarantineManager.initialise();
    const auto result = quarantineManager.quarantineFile(
        m_path, m_threatID, "threatType", "threatName", "threatSHA256", m_SHA256, m_correlationId, std::move(autoFd));
    EXPECT_EQ(result, common::CentralEnums::QuarantineResult::SUCCESS);
}

TEST_F(TestQuarantineManagerDefaultMock, QuarantineFileSetsThreatsWhenThreatNameIsAValidJson)
{
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(m_mockFileSystem) };

    EXPECT_CALL(
        *m_mockSafeStoreWrapper,
        setObjectCustomDataString(
            _, "threats", R"([{"name":"{\"a\": \"b\"}","sha256":"threatSHA256","type":"threatType"}])"))
        .Times(1)
        .WillOnce(Return(true));
    auto quarantineManager = createQuarantineManager();

    quarantineManager.initialise();
    const auto result = quarantineManager.quarantineFile(
        m_path, m_threatID, "threatType", R"({"a": "b"})", "threatSHA256", m_SHA256, m_correlationId, std::move(autoFd));
    EXPECT_EQ(result, common::CentralEnums::QuarantineResult::SUCCESS);
}

TEST_F(TestQuarantineManagerDefaultMock, quarantineFileSettingThreatsFailsDoesntCauseQuarantineFailure)
{
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(m_mockFileSystem) };

    EXPECT_CALL(*m_mockSafeStoreWrapper, setObjectCustomDataString(_, "threats", _))
        .Times(AtLeast(1))
        .WillRepeatedly(Return(false));
    auto quarantineManager = createQuarantineManager();

    quarantineManager.initialise();
    const auto result = quarantineManager.quarantineFile(
        m_path, m_threatID, m_threatType, m_threatName, threatSha256_, m_SHA256, m_correlationId, std::move(autoFd));
    EXPECT_EQ(result, common::CentralEnums::QuarantineResult::SUCCESS);
}

TEST_F(TestQuarantineManagerDefaultMock, QuarantineFileSetsSha256InSafeStoreObject)
{
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(m_mockFileSystem) };

    EXPECT_CALL(*m_mockSafeStoreWrapper, setObjectCustomDataString(_, "SHA256", "SHA256")).WillOnce(Return(true));
    auto quarantineManager = createQuarantineManager();

    quarantineManager.initialise();
    const auto result = quarantineManager.quarantineFile(
        m_path, m_threatID, m_threatType, m_threatName, threatSha256_, "SHA256", m_correlationId, std::move(autoFd));
    EXPECT_EQ(result, common::CentralEnums::QuarantineResult::SUCCESS);
}

TEST_F(TestQuarantineManagerDefaultMock, QuarantineFileSettingSha256FailsDoesntCauseQuarantineFailure)
{
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(m_mockFileSystem) };

    EXPECT_CALL(*m_mockSafeStoreWrapper, setObjectCustomDataString(_, "SHA256", _))
        .Times(AtLeast(1))
        .WillRepeatedly(Return(false));
    auto quarantineManager = createQuarantineManager();

    quarantineManager.initialise();
    const auto result = quarantineManager.quarantineFile(
        m_path, m_threatID, m_threatType, m_threatName, threatSha256_, m_SHA256, m_correlationId, std::move(autoFd));
    EXPECT_EQ(result, common::CentralEnums::QuarantineResult::SUCCESS);
}

TEST_F(QuarantineManagerTests, QuarantineFileFailsIfReadlinkFails)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };
    addCommonPersistValueExpects(*filesystemMock);
    EXPECT_CALL(*filesystemMock, removeFile(Plugin::getSafeStoreDormantFlagPath(), true)).WillOnce(Return()); // 1
    EXPECT_CALL(*filesystemMock, isFile(Plugin::getSafeStorePasswordFilePath())).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(Plugin::getSafeStorePasswordFilePath())).WillOnce(Return("a password"));
    EXPECT_CALL(*filesystemMock, isFile(Plugin::getSafeStoreConfigPath())).WillOnce(Return(false));
    EXPECT_CALL(*filesystemMock, removeFilesInDirectory("/tmp/av/var/tempUnpack")).Times(1);

    EXPECT_CALL(*filesystemMock, getFileInfoDescriptor(_)).WillOnce(Return(100));

    EXPECT_CALL(
        *m_mockSafeStoreWrapper,
        initialise(Plugin::getSafeStoreDbDirPath(), Plugin::getSafeStoreDbFileName(), "a password"))
        .WillOnce(Return(InitReturnCode::OK));

    EXPECT_CALL(*filesystemMock, readlink).Times(1).WillOnce(Return(std::nullopt));

    auto quarantineManager = createQuarantineManager();

    EXPECT_NO_THROW(quarantineManager->initialise());
    ASSERT_EQ(quarantineManager->getState(), QuarantineManagerState::INITIALISED);
    datatypes::AutoFd fdHolder;
    EXPECT_EQ(
        common::CentralEnums::QuarantineResult::FAILED_TO_DELETE_FILE,
        quarantineManager->quarantineFile(
            m_path,
            m_threatID,
            m_threatType,
            m_threatName,
            threatSha256_,
            m_SHA256,
            m_correlationId,
            std::move(fdHolder)));
}

TEST_F(QuarantineManagerTests, QuarantineSucceedsIfFileDeleted)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };
    addCommonPersistValueExpects(*filesystemMock);
    EXPECT_CALL(*filesystemMock, removeFile(Plugin::getSafeStoreDormantFlagPath(), true)).WillOnce(Return()); // 1
    EXPECT_CALL(*filesystemMock, isFile(Plugin::getSafeStorePasswordFilePath())).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(Plugin::getSafeStorePasswordFilePath())).WillOnce(Return("a password"));
    EXPECT_CALL(*filesystemMock, isFile(Plugin::getSafeStoreConfigPath())).WillOnce(Return(false));
    EXPECT_CALL(*filesystemMock, removeFilesInDirectory("/tmp/av/var/tempUnpack")).Times(1);

    EXPECT_CALL(*filesystemMock, getFileInfoDescriptor(_)).WillOnce(Return(100));

    EXPECT_CALL(
        *m_mockSafeStoreWrapper,
        initialise(Plugin::getSafeStoreDbDirPath(), Plugin::getSafeStoreDbFileName(), "a password"))
        .WillOnce(Return(InitReturnCode::OK));

    EXPECT_CALL(*filesystemMock, readlink).Times(1).WillOnce(Return(m_path + " (deleted)"));
    EXPECT_CALL(*filesystemMock, getFileInfoDescriptorFromDirectoryFD(_, m_file + " (deleted)")).WillOnce(Return(-1));

    auto quarantineManager = createQuarantineManager();

    EXPECT_NO_THROW(quarantineManager->initialise());
    ASSERT_EQ(quarantineManager->getState(), QuarantineManagerState::INITIALISED);
    datatypes::AutoFd fdHolder{ 200 };
    EXPECT_EQ(
        common::CentralEnums::QuarantineResult::SUCCESS,
        quarantineManager->quarantineFile(
            m_path,
            m_threatID,
            m_threatType,
            m_threatName,
            threatSha256_,
            m_SHA256,
            m_correlationId,
            std::move(fdHolder)));
}

TEST_F(QuarantineManagerTests, QuarantineFailsIfFileRenamedToHaveDeletedAtEnd)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };
    addCommonPersistValueExpects(*filesystemMock);
    EXPECT_CALL(*filesystemMock, removeFile(Plugin::getSafeStoreDormantFlagPath(), true)).WillOnce(Return()); // 1
    EXPECT_CALL(*filesystemMock, isFile(Plugin::getSafeStorePasswordFilePath())).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(Plugin::getSafeStorePasswordFilePath())).WillOnce(Return("a password"));
    EXPECT_CALL(*filesystemMock, isFile(Plugin::getSafeStoreConfigPath())).WillOnce(Return(false));
    EXPECT_CALL(*filesystemMock, removeFilesInDirectory("/tmp/av/var/tempUnpack")).Times(1);

    EXPECT_CALL(*filesystemMock, getFileInfoDescriptor(_)).WillOnce(Return(100));

    EXPECT_CALL(
        *m_mockSafeStoreWrapper,
        initialise(Plugin::getSafeStoreDbDirPath(), Plugin::getSafeStoreDbFileName(), "a password"))
        .WillOnce(Return(InitReturnCode::OK));

    EXPECT_CALL(*filesystemMock, readlink).Times(1).WillOnce(Return(m_path + " (deleted)"));
    EXPECT_CALL(*filesystemMock, getFileInfoDescriptorFromDirectoryFD(_, m_file + " (deleted)")).WillOnce(Return(101));
    EXPECT_CALL(*filesystemMock, compareFileDescriptors(101, 100)).WillOnce(Return(true));

    auto quarantineManager = createQuarantineManager();

    EXPECT_NO_THROW(quarantineManager->initialise());
    ASSERT_EQ(quarantineManager->getState(), QuarantineManagerState::INITIALISED);
    datatypes::AutoFd fdHolder{ 100 };
    EXPECT_EQ(
        common::CentralEnums::QuarantineResult::NOT_FOUND,
        quarantineManager->quarantineFile(
            m_path,
            m_threatID,
            m_threatType,
            m_threatName,
            threatSha256_,
            m_SHA256,
            m_correlationId,
            std::move(fdHolder)));
}

TEST_F(QuarantineManagerTests, QuarantineFailsIfFileMoved)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };
    addCommonPersistValueExpects(*filesystemMock);
    EXPECT_CALL(*filesystemMock, removeFile(Plugin::getSafeStoreDormantFlagPath(), true)).WillOnce(Return()); // 1
    EXPECT_CALL(*filesystemMock, isFile(Plugin::getSafeStorePasswordFilePath())).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(Plugin::getSafeStorePasswordFilePath())).WillOnce(Return("a password"));
    EXPECT_CALL(*filesystemMock, isFile(Plugin::getSafeStoreConfigPath())).WillOnce(Return(false));
    EXPECT_CALL(*filesystemMock, removeFilesInDirectory("/tmp/av/var/tempUnpack")).Times(1);

    EXPECT_CALL(*filesystemMock, getFileInfoDescriptor(_)).WillOnce(Return(100));

    EXPECT_CALL(
        *m_mockSafeStoreWrapper,
        initialise(Plugin::getSafeStoreDbDirPath(), Plugin::getSafeStoreDbFileName(), "a password"))
        .WillOnce(Return(InitReturnCode::OK));

    EXPECT_CALL(*filesystemMock, readlink).Times(1).WillOnce(Return(m_path + "_some_other_path"));

    auto quarantineManager = createQuarantineManager();

    EXPECT_NO_THROW(quarantineManager->initialise());
    ASSERT_EQ(quarantineManager->getState(), QuarantineManagerState::INITIALISED);
    datatypes::AutoFd fdHolder;
    EXPECT_EQ(
        common::CentralEnums::QuarantineResult::NOT_FOUND,
        quarantineManager->quarantineFile(
            m_path,
            m_threatID,
            m_threatType,
            m_threatName,
            threatSha256_,
            m_SHA256,
            m_correlationId,
            std::move(fdHolder)));
}

TEST_F(QuarantineManagerTests, waitForFilesystemLockReturnsWhenLockDoesNotExist)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };

    addCommonPersistValueExpects(*filesystemMock);
    auto quarantineManager = createQuarantineManager();
    EXPECT_CALL(*filesystemMock, exists(Plugin::getSafeStoreDbLockDirPath())).WillOnce(Return(false));
    EXPECT_TRUE(quarantineManager->waitForFilesystemLock(1));
}

TEST_F(QuarantineManagerTests, waitForFilesystemLockWaitsWhenLockDoesExist)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };

    addCommonPersistValueExpects(*filesystemMock);
    auto quarantineManager = createQuarantineManager();

    EXPECT_CALL(*filesystemMock, exists(Plugin::getSafeStoreDbLockDirPath()))
        .WillOnce(Return(true)) // cause a wait to happen
        .WillOnce(Return(false));
    EXPECT_TRUE(quarantineManager->waitForFilesystemLock(1));
}

TEST_F(QuarantineManagerTests, removeFilesystemLockRemovesDir)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };

    addCommonPersistValueExpects(*filesystemMock);
    auto quarantineManager = createQuarantineManager();

    EXPECT_CALL(*filesystemMock, removeFileOrDirectory(Plugin::getSafeStoreDbLockDirPath()));
    quarantineManager->removeFilesystemLock();
}

TEST_F(QuarantineManagerTests, removeFilesystemLockHandlesFilesystemException)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };

    addCommonPersistValueExpects(*filesystemMock);
    auto quarantineManager = createQuarantineManager();

    EXPECT_CALL(*filesystemMock, removeFileOrDirectory(Plugin::getSafeStoreDbLockDirPath()))
        .WillOnce(Throw(Common::FileSystem::IFileSystemException("Test exception")));
    EXPECT_NO_THROW(quarantineManager->removeFilesystemLock());
}
