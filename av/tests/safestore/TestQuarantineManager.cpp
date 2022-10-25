// Copyright 2022, Sophos Limited.  All rights reserved.

#include "MockISafeStoreWrapper.h"

#include "../common/LogInitializedTests.h"
#include "safestore/IQuarantineManager.h"
#include "safestore/QuarantineManagerImpl.h"

#include <scan_messages/QuarantineResponse.h>

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
#include "Common/FileSystem/IFileSystem.h"
#include "Common/FileSystem/IFileSystemException.h"
#include "Common/Helpers/FileSystemReplaceAndRestore.h"
#include "Common/Helpers/MockFileSystem.h"
#include "common/ApplicationPaths.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;

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

    EXPECT_CALL(*filesystemMock, isFile("/tmp/av/var/safestore_db/safestore.pw")).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile("/tmp/av/var/safestore_db/safestore.pw")).WillOnce(Return("a password"));

    auto mockSafeStoreWrapper = std::make_unique<StrictMock<MockISafeStoreWrapper>>();
    EXPECT_CALL(*mockSafeStoreWrapper, initialise("/tmp/av/var/safestore_db", "safestore.db", "a password"))
        .WillOnce(Return(safestore::InitReturnCode::OK));
    std::shared_ptr<safestore::IQuarantineManager> quarantineManager =
        std::make_shared<safestore::QuarantineManagerImpl>(std::move(mockSafeStoreWrapper));

    EXPECT_NO_THROW(quarantineManager->initialise());
}

TEST_F(QuarantineManagerTests, initDbAndGeneratePassword)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };
    addCommonPersistValueExpects(*filesystemMock);
    EXPECT_CALL(*filesystemMock, isFile("/tmp/av/var/safestore_db/safestore.pw")).WillOnce(Return(false));
    EXPECT_CALL(*filesystemMock, readFile("/tmp/av/var/safestore_db/safestore.pw")).Times(0);
    EXPECT_CALL(*filesystemMock, isDirectory("/tmp/av/var/safestore_db")).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, writeFile("/tmp/av/var/safestore_db/safestore.pw", _)).Times(1);

    auto mockSafeStoreWrapper = std::make_unique<StrictMock<MockISafeStoreWrapper>>();
    EXPECT_CALL(*mockSafeStoreWrapper, initialise("/tmp/av/var/safestore_db", "safestore.db", _))
        .WillOnce(Return(safestore::InitReturnCode::OK));
    std::shared_ptr<safestore::IQuarantineManager> quarantineManager =
        std::make_shared<safestore::QuarantineManagerImpl>(std::move(mockSafeStoreWrapper));

    EXPECT_NO_THROW(quarantineManager->initialise());
}

TEST_F(QuarantineManagerTests, uninitialisedDbIsInitialisedAfterSuccessfulInitCall)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };
    addCommonPersistValueExpects(*filesystemMock);
    EXPECT_CALL(*filesystemMock, isFile("/tmp/av/var/safestore_db/safestore.pw")).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile("/tmp/av/var/safestore_db/safestore.pw")).WillOnce(Return("a password"));

    auto mockSafeStoreWrapper = std::make_unique<StrictMock<MockISafeStoreWrapper>>();
    EXPECT_CALL(*mockSafeStoreWrapper, initialise("/tmp/av/var/safestore_db", "safestore.db", "a password"))
        .WillOnce(Return(safestore::InitReturnCode::OK));
    std::shared_ptr<safestore::IQuarantineManager> quarantineManager =
        std::make_shared<safestore::QuarantineManagerImpl>(std::move(mockSafeStoreWrapper));

    ASSERT_EQ(quarantineManager->getState(), safestore::QuarantineManagerState::UNINITIALISED);
    EXPECT_NO_THROW(quarantineManager->initialise());
    ASSERT_EQ(quarantineManager->getState(), safestore::QuarantineManagerState::INITIALISED);
}

