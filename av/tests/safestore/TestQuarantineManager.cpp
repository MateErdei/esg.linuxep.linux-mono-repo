// Copyright 2022, Sophos Limited.  All rights reserved.

#include "MockISafeStoreWrapper.h"

#include "../common/LogInitializedTests.h"
#include "safestore/QuarantineManager/IQuarantineManager.h"
#include "safestore/QuarantineManager/QuarantineManagerImpl.h"

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
#include "Common/FileSystem/IFileSystem.h"
#include "Common/FileSystem/IFileSystemException.h"
#include "Common/Helpers/FileSystemReplaceAndRestore.h"
#include "Common/Helpers/MockFileSystem.h"
#include "common/ApplicationPaths.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;
using namespace safestore::QuarantineManager;

class QuarantineManagerTests : public LogInitializedTests
{
protected:
    virtual void SetUp()
    {
        auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
        appConfig.setData("SOPHOS_INSTALL", "/tmp");
        appConfig.setData("PLUGIN_INSTALL", "/tmp/av");
    }

    void addCommonPersistValueExpects(StrictMock<MockFileSystem>& filesystemMock)
    {
        EXPECT_CALL(filesystemMock, exists("/tmp/av/var/persist-safeStoreDbErrorThreshold")).WillOnce(Return(false));
        EXPECT_CALL(filesystemMock, writeFile("/tmp/av/var/persist-safeStoreDbErrorThreshold", "10"));
    }

    // Common test constants
    inline static const std::string m_dir = "/dir";
    inline static const std::string m_file = "file";
    inline static const std::string m_threatID = "abcdefghijklmnop"; // 16 bytes
    inline static const std::string m_threatName = "threatName";
    inline static const std::string m_SHA256 = "SHA256abcdef";
};

TEST_F(QuarantineManagerTests, initDbWithExistingPassword)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };

    addCommonPersistValueExpects(*filesystemMock);

    EXPECT_CALL(*filesystemMock, isFile(Plugin::getSafeStorePasswordFilePath())).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(Plugin::getSafeStorePasswordFilePath())).WillOnce(Return("a password"));
    EXPECT_CALL(*filesystemMock, isFile(Plugin::getSafeStoreConfigPath())).WillOnce(Return(false));

    auto mockSafeStoreWrapper = std::make_unique<StrictMock<MockISafeStoreWrapper>>();
    EXPECT_CALL(*mockSafeStoreWrapper, initialise(Plugin::getSafeStoreDbDirPath(), Plugin::getSafeStoreDbFileName(), "a password"))
        .WillOnce(Return(InitReturnCode::OK));
    std::shared_ptr<IQuarantineManager> quarantineManager =
        std::make_shared<QuarantineManagerImpl>(std::move(mockSafeStoreWrapper));

    EXPECT_NO_THROW(quarantineManager->initialise());
}

TEST_F(QuarantineManagerTests, initDbAndGeneratePassword)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };
    addCommonPersistValueExpects(*filesystemMock);
    EXPECT_CALL(*filesystemMock, isFile(Plugin::getSafeStorePasswordFilePath())).WillOnce(Return(false));
    EXPECT_CALL(*filesystemMock, readFile(Plugin::getSafeStorePasswordFilePath())).Times(0);
    EXPECT_CALL(*filesystemMock, isDirectory(Plugin::getSafeStoreDbDirPath())).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, writeFile(Plugin::getSafeStorePasswordFilePath(), _)).Times(1);
    EXPECT_CALL(*filesystemMock, isFile(Plugin::getSafeStoreConfigPath())).WillOnce(Return(false));

    auto mockSafeStoreWrapper = std::make_unique<StrictMock<MockISafeStoreWrapper>>();
    EXPECT_CALL(*mockSafeStoreWrapper, initialise(Plugin::getSafeStoreDbDirPath(), Plugin::getSafeStoreDbFileName(), _))
        .WillOnce(Return(InitReturnCode::OK));
    std::shared_ptr<IQuarantineManager> quarantineManager =
        std::make_shared<QuarantineManagerImpl>(std::move(mockSafeStoreWrapper));

    EXPECT_NO_THROW(quarantineManager->initialise());
}

