// Copyright 2022, Sophos Limited.  All rights reserved.

#include "MockISafeStoreWrapper.h"

#include "../common/LogInitializedTests.h"
#include "safestore/QuarantineManagerImpl.h"
#include "safestore/SafeStoreWrapperImpl.h"
#include "safestore/StateMonitor.h"

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
#include "Common/Helpers/FileSystemReplaceAndRestore.h"
#include "Common/Helpers/MockFileSystem.h"

#include <gtest/gtest.h>

#include <utility>

class StateMonitorTests : public LogInitializedTests
{
    virtual void SetUp()
    {
        auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
        appConfig.setData("SOPHOS_INSTALL", "/tmp");
        appConfig.setData("PLUGIN_INSTALL", "/tmp/av");
    }
};

class TestableStateMonitor : public safestore::StateMonitor
{
public:
    TestableStateMonitor(std::shared_ptr<safestore::IQuarantineManager> quarantineManager) :
        safestore::StateMonitor(std::move(quarantineManager))
    {
    }

    void callInnerRun()
    {
        innerRun();
    }
};

TEST_F(StateMonitorTests, stateMonitorDoesNotReinitialiseQuarantineManagerWhenAlreadyInitialised)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };

    EXPECT_CALL(*filesystemMock, removeFile("/tmp/av/var/safestore_dormant_flag", true)).WillOnce(Return());
    EXPECT_CALL(*filesystemMock, exists("/tmp/av/var/persist-safeStoreDbErrorThreshold")).WillOnce(Return(false));
    EXPECT_CALL(*filesystemMock, writeFile("/tmp/av/var/persist-safeStoreDbErrorThreshold", "10"));
    EXPECT_CALL(*filesystemMock, isFile("/tmp/av/var/safestore_db/safestore.pw")).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile("/tmp/av/var/safestore_db/safestore.pw")).WillOnce(Return("password"));

    auto mockSafeStoreWrapper = std::make_unique<StrictMock<MockISafeStoreWrapper>>();
    // This should only be called once on the quarantineManager->initialise() call and then the statemonitor should
    // not try to re-initialise quarantineManager.
    EXPECT_CALL(*mockSafeStoreWrapper, initialise("/tmp/av/var/safestore_db", "safestore.db", "password"))
        .Times(1)
        .WillOnce(Return(safestore::InitReturnCode::OK));
    std::shared_ptr<safestore::IQuarantineManager> quarantineManager =
        std::make_shared<safestore::QuarantineManagerImpl>(std::move(mockSafeStoreWrapper));
    quarantineManager->initialise();
    TestableStateMonitor stateMonitor = TestableStateMonitor(quarantineManager);
    stateMonitor.callInnerRun();
}

TEST_F(StateMonitorTests, stateMonitorInitialisesQuarantineManagerWhenQuarantineManagerIsNotInitialised)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };

    EXPECT_CALL(*filesystemMock, exists("/tmp/av/var/persist-safeStoreDbErrorThreshold")).WillOnce(Return(false));
    EXPECT_CALL(*filesystemMock, writeFile("/tmp/av/var/persist-safeStoreDbErrorThreshold", "10"));
    EXPECT_CALL(*filesystemMock, isFile("/tmp/av/var/safestore_db/safestore.pw")).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile("/tmp/av/var/safestore_db/safestore.pw")).WillOnce(Return("password"));
    EXPECT_CALL(*filesystemMock, removeFile("/tmp/av/var/safestore_dormant_flag", true)).WillOnce(Return());

    auto mockSafeStoreWrapper = std::make_unique<StrictMock<MockISafeStoreWrapper>>();
    EXPECT_CALL(*mockSafeStoreWrapper, initialise("/tmp/av/var/safestore_db", "safestore.db", "password"))
        .Times(1)
        .WillOnce(Return(safestore::InitReturnCode::OK));
    std::shared_ptr<safestore::IQuarantineManager> quarantineManager =
        std::make_shared<safestore::QuarantineManagerImpl>(std::move(mockSafeStoreWrapper));
    TestableStateMonitor stateMonitor = TestableStateMonitor(quarantineManager);
    ASSERT_EQ(quarantineManager->getState(), safestore::QuarantineManagerState::STARTUP);
    stateMonitor.callInnerRun();
    ASSERT_EQ(quarantineManager->getState(), safestore::QuarantineManagerState::INITIALISED); //CURRENTLY THINKS IN STARTUP
}

TEST_F(StateMonitorTests, stateMonitorReinitialisesQuarantineManagerWhenQuarantineManagerIsCorrupt)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };

    // Force the m_dbErrorCountThreshold variable to be set to 1 for tests so that we only have to see one error
    // before the database is reported to be corrupt.
    EXPECT_CALL(*filesystemMock, exists("/tmp/av/var/persist-safeStoreDbErrorThreshold")).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, writeFile("/tmp/av/var/persist-safeStoreDbErrorThreshold", "1"));
    EXPECT_CALL(*filesystemMock, readFile("/tmp/av/var/persist-safeStoreDbErrorThreshold")).WillOnce(Return("1"));
    EXPECT_CALL(*filesystemMock, writeFileAtomically("/tmp/av/var/safestore_dormant_flag", "Safestore database uninitialised", "/tmp/tmp")).Times(2);
    EXPECT_CALL(*filesystemMock, writeFileAtomically("/tmp/av/var/safestore_dormant_flag", "Safestore database corrupt", "/tmp/tmp"));

    EXPECT_CALL(*filesystemMock, isFile("/tmp/av/var/safestore_db/safestore.pw")).WillRepeatedly(Return(true));
    EXPECT_CALL(*filesystemMock, isFile("/tmp/av/var/safestore_db/safestore.pw")).WillRepeatedly(Return(true));
    EXPECT_CALL(*filesystemMock, readFile("/tmp/av/var/safestore_db/safestore.pw")).WillRepeatedly(Return("password"));
    EXPECT_CALL(*filesystemMock, removeFile("/tmp/av/var/safestore_dormant_flag", true)).WillOnce(Return());

    EXPECT_CALL(*filesystemMock, exists("/tmp/av/var/safestore_db")).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, removeFilesInDirectory("/tmp/av/var/safestore_db")).Times(1);

    auto mockSafeStoreWrapper = std::make_unique<StrictMock<MockISafeStoreWrapper>>();
    EXPECT_CALL(*mockSafeStoreWrapper, initialise("/tmp/av/var/safestore_db", "safestore.db", "password"))
        .WillOnce(Return(safestore::InitReturnCode::DB_ERROR))
        .WillOnce(Return(safestore::InitReturnCode::OK));
    std::shared_ptr<safestore::IQuarantineManager> quarantineManager =
        std::make_shared<safestore::QuarantineManagerImpl>(std::move(mockSafeStoreWrapper));

    TestableStateMonitor stateMonitor = TestableStateMonitor(quarantineManager);
    ASSERT_EQ(quarantineManager->getState(), safestore::QuarantineManagerState::STARTUP);
    quarantineManager->initialise();
    ASSERT_EQ(quarantineManager->getState(), safestore::QuarantineManagerState::CORRUPT);
    stateMonitor.callInnerRun();
    ASSERT_EQ(quarantineManager->getState(), safestore::QuarantineManagerState::INITIALISED);
}