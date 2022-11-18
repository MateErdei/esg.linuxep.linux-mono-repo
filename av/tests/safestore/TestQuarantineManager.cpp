// Copyright 2022, Sophos Limited. All rights reserved.

#include "MockISafeStoreWrapper.h"

#include "../common/LogInitializedTests.h"
#include "safestore/QuarantineManager/IQuarantineManager.h"
#include "safestore/QuarantineManager/QuarantineManagerImpl.h"
#include "scan_messages/QuarantineResponse.h"

#include "common/ApplicationPaths.h"

#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>
#include <Common/FileSystem/IFileSystem.h>
#include <Common/FileSystem/IFileSystemException.h>
#include <Common/Helpers/FileSystemReplaceAndRestore.h>
#include <Common/Helpers/MockFileSystem.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;
using namespace safestore::QuarantineManager;

class QuarantineManagerTests : public LogInitializedTests
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

    std::unique_ptr<StrictMock<MockISafeStoreWrapper>> m_mockSafeStoreWrapper;

    // Common test constants
    inline static const std::string m_dir = "/dir";
    inline static const std::string m_file = "file";
    inline static const std::string m_threatID = "01234567-89ab-cdef-0123-456789abcdef";
    inline static const std::string m_threatName = "threatName";
    inline static const std::string m_SHA256 = "SHA256abcdef";
};

TEST_F(QuarantineManagerTests, initDbWithExistingPassword)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };

    addCommonPersistValueExpects(*filesystemMock);

    EXPECT_CALL(*filesystemMock, removeFile("/tmp/av/var/safestore_dormant_flag", true)).WillOnce(Return());
    EXPECT_CALL(*filesystemMock, isFile("/tmp/av/var/safestore_db/safestore.pw")).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile("/tmp/av/var/safestore_db/safestore.pw")).WillOnce(Return("a password"));

    EXPECT_CALL(*m_mockSafeStoreWrapper, initialise("/tmp/av/var/safestore_db", "safestore.db", "a password"))
        .WillOnce(Return(InitReturnCode::OK));
    std::shared_ptr<IQuarantineManager> quarantineManager =
        std::make_shared<QuarantineManagerImpl>(std::move(m_mockSafeStoreWrapper));

    EXPECT_NO_THROW(quarantineManager->initialise());
}

TEST_F(QuarantineManagerTests, initDbAndGeneratePassword)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };
    addCommonPersistValueExpects(*filesystemMock);
    EXPECT_CALL(*filesystemMock, removeFile("/tmp/av/var/safestore_dormant_flag", true)).WillOnce(Return());
    EXPECT_CALL(*filesystemMock, isFile("/tmp/av/var/safestore_db/safestore.pw")).WillOnce(Return(false));
    EXPECT_CALL(*filesystemMock, readFile("/tmp/av/var/safestore_db/safestore.pw")).Times(0);
    EXPECT_CALL(*filesystemMock, isDirectory("/tmp/av/var/safestore_db")).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, writeFile("/tmp/av/var/safestore_db/safestore.pw", _)).Times(1);

    EXPECT_CALL(*m_mockSafeStoreWrapper, initialise("/tmp/av/var/safestore_db", "safestore.db", _))
        .WillOnce(Return(InitReturnCode::OK));
    std::shared_ptr<IQuarantineManager> quarantineManager =
        std::make_shared<QuarantineManagerImpl>(std::move(m_mockSafeStoreWrapper));

    EXPECT_NO_THROW(quarantineManager->initialise());
}

TEST_F(QuarantineManagerTests, uninitialisedDbIsInitialisedAfterSuccessfulInitCall)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };
    addCommonPersistValueExpects(*filesystemMock);
    EXPECT_CALL(*filesystemMock, removeFile("/tmp/av/var/safestore_dormant_flag", true)).WillOnce(Return());
    EXPECT_CALL(*filesystemMock, isFile("/tmp/av/var/safestore_db/safestore.pw")).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile("/tmp/av/var/safestore_db/safestore.pw")).WillOnce(Return("a password"));

    EXPECT_CALL(*m_mockSafeStoreWrapper, initialise("/tmp/av/var/safestore_db", "safestore.db", "a password"))
        .WillOnce(Return(InitReturnCode::OK));
    std::shared_ptr<IQuarantineManager> quarantineManager =
        std::make_shared<QuarantineManagerImpl>(std::move(m_mockSafeStoreWrapper));

    ASSERT_EQ(quarantineManager->getState(), QuarantineManagerState::STARTUP);
    EXPECT_NO_THROW(quarantineManager->initialise());
    ASSERT_EQ(quarantineManager->getState(), QuarantineManagerState::INITIALISED);
}

TEST_F(QuarantineManagerTests, uninitialisedDbStateIsStillUnitialisedAfterFailedInitCall)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };
    addCommonPersistValueExpects(*filesystemMock);
    EXPECT_CALL(*filesystemMock, isFile("/tmp/av/var/safestore_db/safestore.pw")).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile("/tmp/av/var/safestore_db/safestore.pw")).WillOnce(Return("a password"));
    EXPECT_CALL(
        *filesystemMock,
        writeFileAtomically("/tmp/av/var/safestore_dormant_flag", "SafeStore database uninitialised", "/tmp/tmp"));

    EXPECT_CALL(*m_mockSafeStoreWrapper, initialise("/tmp/av/var/safestore_db", "safestore.db", "a password"))
        .WillOnce(Return(InitReturnCode::DB_OPEN_FAILED));
    std::shared_ptr<IQuarantineManager> quarantineManager =
        std::make_shared<QuarantineManagerImpl>(std::move(m_mockSafeStoreWrapper));

    ASSERT_EQ(quarantineManager->getState(), QuarantineManagerState::STARTUP);
    EXPECT_NO_THROW(quarantineManager->initialise());
    ASSERT_EQ(quarantineManager->getState(), QuarantineManagerState::UNINITIALISED);
}