TEST_F(QuarantineManagerTests, uninitialisedDbIsInitialisedAfterSuccessfulInitCall)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };
    addCommonPersistValueExpects(*filesystemMock);
    EXPECT_CALL(*filesystemMock, isFile(Plugin::getSafeStorePasswordFilePath())).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(Plugin::getSafeStorePasswordFilePath())).WillOnce(Return("a password"));
    EXPECT_CALL(*filesystemMock, isFile(Plugin::getSafeStoreConfigPath())).WillOnce(Return(false));

    auto mockSafeStoreWrapper = std::make_unique<StrictMock<MockISafeStoreWrapper>>();
    EXPECT_CALL(*mockSafeStoreWrapper, initialise(Plugin::getSafeStoreDbDirPath(), Plugin::getSafeStoreDbFileName(), "a password"))
        .WillOnce(Return(InitReturnCode::OK));
    std::shared_ptr<IQuarantineManager> quarantineManager =
        std::make_shared<QuarantineManagerImpl>(std::move(mockSafeStoreWrapper));

    ASSERT_EQ(quarantineManager->getState(), QuarantineManagerState::UNINITIALISED);
    EXPECT_NO_THROW(quarantineManager->initialise());
    ASSERT_EQ(quarantineManager->getState(), QuarantineManagerState::INITIALISED);
}

TEST_F(QuarantineManagerTests, uninitialisedDbStateIsStillUnitialisedAfterFailedInitCall)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };
    addCommonPersistValueExpects(*filesystemMock);
    EXPECT_CALL(*filesystemMock, isFile(Plugin::getSafeStorePasswordFilePath())).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(Plugin::getSafeStorePasswordFilePath())).WillOnce(Return("a password"));
    EXPECT_CALL(*filesystemMock, isFile(Plugin::getSafeStoreConfigPath())).WillRepeatedly(Return(false));

    auto mockSafeStoreWrapper = std::make_unique<StrictMock<MockISafeStoreWrapper>>();
    EXPECT_CALL(*mockSafeStoreWrapper, initialise(Plugin::getSafeStoreDbDirPath(), Plugin::getSafeStoreDbFileName(), "a password"))
        .WillOnce(Return(InitReturnCode::DB_OPEN_FAILED));
    std::shared_ptr<IQuarantineManager> quarantineManager =
        std::make_shared<QuarantineManagerImpl>(std::move(mockSafeStoreWrapper));

    ASSERT_EQ(quarantineManager->getState(), QuarantineManagerState::UNINITIALISED);
    EXPECT_NO_THROW(quarantineManager->initialise());
    ASSERT_EQ(quarantineManager->getState(), QuarantineManagerState::UNINITIALISED);
}