TEST_F(QuarantineManagerTests, uninitialisedDbStateIsStillUnitialisedAfterFailedInitCall)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };
    addCommonPersistValueExpects(*filesystemMock);
    EXPECT_CALL(*filesystemMock, isFile("/tmp/av/var/safestore_db/safestore.pw")).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile("/tmp/av/var/safestore_db/safestore.pw")).WillOnce(Return("a password"));

    auto mockSafeStoreWrapper = std::make_unique<StrictMock<MockISafeStoreWrapper>>();
    EXPECT_CALL(*mockSafeStoreWrapper, initialise("/tmp/av/var/safestore_db", "safestore.db", "a password"))
        .WillOnce(Return(safestore::InitReturnCode::DB_OPEN_FAILED));
    std::shared_ptr<safestore::IQuarantineManager> quarantineManager =
        std::make_shared<safestore::QuarantineManagerImpl>(std::move(mockSafeStoreWrapper));

    ASSERT_EQ(quarantineManager->getState(), safestore::QuarantineManagerState::UNINITIALISED);
    EXPECT_NO_THROW(quarantineManager->initialise());
    ASSERT_EQ(quarantineManager->getState(), safestore::QuarantineManagerState::UNINITIALISED);
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

    auto mockSafeStoreWrapper = std::make_unique<StrictMock<MockISafeStoreWrapper>>();
    EXPECT_CALL(*mockSafeStoreWrapper, initialise("/tmp/av/var/safestore_db", "safestore.db", "a password"))
        .WillRepeatedly(Return(safestore::InitReturnCode::DB_ERROR));
    std::shared_ptr<safestore::IQuarantineManager> quarantineManager =
        std::make_shared<safestore::QuarantineManagerImpl>(std::move(mockSafeStoreWrapper));

    ASSERT_EQ(quarantineManager->getState(), safestore::QuarantineManagerState::UNINITIALISED);

    for (int i = 0; i < 10; ++i)
    {
        EXPECT_NO_THROW(quarantineManager->initialise());
    }
    ASSERT_EQ(quarantineManager->getState(), safestore::QuarantineManagerState::CORRUPT);
}

TEST_F(QuarantineManagerTests, initFailsOnDBOpenFailureAndDbIsMarkedCorrupt)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };
    addCommonPersistValueExpects(*filesystemMock);
    EXPECT_CALL(*filesystemMock, isFile("/tmp/av/var/safestore_db/safestore.pw")).WillRepeatedly(Return(true));
    EXPECT_CALL(*filesystemMock, readFile("/tmp/av/var/safestore_db/safestore.pw"))
        .WillRepeatedly(Return("a password"));

    auto mockSafeStoreWrapper = std::make_unique<StrictMock<MockISafeStoreWrapper>>();
    EXPECT_CALL(*mockSafeStoreWrapper, initialise("/tmp/av/var/safestore_db", "safestore.db", "a password"))
        .WillRepeatedly(Return(safestore::InitReturnCode::DB_OPEN_FAILED));
    std::shared_ptr<safestore::IQuarantineManager> quarantineManager =
        std::make_shared<safestore::QuarantineManagerImpl>(std::move(mockSafeStoreWrapper));

    ASSERT_EQ(quarantineManager->getState(), safestore::QuarantineManagerState::UNINITIALISED);

    for (int i = 0; i < 10; ++i)
    {
        EXPECT_NO_THROW(quarantineManager->initialise());
    }
    ASSERT_EQ(quarantineManager->getState(), safestore::QuarantineManagerState::CORRUPT);
}

TEST_F(QuarantineManagerTests, quarantineFile)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };
    addCommonPersistValueExpects(*filesystemMock);
    EXPECT_CALL(*filesystemMock, isFile("/tmp/av/var/safestore_db/safestore.pw")).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile("/tmp/av/var/safestore_db/safestore.pw")).WillOnce(Return("a password"));
    EXPECT_CALL(*filesystemMock, compareFileDescriptors(_,_)).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, removeFileOrDirectory("/dir/file"));
    EXPECT_CALL(*filesystemMock, getDirectoryFD(_)).WillOnce(Return(100));
    EXPECT_CALL(*filesystemMock, getFDFromDirectoryFD(_,_)).WillOnce(Return(120));

    auto mockSafeStoreWrapper = std::make_unique<StrictMock<MockISafeStoreWrapper>>();
    EXPECT_CALL(*mockSafeStoreWrapper, initialise("/tmp/av/var/safestore_db", "safestore.db", "a password"))
        .WillOnce(Return(safestore::InitReturnCode::OK));

    EXPECT_CALL(*mockSafeStoreWrapper, createObjectHandleHolder())
        .WillOnce(Return(ByMove(std::make_unique<safestore::ObjectHandleHolder>(*mockSafeStoreWrapper))));

    EXPECT_CALL(*mockSafeStoreWrapper, saveFile(m_dir, m_file, m_threatID, m_threatName, _))
        .WillOnce(Return(safestore::SaveFileReturnCode::OK));
    EXPECT_CALL(*mockSafeStoreWrapper, setObjectCustomDataString(_, "SHA256", m_SHA256)).WillOnce(Return(true));
    EXPECT_CALL(*mockSafeStoreWrapper, finaliseObject(_)).WillOnce(Return(true));

    std::shared_ptr<safestore::IQuarantineManager> quarantineManager =
        std::make_shared<safestore::QuarantineManagerImpl>(std::move(mockSafeStoreWrapper));

    EXPECT_NO_THROW(quarantineManager->initialise());
    ASSERT_EQ(quarantineManager->getState(), safestore::QuarantineManagerState::INITIALISED);
    datatypes::AutoFd fdHolder;
    ASSERT_EQ(scan_messages::QUARANTINE_SUCCESS,quarantineManager->quarantineFile(
        m_dir + "/" + m_file, m_threatID, m_threatName, m_SHA256, std::move(fdHolder)));
}