TEST_F(QuarantineManagerTests, initFailsOnDBErrorAndDbIsMarkedCorrupt)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };
    addCommonPersistValueExpects(*filesystemMock);
    EXPECT_CALL(*filesystemMock, isFile("/tmp/av/var/safestore_db/safestore.pw")).WillRepeatedly(Return(true));
    EXPECT_CALL(*filesystemMock, readFile("/tmp/av/var/safestore_db/safestore.pw"))
        .WillRepeatedly(Return("a password"));

    EXPECT_CALL(*m_mockSafeStoreWrapper, initialise("/tmp/av/var/safestore_db", "safestore.db", "a password"))
        .WillRepeatedly(Return(InitReturnCode::DB_ERROR));
    std::shared_ptr<IQuarantineManager> quarantineManager =
        std::make_shared<QuarantineManagerImpl>(std::move(m_mockSafeStoreWrapper));

    EXPECT_CALL(
        *filesystemMock,
        writeFileAtomically("/tmp/av/var/safestore_dormant_flag", "SafeStore database uninitialised", "/tmp/tmp"));
    EXPECT_CALL(
        *filesystemMock,
        writeFileAtomically("/tmp/av/var/safestore_dormant_flag", "SafeStore database corrupt", "/tmp/tmp"));
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
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };
    addCommonPersistValueExpects(*filesystemMock);
    EXPECT_CALL(
        *filesystemMock,
        writeFileAtomically("/tmp/av/var/safestore_dormant_flag", "SafeStore database uninitialised", "/tmp/tmp"));
    EXPECT_CALL(
        *filesystemMock,
        writeFileAtomically("/tmp/av/var/safestore_dormant_flag", "SafeStore database corrupt", "/tmp/tmp"));
    EXPECT_CALL(*filesystemMock, isFile("/tmp/av/var/safestore_db/safestore.pw")).WillRepeatedly(Return(true));
    EXPECT_CALL(*filesystemMock, readFile("/tmp/av/var/safestore_db/safestore.pw"))
        .WillRepeatedly(Return("a password"));

    EXPECT_CALL(*m_mockSafeStoreWrapper, initialise("/tmp/av/var/safestore_db", "safestore.db", "a password"))
        .WillRepeatedly(Return(InitReturnCode::DB_OPEN_FAILED));
    std::shared_ptr<IQuarantineManager> quarantineManager =
        std::make_shared<QuarantineManagerImpl>(std::move(m_mockSafeStoreWrapper));

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
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };
    addCommonPersistValueExpects(*filesystemMock);
    EXPECT_CALL(*filesystemMock, removeFile("/tmp/av/var/safestore_dormant_flag", true)).WillOnce(Return()); // 1
    EXPECT_CALL(*filesystemMock, isFile("/tmp/av/var/safestore_db/safestore.pw")).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile("/tmp/av/var/safestore_db/safestore.pw")).WillOnce(Return("a password"));
    EXPECT_CALL(*filesystemMock, compareFileDescriptors(_, _)).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, removeFileOrDirectory("/dir/file"));
    EXPECT_CALL(*filesystemMock, getFileInfoDescriptor(_)).WillOnce(Return(100));
    EXPECT_CALL(*filesystemMock, getFileInfoDescriptorFromDirectoryFD(_, _)).WillOnce(Return(120));

    EXPECT_CALL(*m_mockSafeStoreWrapper, initialise("/tmp/av/var/safestore_db", "safestore.db", "a password"))
        .WillOnce(Return(InitReturnCode::OK));

    auto mockGetIdMethods = std::make_shared<StrictMock<MockISafeStoreGetIdMethods>>();
    auto mockReleaseMethods = std::make_shared<StrictMock<MockISafeStoreReleaseMethods>>();

    void* rawHandle1 = reinterpret_cast<SafeStoreObjectHandle>(1111);
    auto objectHandle1 = std::make_unique<ObjectHandleHolder>(mockGetIdMethods, mockReleaseMethods);
    *objectHandle1->getRawHandlePtr() = rawHandle1;
    auto handleAsArg1 = Property(&ObjectHandleHolder::getRawHandle, rawHandle1); // Matches argument with raw handle
    EXPECT_CALL(*mockReleaseMethods, releaseObjectHandle(rawHandle1));

    void* rawHandle2 = reinterpret_cast<SafeStoreObjectHandle>(2222);
    ObjectHandleHolder objectHandle2 { mockGetIdMethods, mockReleaseMethods };
    *objectHandle2.getRawHandlePtr() = rawHandle2;
    auto handleAsArg2 = Property(&ObjectHandleHolder::getRawHandle, rawHandle2); // Matches argument with raw handle
    EXPECT_CALL(*mockReleaseMethods, releaseObjectHandle(rawHandle2));

    EXPECT_CALL(*m_mockSafeStoreWrapper, createObjectHandleHolder()).WillOnce(Return(ByMove(std::move(objectHandle1))));

    EXPECT_CALL(*m_mockSafeStoreWrapper, saveFile(m_dir, m_file, m_threatID, m_threatName, handleAsArg1))
        .WillOnce(Return(SaveFileReturnCode::OK));
    EXPECT_CALL(*m_mockSafeStoreWrapper, setObjectCustomDataString(handleAsArg1, "SHA256", m_SHA256))
        .WillOnce(Return(true));
    EXPECT_CALL(*m_mockSafeStoreWrapper, finaliseObject(handleAsArg1)).WillOnce(Return(true));

    std::vector<ObjectHandleHolder> searchResults;
    searchResults.push_back(std::move(objectHandle2));
    EXPECT_CALL(*m_mockSafeStoreWrapper, find(SafeStoreFilter { m_threatID, {}, {}, {}, ObjectType::FILE, {}, {}, {} }))
        .WillOnce(Return(ByMove(std::move(searchResults))));

    std::shared_ptr<IQuarantineManager> quarantineManager =
        std::make_shared<QuarantineManagerImpl>(std::move(m_mockSafeStoreWrapper));

    EXPECT_NO_THROW(quarantineManager->initialise());
    ASSERT_EQ(quarantineManager->getState(), QuarantineManagerState::INITIALISED);
    datatypes::AutoFd fdHolder;
    ASSERT_EQ(
        common::CentralEnums::QuarantineResult::SUCCESS,
        quarantineManager->quarantineFile(
            m_dir + "/" + m_file, m_threatID, m_threatName, m_SHA256, std::move(fdHolder)));
}