TEST_F(QuarantineManagerTests, initFailsOnDBErrorAndDbIsMarkedCorrupt)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };
    addCommonPersistValueExpects(*filesystemMock);
    EXPECT_CALL(*filesystemMock, isFile(Plugin::getSafeStorePasswordFilePath())).WillRepeatedly(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(Plugin::getSafeStorePasswordFilePath()))
        .WillRepeatedly(Return("a password"));
    EXPECT_CALL(*filesystemMock, isFile(Plugin::getSafeStoreConfigPath())).WillRepeatedly(Return(false));

    auto mockSafeStoreWrapper = std::make_unique<StrictMock<MockISafeStoreWrapper>>();
    EXPECT_CALL(*mockSafeStoreWrapper, initialise(Plugin::getSafeStoreDbDirPath(), Plugin::getSafeStoreDbFileName(), "a password"))
        .WillRepeatedly(Return(InitReturnCode::DB_ERROR));
    std::shared_ptr<IQuarantineManager> quarantineManager =
        std::make_shared<QuarantineManagerImpl>(std::move(mockSafeStoreWrapper));

    ASSERT_EQ(quarantineManager->getState(), QuarantineManagerState::UNINITIALISED);

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
    EXPECT_CALL(*filesystemMock, isFile(Plugin::getSafeStorePasswordFilePath())).WillRepeatedly(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(Plugin::getSafeStorePasswordFilePath()))
        .WillRepeatedly(Return("a password"));
    EXPECT_CALL(*filesystemMock, isFile(Plugin::getSafeStoreConfigPath())).WillRepeatedly(Return(false));

    auto mockSafeStoreWrapper = std::make_unique<StrictMock<MockISafeStoreWrapper>>();
    EXPECT_CALL(*mockSafeStoreWrapper, initialise(Plugin::getSafeStoreDbDirPath(), Plugin::getSafeStoreDbFileName(), "a password"))
        .WillRepeatedly(Return(InitReturnCode::DB_OPEN_FAILED));
    std::shared_ptr<IQuarantineManager> quarantineManager =
        std::make_shared<QuarantineManagerImpl>(std::move(mockSafeStoreWrapper));

    ASSERT_EQ(quarantineManager->getState(), QuarantineManagerState::UNINITIALISED);

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
    EXPECT_CALL(*filesystemMock, isFile(Plugin::getSafeStorePasswordFilePath())).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(Plugin::getSafeStorePasswordFilePath())).WillOnce(Return("a password"));
    EXPECT_CALL(*filesystemMock, isFile(Plugin::getSafeStoreConfigPath())).WillOnce(Return(false));

    auto mockSafeStoreWrapper = std::make_unique<StrictMock<MockISafeStoreWrapper>>();
    EXPECT_CALL(*mockSafeStoreWrapper, initialise(Plugin::getSafeStoreDbDirPath(), Plugin::getSafeStoreDbFileName(), "a password"))
        .WillOnce(Return(InitReturnCode::OK));

    auto mockGetIdMethods = std::make_shared<StrictMock<MockISafeStoreGetIdMethods>>();
    auto  mockReleaseMethods = std::make_shared<StrictMock<MockISafeStoreReleaseMethods>>();
    EXPECT_CALL(*mockReleaseMethods, releaseObjectHandle(_));

    EXPECT_CALL(*mockSafeStoreWrapper, createObjectHandleHolder())
        .WillOnce(Return(ByMove(std::make_unique<ObjectHandleHolder>(mockGetIdMethods, mockReleaseMethods))));

    EXPECT_CALL(*mockSafeStoreWrapper, saveFile(m_dir, m_file, m_threatID, m_threatName, _))
        .WillOnce(Return(SaveFileReturnCode::OK));
    EXPECT_CALL(*mockSafeStoreWrapper, setObjectCustomDataString(_, "SHA256", m_SHA256)).WillOnce(Return(true));
    EXPECT_CALL(*mockSafeStoreWrapper, finaliseObject(_)).WillOnce(Return(true));

    std::shared_ptr<IQuarantineManager> quarantineManager =
        std::make_shared<QuarantineManagerImpl>(std::move(mockSafeStoreWrapper));

    EXPECT_NO_THROW(quarantineManager->initialise());
    ASSERT_EQ(quarantineManager->getState(), QuarantineManagerState::INITIALISED);
    datatypes::AutoFd fdHolder;
    ASSERT_TRUE(quarantineManager->quarantineFile(
        m_dir + "/" + m_file, m_threatID, m_threatName, m_SHA256, std::move(fdHolder)));
}