TEST_F(QuarantineManagerTests, quarantineFileFailsWhenFileDescriptorsDoNotMatch)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };
    addCommonPersistValueExpects(*filesystemMock);
    EXPECT_CALL(*filesystemMock, isFile("/tmp/av/var/safestore_db/safestore.pw")).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile("/tmp/av/var/safestore_db/safestore.pw")).WillOnce(Return("a password"));
    EXPECT_CALL(*filesystemMock, compareFileDescriptors(_,_)).WillOnce(Return(false));
    EXPECT_CALL(*filesystemMock, getDirectoryFD(_)).WillOnce(Return(100));
    EXPECT_CALL(*filesystemMock, getFDFromDirectoryFD(_,_)).WillOnce(Return(120));


    auto mockSafeStoreWrapper = std::make_unique<StrictMock<MockISafeStoreWrapper>>();
    EXPECT_CALL(*mockSafeStoreWrapper, initialise("/tmp/av/var/safestore_db", "safestore.db", "a password"))
        .WillOnce(Return(InitReturnCode::OK));

    EXPECT_CALL(*mockSafeStoreWrapper, createObjectHandleHolder())
        .WillOnce(Return(ByMove(std::make_unique<ObjectHandleHolder>(*mockSafeStoreWrapper))));

    EXPECT_CALL(*mockSafeStoreWrapper, saveFile(m_dir, m_file, m_threatID, m_threatName, _))
        .WillOnce(Return(SaveFileReturnCode::OK));


    std::shared_ptr<IQuarantineManager> quarantineManager =
        std::make_shared<QuarantineManagerImpl>(std::move(mockSafeStoreWrapper));

    EXPECT_NO_THROW(quarantineManager->initialise());
    ASSERT_EQ(quarantineManager->getState(), QuarantineManagerState::INITIALISED);
    datatypes::AutoFd fdHolder;
    ASSERT_EQ(scan_messages::QUARANTINE_FAIL_TO_DELETE_FILE,quarantineManager->quarantineFile(
                                                     m_dir + "/" + m_file, m_threatID, m_threatName, m_SHA256, std::move(fdHolder)));
}

TEST_F(QuarantineManagerTests, quarantineFileFailsWhenthreatDirectoryDoesNotExist)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };
    addCommonPersistValueExpects(*filesystemMock);
    EXPECT_CALL(*filesystemMock, isFile("/tmp/av/var/safestore_db/safestore.pw")).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile("/tmp/av/var/safestore_db/safestore.pw")).WillOnce(Return("a password"));
    EXPECT_CALL(*filesystemMock, getDirectoryFD(_)).WillOnce(Return(-1));


    auto mockSafeStoreWrapper = std::make_unique<StrictMock<MockISafeStoreWrapper>>();
    EXPECT_CALL(*mockSafeStoreWrapper, initialise("/tmp/av/var/safestore_db", "safestore.db", "a password"))
        .WillOnce(Return(InitReturnCode::OK));

    EXPECT_CALL(*mockSafeStoreWrapper, createObjectHandleHolder())
        .WillOnce(Return(ByMove(std::make_unique<ObjectHandleHolder>(*mockSafeStoreWrapper))));

    EXPECT_CALL(*mockSafeStoreWrapper, saveFile(m_dir, m_file, m_threatID, m_threatName, _))
        .WillOnce(Return(SaveFileReturnCode::OK));


    std::shared_ptr<IQuarantineManager> quarantineManager =
        std::make_shared<QuarantineManagerImpl>(std::move(mockSafeStoreWrapper));

    EXPECT_NO_THROW(quarantineManager->initialise());
    ASSERT_EQ(quarantineManager->getState(), QuarantineManagerState::INITIALISED);
    datatypes::AutoFd fdHolder;
    ASSERT_EQ(scan_messages::QUARANTINE_FAIL_TO_DELETE_FILE,quarantineManager->quarantineFile(
                                                                 m_dir + "/" + m_file, m_threatID, m_threatName, m_SHA256, std::move(fdHolder)));
}