TEST_F(QuarantineManagerTests, quarantineFileFailsWhenFileDescriptorsDoNotMatch)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };
    addCommonPersistValueExpects(*filesystemMock);
    EXPECT_CALL(*filesystemMock, removeFile("/tmp/av/var/safestore_dormant_flag", true)).WillOnce(Return());
    EXPECT_CALL(*filesystemMock, isFile("/tmp/av/var/safestore_db/safestore.pw")).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile("/tmp/av/var/safestore_db/safestore.pw")).WillOnce(Return("a password"));
    EXPECT_CALL(*filesystemMock, compareFileDescriptors(_, _)).WillOnce(Return(false));
    EXPECT_CALL(*filesystemMock, getFileInfoDescriptor(_)).WillOnce(Return(100));
    EXPECT_CALL(*filesystemMock, getFileInfoDescriptorFromDirectoryFD(_, _)).WillOnce(Return(120));

    auto mockSafeStoreWrapper = std::make_unique<StrictMock<MockISafeStoreWrapper>>();
    EXPECT_CALL(*mockSafeStoreWrapper, initialise("/tmp/av/var/safestore_db", "safestore.db", "a password"))
        .WillOnce(Return(InitReturnCode::OK));
    auto mockGetIdMethods = std::make_shared<StrictMock<MockISafeStoreGetIdMethods>>();
    auto mockReleaseMethods = std::make_shared<StrictMock<MockISafeStoreReleaseMethods>>();
    EXPECT_CALL(*mockReleaseMethods, releaseObjectHandle(_));
    EXPECT_CALL(*mockSafeStoreWrapper, createObjectHandleHolder())
        .WillOnce(Return(ByMove(std::make_unique<ObjectHandleHolder>(mockGetIdMethods, mockReleaseMethods))));

    EXPECT_CALL(*mockSafeStoreWrapper, saveFile(m_dir, m_file, m_threatID, m_threatName, _))
        .WillOnce(Return(SaveFileReturnCode::OK));

    std::shared_ptr<IQuarantineManager> quarantineManager =
        std::make_shared<QuarantineManagerImpl>(std::move(mockSafeStoreWrapper));

    EXPECT_NO_THROW(quarantineManager->initialise());
    ASSERT_EQ(quarantineManager->getState(), QuarantineManagerState::INITIALISED);
    datatypes::AutoFd fdHolder;
    ASSERT_EQ(
        common::CentralEnums::QuarantineResult::FAILED_TO_DELETE_FILE,
        quarantineManager->quarantineFile(
            m_dir + "/" + m_file, m_threatID, m_threatName, m_SHA256, std::move(fdHolder)));
}