TEST_F(QuarantineManagerTests, quarantineFileFailsAndDbIsMarkedCorrupt)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };

    // Force the m_dbErrorCountThreshold variable to be set to 1 for tests so that we only have to see one error
    // before the database is reported to be corrupt.
    EXPECT_CALL(*filesystemMock, exists("/tmp/av/var/persist-safeStoreDbErrorThreshold")).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, writeFile("/tmp/av/var/persist-safeStoreDbErrorThreshold", "1"));
    EXPECT_CALL(*filesystemMock, readFile("/tmp/av/var/persist-safeStoreDbErrorThreshold")).WillOnce(Return("1"));

    EXPECT_CALL(*filesystemMock, isFile(Plugin::getSafeStorePasswordFilePath())).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(Plugin::getSafeStorePasswordFilePath())).WillOnce(Return("a password"));
    EXPECT_CALL(*filesystemMock, isFile(Plugin::getSafeStoreConfigPath())).WillOnce(Return(false));

    auto mockSafeStoreWrapper = std::make_unique<StrictMock<MockISafeStoreWrapper>>();
    EXPECT_CALL(*mockSafeStoreWrapper, initialise(Plugin::getSafeStoreDbDirPath(), Plugin::getSafeStoreDbFileName(), "a password"))
        .WillRepeatedly(Return(InitReturnCode::OK));

    EXPECT_CALL(*mockSafeStoreWrapper, saveFile(m_dir, m_file, m_threatID, m_threatName, _))
        .WillRepeatedly(Return(SaveFileReturnCode::DB_ERROR));
    EXPECT_CALL(*mockSafeStoreWrapper, setObjectCustomDataString(_, "SHA256", m_SHA256)).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockSafeStoreWrapper, finaliseObject(_)).WillRepeatedly(Return(true));

    auto mockGetIdMethods = std::make_shared<StrictMock<MockISafeStoreGetIdMethods>>();
    auto  mockReleaseMethods = std::make_shared<StrictMock<MockISafeStoreReleaseMethods>>();
    EXPECT_CALL(*mockReleaseMethods, releaseObjectHandle(_));

    EXPECT_CALL(*mockSafeStoreWrapper, createObjectHandleHolder())
        .WillOnce(Return(ByMove(std::make_unique<ObjectHandleHolder>(mockGetIdMethods, mockReleaseMethods))));

    std::shared_ptr<IQuarantineManager> quarantineManager =
        std::make_shared<QuarantineManagerImpl>(std::move(mockSafeStoreWrapper));

    EXPECT_NO_THROW(quarantineManager->initialise());
    ASSERT_EQ(quarantineManager->getState(), QuarantineManagerState::INITIALISED);
    datatypes::AutoFd fdHolder;
    ASSERT_FALSE(quarantineManager->quarantineFile(
        m_dir + "/" + m_file, m_threatID, m_threatName, m_SHA256, std::move(fdHolder)));

    ASSERT_EQ(quarantineManager->getState(), QuarantineManagerState::CORRUPT);
}

