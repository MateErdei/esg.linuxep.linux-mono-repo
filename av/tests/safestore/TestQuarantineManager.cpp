// Copyright 2022, Sophos Limited.  All rights reserved.

#include "MockISafeStoreWrapper.h"

#include "../../redist/pluginapi/tests/include/Common/Helpers/FileSystemReplaceAndRestore.h"
#include "../../redist/pluginapi/tests/include/Common/Helpers/MockFileSystem.h"
#include "../common/LogInitializedTests.h"
#include "safestore/IQuarantineManager.h"
#include "safestore/QuarantineManagerImpl.h"

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
#include "Common/FileSystem/IFileSystem.h"
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
};

TEST_F(QuarantineManagerTests, initDbWithExistingPassword)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };
    EXPECT_CALL(*filesystemMock, isFile("/tmp/av/var/safestore_db/safestore.pw")).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile("/tmp/av/var/safestore_db/safestore.pw")).WillOnce(Return("a password"));

    auto mockSafeStoreWrapper = std::make_shared<StrictMock<MockISafeStoreWrapper>>();
    EXPECT_CALL(*mockSafeStoreWrapper, initialise("/tmp/av/var/safestore_db", "safestore.db", "a password"))
        .WillOnce(Return(safestore::InitReturnCode::OK));
    std::shared_ptr<safestore::IQuarantineManager> quarantineManager =
        std::make_shared<safestore::QuarantineManagerImpl>(mockSafeStoreWrapper);

    EXPECT_NO_THROW(quarantineManager->initialise());
}

TEST_F(QuarantineManagerTests, initDbAndGeneratePassword)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };
    EXPECT_CALL(*filesystemMock, isFile("/tmp/av/var/safestore_db/safestore.pw")).WillOnce(Return(false));
    EXPECT_CALL(*filesystemMock, readFile("/tmp/av/var/safestore_db/safestore.pw")).Times(0);
    EXPECT_CALL(*filesystemMock, isDirectory("/tmp/av/var/safestore_db")).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, writeFile("/tmp/av/var/safestore_db/safestore.pw", _)).Times(1);

    auto mockSafeStoreWrapper = std::make_shared<StrictMock<MockISafeStoreWrapper>>();
    EXPECT_CALL(*mockSafeStoreWrapper, initialise("/tmp/av/var/safestore_db", "safestore.db", _))
        .WillOnce(Return(safestore::InitReturnCode::OK));
    std::shared_ptr<safestore::IQuarantineManager> quarantineManager =
        std::make_shared<safestore::QuarantineManagerImpl>(mockSafeStoreWrapper);

    EXPECT_NO_THROW(quarantineManager->initialise());
}

TEST_F(QuarantineManagerTests, uninitialisedDbStateIsInitialisedAfterSuccessfulInitCall)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };
    EXPECT_CALL(*filesystemMock, isFile("/tmp/av/var/safestore_db/safestore.pw")).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile("/tmp/av/var/safestore_db/safestore.pw")).WillOnce(Return("a password"));

    auto mockSafeStoreWrapper = std::make_shared<StrictMock<MockISafeStoreWrapper>>();
    EXPECT_CALL(*mockSafeStoreWrapper, initialise("/tmp/av/var/safestore_db", "safestore.db", "a password"))
        .WillOnce(Return(safestore::InitReturnCode::OK));
    std::shared_ptr<safestore::IQuarantineManager> quarantineManager =
        std::make_shared<safestore::QuarantineManagerImpl>(mockSafeStoreWrapper);

    ASSERT_EQ(quarantineManager->getState(), safestore::QuarantineManagerState::UNINITIALISED);
    EXPECT_NO_THROW(quarantineManager->initialise());
    ASSERT_EQ(quarantineManager->getState(), safestore::QuarantineManagerState::INITIALISED);
}

TEST_F(QuarantineManagerTests, uninitialisedDbStateIsStillUnitialisedAfterFailedInitCall)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };
    EXPECT_CALL(*filesystemMock, isFile("/tmp/av/var/safestore_db/safestore.pw")).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile("/tmp/av/var/safestore_db/safestore.pw")).WillOnce(Return("a password"));

    auto mockSafeStoreWrapper = std::make_shared<StrictMock<MockISafeStoreWrapper>>();
    EXPECT_CALL(*mockSafeStoreWrapper, initialise("/tmp/av/var/safestore_db", "safestore.db", "a password"))
        .WillOnce(Return(safestore::InitReturnCode::DB_OPEN_FAILED));
    std::shared_ptr<safestore::IQuarantineManager> quarantineManager =
        std::make_shared<safestore::QuarantineManagerImpl>(mockSafeStoreWrapper);

    ASSERT_EQ(quarantineManager->getState(), safestore::QuarantineManagerState::UNINITIALISED);
    EXPECT_NO_THROW(quarantineManager->initialise());
    ASSERT_EQ(quarantineManager->getState(), safestore::QuarantineManagerState::UNINITIALISED);
}