TEST_F(QuarantineManagerTests, quarantineFileFailsWhenthreatDoesNotExist)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };
    addCommonPersistValueExpects(*filesystemMock);
    EXPECT_CALL(*filesystemMock, isFile("/tmp/av/var/safestore_db/safestore.pw")).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile("/tmp/av/var/safestore_db/safestore.pw")).WillOnce(Return("a password"));
    EXPECT_CALL(*filesystemMock, getDirectoryFD(_)).WillOnce(Return(100));
    EXPECT_CALL(*filesystemMock, getFDFromDirectoryFD(_,_)).WillOnce(Return(-1));


    auto mockSafeStoreWrapper = std::make_unique<StrictMock<MockISafeStoreWrapper>>();
    EXPECT_CALL(*mockSafeStoreWrapper, initialise("/tmp/av/var/safestore_db", "safestore.db", "a password"))
        .WillOnce(Return(InitReturnCode::OK));

    EXPECT_CALL(*mockSafeStoreWrapper, createObjectHandleHolder())
        .WillOnce(Return(ByMove(std::make_unique<ObjectHandleHolder>(*mockSafeStoreWrapper))));

    EXPECT_CALL(*mockSafeStoreWrapper, saveFile(m_dir, m_file, m_threatID, m_threatName, _))
        .WillOnce(Return(SaveFileReturnCode::OK));


    std::shared_ptr<IQuarantineManager> quarantineManager =
        std::make_shared<QuarantineManagerImpl>(std::move(mockSafeStoreWrapper));

    EXPECT_NO_THROW(quarantineManager->initialise());
    ASSERT_EQ(quarantineManager->getState(), QuarantineManagerState::INITIALISED);
    datatypes::AutoFd fdHolder;
    ASSERT_EQ(scan_messages::QUARANTINE_FAIL_TO_DELETE_FILE,quarantineManager->quarantineFile(
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

    EXPECT_CALL(*filesystemMock, isFile("/tmp/av/var/safestore_db/safestore.pw")).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile("/tmp/av/var/safestore_db/safestore.pw")).WillOnce(Return("a password"));

    auto mockSafeStoreWrapper = std::make_unique<StrictMock<MockISafeStoreWrapper>>();
    EXPECT_CALL(*mockSafeStoreWrapper, initialise("/tmp/av/var/safestore_db", "safestore.db", "a password"))
        .WillRepeatedly(Return(safestore::InitReturnCode::OK));

    EXPECT_CALL(*mockSafeStoreWrapper, saveFile(m_dir, m_file, m_threatID, m_threatName, _))
        .WillRepeatedly(Return(safestore::SaveFileReturnCode::DB_ERROR));
    EXPECT_CALL(*mockSafeStoreWrapper, setObjectCustomDataString(_, "SHA256", m_SHA256)).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockSafeStoreWrapper, finaliseObject(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockSafeStoreWrapper, createObjectHandleHolder())
        .WillOnce(Return(ByMove(std::make_unique<safestore::ObjectHandleHolder>(*mockSafeStoreWrapper))));

    std::shared_ptr<safestore::IQuarantineManager> quarantineManager =
        std::make_shared<safestore::QuarantineManagerImpl>(std::move(mockSafeStoreWrapper));

    EXPECT_NO_THROW(quarantineManager->initialise());
    ASSERT_EQ(quarantineManager->getState(), safestore::QuarantineManagerState::INITIALISED);
    datatypes::AutoFd fdHolder;
    ASSERT_EQ(scan_messages::QUARANTINE_FAIL,quarantineManager->quarantineFile(
        m_dir + "/" + m_file, m_threatID, m_threatName, m_SHA256, std::move(fdHolder)));

    ASSERT_EQ(quarantineManager->getState(), safestore::QuarantineManagerState::CORRUPT);
}