TEST_F(QuarantineManagerTests, quarantineFileFailsWhenThreatDirectoryDoesNotExist)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };
    addCommonPersistValueExpects(*filesystemMock);
    EXPECT_CALL(*filesystemMock, removeFile("/tmp/av/var/safestore_dormant_flag", true)).WillOnce(Return());
    EXPECT_CALL(*filesystemMock, isFile("/tmp/av/var/safestore_db/safestore.pw")).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile("/tmp/av/var/safestore_db/safestore.pw")).WillOnce(Return("a password"));
    EXPECT_CALL(*filesystemMock, getFileInfoDescriptor(_)).WillOnce(Return(-1));

    auto mockSafeStoreWrapper = std::make_unique<StrictMock<MockISafeStoreWrapper>>();
    EXPECT_CALL(*mockSafeStoreWrapper, initialise("/tmp/av/var/safestore_db", "safestore.db", "a password"))
        .WillOnce(Return(InitReturnCode::OK));
    auto mockGetIdMethods = std::make_shared<StrictMock<MockISafeStoreGetIdMethods>>();
    auto mockReleaseMethods = std::make_shared<StrictMock<MockISafeStoreReleaseMethods>>();
    EXPECT_CALL(*mockReleaseMethods, releaseObjectHandle(_));
    EXPECT_CALL(*mockSafeStoreWrapper, createObjectHandleHolder())
        .WillOnce(Return(ByMove(std::make_unique<ObjectHandleHolder>(mockGetIdMethods, mockReleaseMethods))));

    EXPECT_CALL(*mockSafeStoreWrapper, saveFile(m_dir, m_file, m_threatID, m_threatName, _))
        .WillOnce(Return(SaveFileReturnCode::OK));

    std::shared_ptr<IQuarantineManager> quarantineManager =
        std::make_shared<QuarantineManagerImpl>(std::move(mockSafeStoreWrapper));

    EXPECT_NO_THROW(quarantineManager->initialise());
    ASSERT_EQ(quarantineManager->getState(), QuarantineManagerState::INITIALISED);
    datatypes::AutoFd fdHolder;
    ASSERT_EQ(
        common::CentralEnums::QuarantineResult::NOT_FOUND,
        quarantineManager->quarantineFile(
            m_dir + "/" + m_file, m_threatID, m_threatName, m_SHA256, std::move(fdHolder)));
}

TEST_F(QuarantineManagerTests, quarantineFileFailsWhenthreatDoesNotExist)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };
    addCommonPersistValueExpects(*filesystemMock);
    EXPECT_CALL(*filesystemMock, removeFile("/tmp/av/var/safestore_dormant_flag", true)).WillOnce(Return());
    EXPECT_CALL(*filesystemMock, isFile("/tmp/av/var/safestore_db/safestore.pw")).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile("/tmp/av/var/safestore_db/safestore.pw")).WillOnce(Return("a password"));
    EXPECT_CALL(*filesystemMock, getFileInfoDescriptor(_)).WillOnce(Return(100));
    EXPECT_CALL(*filesystemMock, getFileInfoDescriptorFromDirectoryFD(_, _)).WillOnce(Return(-1));

    auto mockSafeStoreWrapper = std::make_unique<StrictMock<MockISafeStoreWrapper>>();
    EXPECT_CALL(*mockSafeStoreWrapper, initialise("/tmp/av/var/safestore_db", "safestore.db", "a password"))
        .WillOnce(Return(InitReturnCode::OK));
    auto mockGetIdMethods = std::make_shared<StrictMock<MockISafeStoreGetIdMethods>>();
    auto mockReleaseMethods = std::make_shared<StrictMock<MockISafeStoreReleaseMethods>>();
    EXPECT_CALL(*mockReleaseMethods, releaseObjectHandle(_));
    EXPECT_CALL(*mockSafeStoreWrapper, createObjectHandleHolder())
        .WillOnce(Return(ByMove(std::make_unique<ObjectHandleHolder>(mockGetIdMethods, mockReleaseMethods))));

    EXPECT_CALL(*mockSafeStoreWrapper, saveFile(m_dir, m_file, m_threatID, m_threatName, _))
        .WillOnce(Return(SaveFileReturnCode::OK));

    std::shared_ptr<IQuarantineManager> quarantineManager =
        std::make_shared<QuarantineManagerImpl>(std::move(mockSafeStoreWrapper));

    EXPECT_NO_THROW(quarantineManager->initialise());
    ASSERT_EQ(quarantineManager->getState(), QuarantineManagerState::INITIALISED);
    datatypes::AutoFd fdHolder;
    ASSERT_EQ(
        common::CentralEnums::QuarantineResult::NOT_FOUND,
        quarantineManager->quarantineFile(
            m_dir + "/" + m_file, m_threatID, m_threatName, m_SHA256, std::move(fdHolder)));
}
TEST_F(QuarantineManagerTests, quarantineFileFailsAndDbIsMarkedCorrupt)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };

    // Force the m_dbErrorCountThreshold variable to be set to 1 for tests so that we only have to see one error
    // before the database is reported to be corrupt.
    EXPECT_CALL(*filesystemMock, removeFile("/tmp/av/var/safestore_dormant_flag", true)).WillOnce(Return());
    EXPECT_CALL(*filesystemMock, exists("/tmp/av/var/persist-safeStoreDbErrorThreshold")).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, writeFile("/tmp/av/var/persist-safeStoreDbErrorThreshold", "1"));
    EXPECT_CALL(*filesystemMock, readFile("/tmp/av/var/persist-safeStoreDbErrorThreshold")).WillOnce(Return("1"));

    EXPECT_CALL(*filesystemMock, isFile("/tmp/av/var/safestore_db/safestore.pw")).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile("/tmp/av/var/safestore_db/safestore.pw")).WillOnce(Return("a password"));

    EXPECT_CALL(*m_mockSafeStoreWrapper, initialise("/tmp/av/var/safestore_db", "safestore.db", "a password"))
        .WillRepeatedly(Return(InitReturnCode::OK));

    EXPECT_CALL(*m_mockSafeStoreWrapper, saveFile(m_dir, m_file, m_threatID, m_threatName, _))
        .WillRepeatedly(Return(SaveFileReturnCode::DB_ERROR));
    EXPECT_CALL(
        *filesystemMock,
        writeFileAtomically("/tmp/av/var/safestore_dormant_flag", "SafeStore database corrupt", "/tmp/tmp"));
    EXPECT_CALL(*m_mockSafeStoreWrapper, setObjectCustomDataString(_, "SHA256", m_SHA256)).WillRepeatedly(Return(true));
    EXPECT_CALL(*m_mockSafeStoreWrapper, finaliseObject(_)).WillRepeatedly(Return(true));

    auto mockGetIdMethods = std::make_shared<StrictMock<MockISafeStoreGetIdMethods>>();
    auto mockReleaseMethods = std::make_shared<StrictMock<MockISafeStoreReleaseMethods>>();
    EXPECT_CALL(*mockReleaseMethods, releaseObjectHandle(_));

    EXPECT_CALL(*m_mockSafeStoreWrapper, createObjectHandleHolder())
        .WillOnce(Return(ByMove(std::make_unique<ObjectHandleHolder>(mockGetIdMethods, mockReleaseMethods))));

    std::shared_ptr<IQuarantineManager> quarantineManager =
        std::make_shared<QuarantineManagerImpl>(std::move(m_mockSafeStoreWrapper));

    EXPECT_NO_THROW(quarantineManager->initialise());
    ASSERT_EQ(quarantineManager->getState(), QuarantineManagerState::INITIALISED);
    datatypes::AutoFd fdHolder;
    ASSERT_EQ(
        common::CentralEnums::QuarantineResult::FAILED_TO_DELETE_FILE,
        quarantineManager->quarantineFile(
            m_dir + "/" + m_file, m_threatID, m_threatName, m_SHA256, std::move(fdHolder)));

    ASSERT_EQ(quarantineManager->getState(), QuarantineManagerState::CORRUPT);
}

