// Copyright 2019-2022, Sophos Limited.  All rights reserved.

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