TEST_F(QuarantineManagerTests, quarantineFileFailsToFinaliseFile)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };
    addCommonPersistValueExpects(*filesystemMock);
    EXPECT_CALL(*filesystemMock, isFile("/tmp/av/var/safestore_db/safestore.pw")).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile("/tmp/av/var/safestore_db/safestore.pw")).WillOnce(Return("a password"));
    EXPECT_CALL(*filesystemMock, compareFileDescriptors(_,_)).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, removeFileOrDirectory(_));
    EXPECT_CALL(*filesystemMock, getDirectoryFD(_)).WillOnce(Return(100));
    EXPECT_CALL(*filesystemMock, getFDFromDirectoryFD(_,_)).WillOnce(Return(120));

    auto mockSafeStoreWrapper = std::make_unique<StrictMock<MockISafeStoreWrapper>>();
    EXPECT_CALL(*mockSafeStoreWrapper, initialise("/tmp/av/var/safestore_db", "safestore.db", "a password"))
        .WillOnce(Return(safestore::InitReturnCode::OK));

    EXPECT_CALL(*mockSafeStoreWrapper, createObjectHandleHolder())
        .WillOnce(Return(ByMove(std::make_unique<safestore::ObjectHandleHolder>(*mockSafeStoreWrapper))));

    EXPECT_CALL(*mockSafeStoreWrapper, saveFile(m_dir, m_file, m_threatID, m_threatName, _))
        .WillOnce(Return(safestore::SaveFileReturnCode::OK));
    EXPECT_CALL(*mockSafeStoreWrapper, setObjectCustomDataString(_, "SHA256", m_SHA256)).WillOnce(Return(true));
    EXPECT_CALL(*mockSafeStoreWrapper, finaliseObject(_)).WillOnce(Return(false));

    std::shared_ptr<safestore::IQuarantineManager> quarantineManager =
        std::make_shared<safestore::QuarantineManagerImpl>(std::move(mockSafeStoreWrapper));

    EXPECT_NO_THROW(quarantineManager->initialise());
    ASSERT_EQ(quarantineManager->getState(), safestore::QuarantineManagerState::INITIALISED);
    datatypes::AutoFd fdHolder;
    ASSERT_EQ(scan_messages::QUARANTINE_FAIL,quarantineManager->quarantineFile(
        m_dir + "/" + m_file, m_threatID, m_threatName, m_SHA256, std::move(fdHolder)));
}

TEST_F(QuarantineManagerTests, tryToQuarantineFileWhenThreatIdIsIncorrectSize)
{
    std::string threatId = "Tfe8974b97b4b7a6a33b4c52acb4ffba0c11ebbf208a519245791ad32a96227d8";

    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };
    addCommonPersistValueExpects(*filesystemMock);
    EXPECT_CALL(*filesystemMock, isFile("/tmp/av/var/safestore_db/safestore.pw")).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile("/tmp/av/var/safestore_db/safestore.pw")).WillOnce(Return("a password"));

    auto mockSafeStoreWrapper = std::make_unique<StrictMock<MockISafeStoreWrapper>>();
    EXPECT_CALL(*mockSafeStoreWrapper, initialise("/tmp/av/var/safestore_db", "safestore.db", "a password"))
        .WillOnce(Return(safestore::InitReturnCode::OK));

    std::shared_ptr<safestore::IQuarantineManager> quarantineManager =
        std::make_shared<safestore::QuarantineManagerImpl>(std::move(mockSafeStoreWrapper));
    EXPECT_NO_THROW(quarantineManager->initialise());
    ASSERT_EQ(quarantineManager->getState(), safestore::QuarantineManagerState::INITIALISED);
    datatypes::AutoFd fdHolder;
    ASSERT_EQ(scan_messages::QUARANTINE_FAIL,
        quarantineManager->quarantineFile(m_dir + "/" + m_file, threatId, m_threatName, m_SHA256, std::move(fdHolder)));
}