TEST_F(QuarantineManagerTests, quarantineFileFailsToFinaliseFile)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };
    addCommonPersistValueExpects(*filesystemMock);
    EXPECT_CALL(*filesystemMock, removeFile("/tmp/av/var/safestore_dormant_flag", true)).WillOnce(Return());
    EXPECT_CALL(*filesystemMock, isFile("/tmp/av/var/safestore_db/safestore.pw")).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile("/tmp/av/var/safestore_db/safestore.pw")).WillOnce(Return("a password"));
    EXPECT_CALL(*filesystemMock, compareFileDescriptors(_, _)).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, removeFileOrDirectory(_));
    EXPECT_CALL(*filesystemMock, getFileInfoDescriptor(_)).WillOnce(Return(100));
    EXPECT_CALL(*filesystemMock, getFileInfoDescriptorFromDirectoryFD(_, _)).WillOnce(Return(120));

    EXPECT_CALL(*m_mockSafeStoreWrapper, initialise("/tmp/av/var/safestore_db", "safestore.db", "a password"))
        .WillOnce(Return(InitReturnCode::OK));

    auto mockGetIdMethods = std::make_shared<StrictMock<MockISafeStoreGetIdMethods>>();
    auto mockReleaseMethods = std::make_shared<StrictMock<MockISafeStoreReleaseMethods>>();

    EXPECT_CALL(*mockReleaseMethods, releaseObjectHandle(_));
    EXPECT_CALL(*m_mockSafeStoreWrapper, createObjectHandleHolder())
        .WillOnce(Return(ByMove(std::make_unique<ObjectHandleHolder>(mockGetIdMethods, mockReleaseMethods))));

    EXPECT_CALL(*m_mockSafeStoreWrapper, saveFile(m_dir, m_file, m_threatID, m_threatName, _))
        .WillOnce(Return(SaveFileReturnCode::OK));
    EXPECT_CALL(*m_mockSafeStoreWrapper, setObjectCustomDataString(_, "SHA256", m_SHA256)).WillOnce(Return(true));
    EXPECT_CALL(*m_mockSafeStoreWrapper, finaliseObject(_)).WillOnce(Return(false));

    std::shared_ptr<IQuarantineManager> quarantineManager =
        std::make_shared<QuarantineManagerImpl>(std::move(m_mockSafeStoreWrapper));

    EXPECT_NO_THROW(quarantineManager->initialise());
    ASSERT_EQ(quarantineManager->getState(), QuarantineManagerState::INITIALISED);
    datatypes::AutoFd fdHolder;
    ASSERT_EQ(
        common::CentralEnums::QuarantineResult::FAILED_TO_DELETE_FILE,
        quarantineManager->quarantineFile(
            m_dir + "/" + m_file, m_threatID, m_threatName, m_SHA256, std::move(fdHolder)));
}