TEST_F(QuarantineManagerTests, quarantineFile)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };
    EXPECT_CALL(*filesystemMock, isFile("/tmp/av/var/safestore_db/safestore.pw")).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile("/tmp/av/var/safestore_db/safestore.pw")).WillOnce(Return("a password"));

    auto mockSafeStoreWrapper = std::make_shared<StrictMock<MockISafeStoreWrapper>>();
    EXPECT_CALL(*mockSafeStoreWrapper, initialise("/tmp/av/var/safestore_db", "safestore.db", "a password"))
        .WillOnce(Return(safestore::InitReturnCode::OK));

    std::string dir = "/dir";
    std::string file = "file";
    std::string threatID = "abcdefghijklmnop"; // 16 bytes
    std::string threatName = "threatName";
    std::string SHA256 = "SHA256abcdef";

    // TODO - fix - can't pass this in to the expect call because it can't be copy constructed?
    //    safestore::ObjectHandleHolder objectHandle(*mockSafeStoreWrapper);
    EXPECT_CALL(*mockSafeStoreWrapper, saveFile(dir, file, threatID, threatName, _))
        .WillOnce(Return(safestore::SaveFileReturnCode::OK));
    EXPECT_CALL(*mockSafeStoreWrapper, setObjectCustomDataString(_, "SHA256", SHA256)).WillOnce(Return(true));
    EXPECT_CALL(*mockSafeStoreWrapper, finaliseObject(_)).WillOnce(Return(true));

    std::shared_ptr<safestore::IQuarantineManager> quarantineManager =
        std::make_shared<safestore::QuarantineManagerImpl>(mockSafeStoreWrapper);

    EXPECT_NO_THROW(quarantineManager->initialise());
    ASSERT_EQ(quarantineManager->getState(), safestore::QuarantineManagerState::INITIALISED);
    datatypes::AutoFd fdHolder;
    ASSERT_TRUE(quarantineManager->quarantineFile(dir + "/" + file, threatID, threatName, SHA256, std::move(fdHolder)));
}

TEST_F(QuarantineManagerTests, quartineFileFailsAndDbIsMarkedCorrupt)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };
    EXPECT_CALL(*filesystemMock, isFile("/tmp/av/var/safestore_db/safestore.pw")).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile("/tmp/av/var/safestore_db/safestore.pw")).WillOnce(Return("a password"));

    auto mockSafeStoreWrapper = std::make_shared<StrictMock<MockISafeStoreWrapper>>();
    EXPECT_CALL(*mockSafeStoreWrapper, initialise("/tmp/av/var/safestore_db", "safestore.db", "a password"))
        .WillRepeatedly(Return(safestore::InitReturnCode::OK));

    std::string dir = "/dir";
    std::string file = "file";
    std::string threatID = "abcdefghijklmnop";
    std::string threatName = "threatName";
    std::string SHA256 = "SHA256abcdef";

    EXPECT_CALL(*mockSafeStoreWrapper, saveFile(dir, file, threatID, threatName, _))
        .WillRepeatedly(Return(safestore::SaveFileReturnCode::DB_ERROR));
    EXPECT_CALL(*mockSafeStoreWrapper, setObjectCustomDataString(_, "SHA256", SHA256)).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockSafeStoreWrapper, finaliseObject(_)).WillRepeatedly(Return(true));

    std::shared_ptr<safestore::IQuarantineManager> quarantineManager =
        std::make_shared<safestore::QuarantineManagerImpl>(mockSafeStoreWrapper);

    EXPECT_NO_THROW(quarantineManager->initialise());
    ASSERT_EQ(quarantineManager->getState(), safestore::QuarantineManagerState::INITIALISED);
    datatypes::AutoFd fdHolder;
    for (int i = 0; i < 11; ++i)
        ASSERT_FALSE(
            quarantineManager->quarantineFile(dir + "/" + file, threatID, threatName, SHA256, std::move(fdHolder)));
    ASSERT_EQ(quarantineManager->getState(), safestore::QuarantineManagerState::CORRUPT);
}