TEST_F(QuarantineManagerTests, tryToQuarantineFileWhenUninitialised)
{
    auto mockSafeStoreWrapper = std::make_unique<StrictMock<MockISafeStoreWrapper>>();
    EXPECT_CALL(*mockSafeStoreWrapper, saveFile(m_dir, m_file, m_threatID, m_threatName, _)).Times(0);

    std::shared_ptr<safestore::IQuarantineManager> quarantineManager =
        std::make_shared<safestore::QuarantineManagerImpl>(std::move(mockSafeStoreWrapper));

    ASSERT_EQ(quarantineManager->getState(), safestore::QuarantineManagerState::UNINITIALISED);
    datatypes::AutoFd fdHolder;
    scan_messages::QuarantineResult result;
    EXPECT_NO_THROW(
        result = quarantineManager->quarantineFile(
            m_dir + "/" + m_file, m_threatID, m_threatName, m_SHA256, std::move(fdHolder)));
    ASSERT_EQ(result,scan_messages::QUARANTINE_FAIL);
}

TEST_F(QuarantineManagerTests, deleteDatabaseCalledOnInitialisedDb)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };
    addCommonPersistValueExpects(*filesystemMock);
    EXPECT_CALL(*filesystemMock, isFile("/tmp/av/var/safestore_db/safestore.pw")).WillOnce(Return(false));
    EXPECT_CALL(*filesystemMock, readFile("/tmp/av/var/safestore_db/safestore.pw")).Times(0);
    EXPECT_CALL(*filesystemMock, isDirectory("/tmp/av/var/safestore_db")).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, writeFile("/tmp/av/var/safestore_db/safestore.pw", _)).Times(1);
    EXPECT_CALL(*filesystemMock, exists("/tmp/av/var/safestore_db")).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, removeFilesInDirectory("/tmp/av/var/safestore_db")).Times(1);

    auto mockSafeStoreWrapper = std::make_unique<StrictMock<MockISafeStoreWrapper>>();
    EXPECT_CALL(*mockSafeStoreWrapper, initialise("/tmp/av/var/safestore_db", "safestore.db", _))
        .WillOnce(Return(safestore::InitReturnCode::OK));
    std::shared_ptr<safestore::IQuarantineManager> quarantineManager =
        std::make_shared<safestore::QuarantineManagerImpl>(std::move(mockSafeStoreWrapper));

    EXPECT_NO_THROW(quarantineManager->initialise());
    ASSERT_EQ(quarantineManager->getState(), safestore::QuarantineManagerState::INITIALISED);
    EXPECT_NO_THROW(quarantineManager->deleteDatabase());
}

TEST_F(QuarantineManagerTests, deleteDatabaseCalledOnUninitialisedDbThatDoesNotExist)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };
    addCommonPersistValueExpects(*filesystemMock);
    EXPECT_CALL(*filesystemMock, exists("/tmp/av/var/safestore_db")).WillOnce(Return(false));

    auto mockSafeStoreWrapper = std::make_unique<StrictMock<MockISafeStoreWrapper>>();

    std::shared_ptr<safestore::IQuarantineManager> quarantineManager =
        std::make_shared<safestore::QuarantineManagerImpl>(std::move(mockSafeStoreWrapper));

    ASSERT_EQ(quarantineManager->getState(), safestore::QuarantineManagerState::UNINITIALISED);
    EXPECT_NO_THROW(quarantineManager->deleteDatabase());
}

TEST_F(QuarantineManagerTests, deleteDatabaseCalledOnUninitialisedDbThatDoesExist)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };
    addCommonPersistValueExpects(*filesystemMock);
    EXPECT_CALL(*filesystemMock, exists("/tmp/av/var/safestore_db")).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, removeFilesInDirectory("/tmp/av/var/safestore_db")).Times(1);

    auto mockSafeStoreWrapper = std::make_unique<StrictMock<MockISafeStoreWrapper>>();

    std::shared_ptr<safestore::IQuarantineManager> quarantineManager =
        std::make_shared<safestore::QuarantineManagerImpl>(std::move(mockSafeStoreWrapper));

    ASSERT_EQ(quarantineManager->getState(), safestore::QuarantineManagerState::UNINITIALISED);
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

    auto mockSafeStoreWrapper = std::make_unique<StrictMock<MockISafeStoreWrapper>>();

    std::shared_ptr<safestore::IQuarantineManager> quarantineManager =
        std::make_shared<safestore::QuarantineManagerImpl>(std::move(mockSafeStoreWrapper));

    bool result = true;
    EXPECT_NO_THROW(result = quarantineManager->deleteDatabase());
    ASSERT_FALSE(result);
}