TEST_F(QuarantineManagerTests, fileQuarantinesAndRemovesPreviouslySavedObjectsWithSameThreatId)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };
    addCommonPersistValueExpects(*filesystemMock);
    EXPECT_CALL(*filesystemMock, removeFile("/tmp/av/var/safestore_dormant_flag", true)).WillOnce(Return()); // 1
    EXPECT_CALL(*filesystemMock, isFile("/tmp/av/var/safestore_db/safestore.pw")).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile("/tmp/av/var/safestore_db/safestore.pw")).WillOnce(Return("a password"));
    EXPECT_CALL(*filesystemMock, compareFileDescriptors(_, _)).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, removeFileOrDirectory("/dir/file"));
    EXPECT_CALL(*filesystemMock, getFileInfoDescriptor(_)).WillOnce(Return(100));
    EXPECT_CALL(*filesystemMock, getFileInfoDescriptorFromDirectoryFD(_, _)).WillOnce(Return(120));

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
    ObjectHandleHolder object1_2_handle { mockGetIdMethods, mockReleaseMethods };
    *object1_2_handle.getRawHandlePtr() = object1_2_rawHandle;
    auto object1_2_handleArg = Property(&ObjectHandleHolder::getRawHandle, object1_2_rawHandle);
    EXPECT_CALL(*mockReleaseMethods, releaseObjectHandle(object1_2_rawHandle));

    // Previously quarantined objects
    const std::string object2_id = "22222222-2222-2222-2222-222222222222";
    void* object2_rawHandle = reinterpret_cast<SafeStoreObjectHandle>(3333);
    ObjectHandleHolder object2_handle { mockGetIdMethods, mockReleaseMethods };
    *object2_handle.getRawHandlePtr() = object2_rawHandle;
    auto object2_handleArg = Property(&ObjectHandleHolder::getRawHandle, object2_rawHandle);
    EXPECT_CALL(*mockReleaseMethods, releaseObjectHandle(object2_rawHandle));

    const std::string object3_id = "33333333-3333-3333-3333-333333333333";
    void* object3_rawHandle = reinterpret_cast<SafeStoreObjectHandle>(4444);
    ObjectHandleHolder object3_handle { mockGetIdMethods, mockReleaseMethods };
    *object3_handle.getRawHandlePtr() = object3_rawHandle;
    auto object3_handleArg = Property(&ObjectHandleHolder::getRawHandle, object3_rawHandle);
    EXPECT_CALL(*mockReleaseMethods, releaseObjectHandle(object3_rawHandle));

    EXPECT_CALL(*m_mockSafeStoreWrapper, createObjectHandleHolder())
        .WillOnce(Return(ByMove(std::move(object1_1_handle))));

    EXPECT_CALL(*m_mockSafeStoreWrapper, saveFile(m_dir, m_file, m_threatID, m_threatName, object1_1_handleArg))
        .WillOnce(Return(SaveFileReturnCode::OK));
    EXPECT_CALL(*m_mockSafeStoreWrapper, setObjectCustomDataString(object1_1_handleArg, "SHA256", m_SHA256))
        .WillOnce(Return(true));
    EXPECT_CALL(*m_mockSafeStoreWrapper, finaliseObject(object1_1_handleArg)).WillOnce(Return(true));

    std::vector<ObjectHandleHolder> searchResults;
    searchResults.push_back(std::move(object1_2_handle));
    searchResults.push_back(std::move(object2_handle));
    searchResults.push_back(std::move(object3_handle));
    EXPECT_CALL(*m_mockSafeStoreWrapper, find(SafeStoreFilter { m_threatID, {}, {}, {}, ObjectType::FILE, {}, {}, {} }))
        .WillOnce(Return(ByMove(std::move(searchResults))));

    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectId(object1_1_handleArg)).WillOnce(Return(object1_id));

    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectId(object1_2_handleArg)).WillOnce(Return(object1_id));

    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectId(object2_handleArg)).WillOnce(Return(object2_id));
    EXPECT_CALL(*m_mockSafeStoreWrapper, deleteObjectById(object2_id)).WillOnce(Return(true));

    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectId(object3_handleArg)).WillOnce(Return(object3_id));
    EXPECT_CALL(*m_mockSafeStoreWrapper, deleteObjectById(object3_id)).WillOnce(Return(true));

    std::shared_ptr<IQuarantineManager> quarantineManager =
        std::make_shared<QuarantineManagerImpl>(std::move(m_mockSafeStoreWrapper));

    EXPECT_NO_THROW(quarantineManager->initialise());
    ASSERT_EQ(quarantineManager->getState(), QuarantineManagerState::INITIALISED);
    datatypes::AutoFd fdHolder;
    ASSERT_EQ(
        common::CentralEnums::QuarantineResult::SUCCESS,
        quarantineManager->quarantineFile(
            m_dir + "/" + m_file, m_threatID, m_threatName, m_SHA256, std::move(fdHolder)));
}