TEST_F(QuarantineManagerTests, quarantineFileFailsToFinaliseFile)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };
    addCommonPersistValueExpects(*filesystemMock);
    EXPECT_CALL(*filesystemMock, isFile(Plugin::getSafeStorePasswordFilePath())).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(Plugin::getSafeStorePasswordFilePath())).WillOnce(Return("a password"));
    EXPECT_CALL(*filesystemMock, isFile(Plugin::getSafeStoreConfigPath())).WillOnce(Return(false));

    auto mockSafeStoreWrapper = std::make_unique<StrictMock<MockISafeStoreWrapper>>();
    EXPECT_CALL(*mockSafeStoreWrapper, initialise(Plugin::getSafeStoreDbDirPath(), Plugin::getSafeStoreDbFileName(), "a password"))
        .WillOnce(Return(InitReturnCode::OK));

    auto mockGetIdMethods = std::make_shared<StrictMock<MockISafeStoreGetIdMethods>>();
    auto  mockReleaseMethods = std::make_shared<StrictMock<MockISafeStoreReleaseMethods>>();

    EXPECT_CALL(*mockReleaseMethods, releaseObjectHandle(_));
    EXPECT_CALL(*mockSafeStoreWrapper, createObjectHandleHolder())
        .WillOnce(Return(ByMove(std::make_unique<ObjectHandleHolder>(mockGetIdMethods, mockReleaseMethods))));

    EXPECT_CALL(*mockSafeStoreWrapper, saveFile(m_dir, m_file, m_threatID, m_threatName, _))
        .WillOnce(Return(SaveFileReturnCode::OK));
    EXPECT_CALL(*mockSafeStoreWrapper, setObjectCustomDataString(_, "SHA256", m_SHA256)).WillOnce(Return(true));
    EXPECT_CALL(*mockSafeStoreWrapper, finaliseObject(_)).WillOnce(Return(false));

    std::shared_ptr<IQuarantineManager> quarantineManager =
        std::make_shared<QuarantineManagerImpl>(std::move(mockSafeStoreWrapper));

    EXPECT_NO_THROW(quarantineManager->initialise());
    ASSERT_EQ(quarantineManager->getState(), QuarantineManagerState::INITIALISED);
    datatypes::AutoFd fdHolder;
    ASSERT_FALSE(quarantineManager->quarantineFile(
        m_dir + "/" + m_file, m_threatID, m_threatName, m_SHA256, std::move(fdHolder)));
}

TEST_F(QuarantineManagerTests, tryToQuarantineFileWhenThreatIdIsIncorrectSize)
{
    std::string threatId = "Tfe8974b97b4b7a6a33b4c52acb4ffba0c11ebbf208a519245791ad32a96227d8";

    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };
    addCommonPersistValueExpects(*filesystemMock);
    EXPECT_CALL(*filesystemMock, isFile(Plugin::getSafeStorePasswordFilePath())).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(Plugin::getSafeStorePasswordFilePath())).WillOnce(Return("a password"));
    EXPECT_CALL(*filesystemMock, isFile(Plugin::getSafeStoreConfigPath())).WillOnce(Return(false));

    auto mockSafeStoreWrapper = std::make_unique<StrictMock<MockISafeStoreWrapper>>();
    EXPECT_CALL(*mockSafeStoreWrapper, initialise(Plugin::getSafeStoreDbDirPath(), Plugin::getSafeStoreDbFileName(), "a password"))
        .WillOnce(Return(InitReturnCode::OK));

    std::shared_ptr<IQuarantineManager> quarantineManager =
        std::make_shared<QuarantineManagerImpl>(std::move(mockSafeStoreWrapper));
    EXPECT_NO_THROW(quarantineManager->initialise());
    ASSERT_EQ(quarantineManager->getState(), QuarantineManagerState::INITIALISED);
    datatypes::AutoFd fdHolder;
    ASSERT_FALSE(
        quarantineManager->quarantineFile(m_dir + "/" + m_file, threatId, m_threatName, m_SHA256, std::move(fdHolder)));
}

TEST_F(QuarantineManagerTests, tryToQuarantineFileWhenUninitialised)
{
    auto mockSafeStoreWrapper = std::make_unique<StrictMock<MockISafeStoreWrapper>>();
    EXPECT_CALL(*mockSafeStoreWrapper, saveFile(m_dir, m_file, m_threatID, m_threatName, _)).Times(0);

    std::shared_ptr<IQuarantineManager> quarantineManager =
        std::make_shared<QuarantineManagerImpl>(std::move(mockSafeStoreWrapper));

    ASSERT_EQ(quarantineManager->getState(), QuarantineManagerState::UNINITIALISED);
    datatypes::AutoFd fdHolder;
    bool quarantined = false;
    EXPECT_NO_THROW(
        quarantined = quarantineManager->quarantineFile(
            m_dir + "/" + m_file, m_threatID, m_threatName, m_SHA256, std::move(fdHolder)));
    ASSERT_FALSE(quarantined);
}