TEST_F(QuarantineManagerTests, fileQuarantinesButFailsToRemovePreviouslySavedObjectWithSameThreatId)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };
    addCommonPersistValueExpects(*filesystemMock);
    EXPECT_CALL(*filesystemMock, removeFile("/tmp/av/var/safestore_dormant_flag", true)).WillOnce(Return()); // 1
    EXPECT_CALL(*filesystemMock, isFile("/tmp/av/var/safestore_db/safestore.pw")).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile("/tmp/av/var/safestore_db/safestore.pw")).WillOnce(Return("a password"));
    EXPECT_CALL(*filesystemMock, compareFileDescriptors(_, _)).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, removeFileOrDirectory("/dir/file"));
    EXPECT_CALL(*filesystemMock, getFileInfoDescriptor(_)).WillOnce(Return(100));
    EXPECT_CALL(*filesystemMock, getFileInfoDescriptorFromDirectoryFD(_, _)).WillOnce(Return(120));

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
    ObjectHandleHolder object1_2_handle { mockGetIdMethods, mockReleaseMethods };
    *object1_2_handle.getRawHandlePtr() = object1_2_rawHandle;
    auto object1_2_handleArg = Property(&ObjectHandleHolder::getRawHandle, object1_2_rawHandle);
    EXPECT_CALL(*mockReleaseMethods, releaseObjectHandle(object1_2_rawHandle));

    // Previously quarantined objects
    const std::string object2_id = "22222222-2222-2222-2222-222222222222";
    void* object2_rawHandle = reinterpret_cast<SafeStoreObjectHandle>(3333);
    ObjectHandleHolder object2_handle { mockGetIdMethods, mockReleaseMethods };
    *object2_handle.getRawHandlePtr() = object2_rawHandle;
    auto object2_handleArg = Property(&ObjectHandleHolder::getRawHandle, object2_rawHandle);
    EXPECT_CALL(*mockReleaseMethods, releaseObjectHandle(object2_rawHandle));

    const std::string object3_id = "33333333-3333-3333-3333-333333333333";
    void* object3_rawHandle = reinterpret_cast<SafeStoreObjectHandle>(4444);
    ObjectHandleHolder object3_handle { mockGetIdMethods, mockReleaseMethods };
    *object3_handle.getRawHandlePtr() = object3_rawHandle;
    auto object3_handleArg = Property(&ObjectHandleHolder::getRawHandle, object3_rawHandle);
    EXPECT_CALL(*mockReleaseMethods, releaseObjectHandle(object3_rawHandle));

    EXPECT_CALL(*m_mockSafeStoreWrapper, createObjectHandleHolder())
        .WillOnce(Return(ByMove(std::move(object1_1_handle))));

    EXPECT_CALL(*m_mockSafeStoreWrapper, saveFile(m_dir, m_file, m_threatID, m_threatName, object1_1_handleArg))
        .WillOnce(Return(SaveFileReturnCode::OK));
    EXPECT_CALL(*m_mockSafeStoreWrapper, setObjectCustomDataString(object1_1_handleArg, "SHA256", m_SHA256))
        .WillOnce(Return(true));
    EXPECT_CALL(*m_mockSafeStoreWrapper, finaliseObject(object1_1_handleArg)).WillOnce(Return(true));

    std::vector<ObjectHandleHolder> searchResults;
    searchResults.push_back(std::move(object1_2_handle));
    searchResults.push_back(std::move(object2_handle));
    searchResults.push_back(std::move(object3_handle));
    EXPECT_CALL(*m_mockSafeStoreWrapper, find(SafeStoreFilter { m_threatID, {}, {}, {}, ObjectType::FILE, {}, {}, {} }))
        .WillOnce(Return(ByMove(std::move(searchResults))));

    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectId(object1_1_handleArg)).WillOnce(Return(object1_id));

    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectId(object1_2_handleArg)).WillOnce(Return(object1_id));

    // This one fails to be deleted
    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectId(object2_handleArg)).WillOnce(Return(object2_id));
    EXPECT_CALL(*m_mockSafeStoreWrapper, deleteObjectById(object2_id)).WillOnce(Return(false));

    EXPECT_CALL(*m_mockSafeStoreWrapper, getObjectId(object3_handleArg)).WillOnce(Return(object3_id));
    EXPECT_CALL(*m_mockSafeStoreWrapper, deleteObjectById(object3_id)).WillOnce(Return(true));

    std::shared_ptr<IQuarantineManager> quarantineManager =
        std::make_shared<QuarantineManagerImpl>(std::move(m_mockSafeStoreWrapper));

    EXPECT_NO_THROW(quarantineManager->initialise());
    ASSERT_EQ(quarantineManager->getState(), QuarantineManagerState::INITIALISED);
    datatypes::AutoFd fdHolder;
    ASSERT_EQ(
        common::CentralEnums::QuarantineResult::SUCCESS,
        quarantineManager->quarantineFile(
            m_dir + "/" + m_file, m_threatID, m_threatName, m_SHA256, std::move(fdHolder)));
}

TEST_F(QuarantineManagerTests, tryToQuarantineFileWhenThreatIdIsIncorrectSize)
{
    std::string threatId = "Tfe8974b97b4b7a6a33b4c52acb4ffba0c11ebbf208a519245791ad32a96227d8";

    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };
    addCommonPersistValueExpects(*filesystemMock);
    EXPECT_CALL(*filesystemMock, removeFile("/tmp/av/var/safestore_dormant_flag", true)).WillOnce(Return());
    EXPECT_CALL(*filesystemMock, isFile("/tmp/av/var/safestore_db/safestore.pw")).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile("/tmp/av/var/safestore_db/safestore.pw")).WillOnce(Return("a password"));

    EXPECT_CALL(*m_mockSafeStoreWrapper, initialise("/tmp/av/var/safestore_db", "safestore.db", "a password"))
        .WillOnce(Return(InitReturnCode::OK));

    std::shared_ptr<IQuarantineManager> quarantineManager =
        std::make_shared<QuarantineManagerImpl>(std::move(m_mockSafeStoreWrapper));
    EXPECT_NO_THROW(quarantineManager->initialise());
    ASSERT_EQ(quarantineManager->getState(), QuarantineManagerState::INITIALISED);
    datatypes::AutoFd fdHolder;
    ASSERT_EQ(
        common::CentralEnums::QuarantineResult::NOT_FOUND,
        quarantineManager->quarantineFile(m_dir + "/" + m_file, threatId, m_threatName, m_SHA256, std::move(fdHolder)));
}