TEST_F(QuarantineManagerTests, deleteDatabaseCalledOnInitialisedDb)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };
    addCommonPersistValueExpects(*filesystemMock);
    EXPECT_CALL(*filesystemMock, isFile(Plugin::getSafeStorePasswordFilePath())).WillOnce(Return(false));
    EXPECT_CALL(*filesystemMock, readFile(Plugin::getSafeStorePasswordFilePath())).Times(0);
    EXPECT_CALL(*filesystemMock, isDirectory(Plugin::getSafeStoreDbDirPath())).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, writeFile(Plugin::getSafeStorePasswordFilePath(), _)).Times(1);
    EXPECT_CALL(*filesystemMock, exists(Plugin::getSafeStoreDbDirPath())).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, removeFilesInDirectory(Plugin::getSafeStoreDbDirPath())).Times(1);
    EXPECT_CALL(*filesystemMock, isFile(Plugin::getSafeStoreConfigPath())).WillRepeatedly(Return(false));

    auto mockSafeStoreWrapper = std::make_unique<StrictMock<MockISafeStoreWrapper>>();
    EXPECT_CALL(*mockSafeStoreWrapper, initialise(Plugin::getSafeStoreDbDirPath(), Plugin::getSafeStoreDbFileName(), _))
        .WillOnce(Return(InitReturnCode::OK));
    std::shared_ptr<IQuarantineManager> quarantineManager =
        std::make_shared<QuarantineManagerImpl>(std::move(mockSafeStoreWrapper));

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
    EXPECT_CALL(*filesystemMock, exists(Plugin::getSafeStoreDbDirPath())).WillOnce(Return(false));

    auto mockSafeStoreWrapper = std::make_unique<StrictMock<MockISafeStoreWrapper>>();

    std::shared_ptr<IQuarantineManager> quarantineManager =
        std::make_shared<QuarantineManagerImpl>(std::move(mockSafeStoreWrapper));

    ASSERT_EQ(quarantineManager->getState(), QuarantineManagerState::UNINITIALISED);
    EXPECT_NO_THROW(quarantineManager->deleteDatabase());
}

TEST_F(QuarantineManagerTests, deleteDatabaseCalledOnUninitialisedDbThatDoesExist)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };
    addCommonPersistValueExpects(*filesystemMock);
    EXPECT_CALL(*filesystemMock, exists(Plugin::getSafeStoreDbDirPath())).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, removeFilesInDirectory(Plugin::getSafeStoreDbDirPath())).Times(1);

    auto mockSafeStoreWrapper = std::make_unique<StrictMock<MockISafeStoreWrapper>>();

    std::shared_ptr<IQuarantineManager> quarantineManager =
        std::make_shared<QuarantineManagerImpl>(std::move(mockSafeStoreWrapper));

    ASSERT_EQ(quarantineManager->getState(), QuarantineManagerState::UNINITIALISED);
    EXPECT_NO_THROW(quarantineManager->deleteDatabase());
}

TEST_F(QuarantineManagerTests, deleteDatabaseDoesNotThrowOnFailure)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };
    addCommonPersistValueExpects(*filesystemMock);
    EXPECT_CALL(*filesystemMock, exists(Plugin::getSafeStoreDbDirPath())).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, removeFilesInDirectory(Plugin::getSafeStoreDbDirPath()))
        .WillOnce(Throw(Common::FileSystem::IFileSystemException("File does not exist.")));

    auto mockSafeStoreWrapper = std::make_unique<StrictMock<MockISafeStoreWrapper>>();

    std::shared_ptr<IQuarantineManager> quarantineManager =
        std::make_shared<QuarantineManagerImpl>(std::move(mockSafeStoreWrapper));

    bool result = true;
    EXPECT_NO_THROW(result = quarantineManager->deleteDatabase());
    ASSERT_FALSE(result);
}