TEST_F(QuarantineManagerTests, tryToQuarantineFileWhenUninitialised)
{
    EXPECT_CALL(*m_mockSafeStoreWrapper, saveFile(m_dir, m_file, m_threatID, m_threatName, _)).Times(0);

    std::shared_ptr<IQuarantineManager> quarantineManager =
        std::make_shared<QuarantineManagerImpl>(std::move(m_mockSafeStoreWrapper));

    ASSERT_EQ(quarantineManager->getState(), QuarantineManagerState::STARTUP);
    datatypes::AutoFd fdHolder;
    common::CentralEnums::QuarantineResult result;
    EXPECT_NO_THROW(
        result = quarantineManager->quarantineFile(
            m_dir + "/" + m_file, m_threatID, m_threatName, m_SHA256, std::move(fdHolder)));
    ASSERT_EQ(result, common::CentralEnums::QuarantineResult::NOT_FOUND);
}

TEST_F(QuarantineManagerTests, deleteDatabaseCalledOnInitialisedDb)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };
    addCommonPersistValueExpects(*filesystemMock);
    EXPECT_CALL(*filesystemMock, removeFile("/tmp/av/var/safestore_dormant_flag", true)).WillOnce(Return());
    EXPECT_CALL(*filesystemMock, isFile("/tmp/av/var/safestore_db/safestore.pw")).WillOnce(Return(false));
    EXPECT_CALL(*filesystemMock, readFile("/tmp/av/var/safestore_db/safestore.pw")).Times(0);
    EXPECT_CALL(*filesystemMock, isDirectory("/tmp/av/var/safestore_db")).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, writeFile("/tmp/av/var/safestore_db/safestore.pw", _)).Times(1);
    EXPECT_CALL(*filesystemMock, exists("/tmp/av/var/safestore_db")).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, removeFilesInDirectory("/tmp/av/var/safestore_db")).Times(1);

    EXPECT_CALL(
        *filesystemMock,
        writeFileAtomically("/tmp/av/var/safestore_dormant_flag", "SafeStore database uninitialised", "/tmp/tmp"));
    EXPECT_CALL(*m_mockSafeStoreWrapper, initialise("/tmp/av/var/safestore_db", "safestore.db", _))
        .WillOnce(Return(InitReturnCode::OK));
    std::shared_ptr<IQuarantineManager> quarantineManager =
        std::make_shared<QuarantineManagerImpl>(std::move(m_mockSafeStoreWrapper));

    EXPECT_NO_THROW(quarantineManager->initialise());
    ASSERT_EQ(quarantineManager->getState(), QuarantineManagerState::INITIALISED);
    EXPECT_NO_THROW(quarantineManager->deleteDatabase());
}

TEST_F(QuarantineManagerTests, deleteDatabaseCalledOnUninitialisedDbThatDoesNotExist)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };
    addCommonPersistValueExpects(*filesystemMock);
    EXPECT_CALL(*filesystemMock, exists("/tmp/av/var/safestore_db")).WillOnce(Return(false));

    std::shared_ptr<IQuarantineManager> quarantineManager =
        std::make_shared<QuarantineManagerImpl>(std::move(m_mockSafeStoreWrapper));

    ASSERT_EQ(quarantineManager->getState(), QuarantineManagerState::STARTUP);
    EXPECT_NO_THROW(quarantineManager->deleteDatabase());
}

TEST_F(QuarantineManagerTests, deleteDatabaseCalledOnUninitialisedDbThatDoesExist)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };
    addCommonPersistValueExpects(*filesystemMock);
    EXPECT_CALL(
        *filesystemMock,
        writeFileAtomically("/tmp/av/var/safestore_dormant_flag", "SafeStore database uninitialised", "/tmp/tmp"));
    EXPECT_CALL(*filesystemMock, exists("/tmp/av/var/safestore_db")).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, removeFilesInDirectory("/tmp/av/var/safestore_db")).Times(1);

    std::shared_ptr<IQuarantineManager> quarantineManager =
        std::make_shared<QuarantineManagerImpl>(std::move(m_mockSafeStoreWrapper));

    ASSERT_EQ(quarantineManager->getState(), QuarantineManagerState::STARTUP);
    EXPECT_NO_THROW(quarantineManager->deleteDatabase());
}

TEST_F(QuarantineManagerTests, deleteDatabaseDoesNotThrowOnFailure)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };
    addCommonPersistValueExpects(*filesystemMock);
    EXPECT_CALL(*filesystemMock, exists("/tmp/av/var/safestore_db")).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, removeFilesInDirectory("/tmp/av/var/safestore_db"))
        .WillOnce(Throw(Common::FileSystem::IFileSystemException("File does not exist.")));

    std::shared_ptr<IQuarantineManager> quarantineManager =
        std::make_shared<QuarantineManagerImpl>(std::move(m_mockSafeStoreWrapper));

    bool result = true;
    EXPECT_NO_THROW(result = quarantineManager->deleteDatabase());
    ASSERT_FALSE(result);
}

// TODO: LINUXDAR-5734 - replace this placeholder test
TEST_F(QuarantineManagerTests, testRescanDatabase)
{
    std::shared_ptr<IQuarantineManager> quarantineManager =
        std::make_shared<QuarantineManagerImpl>(std::move(m_mockSafeStoreWrapper));

    EXPECT_NO_THROW(quarantineManager->rescanDatabase());
}