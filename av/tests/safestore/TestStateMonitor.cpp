// Copyright 2022, Sophos Limited.  All rights reserved.

#include "MockISafeStoreWrapper.h"

#include "../common/LogInitializedTests.h"
#include "safestore/QuarantineManager/QuarantineManagerImpl.h"
#include "safestore/QuarantineManager/StateMonitor.h"
#include "safestore/SafeStoreWrapper/SafeStoreWrapperImpl.h"

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
#include "Common/Helpers/FileSystemReplaceAndRestore.h"
#include "Common/Helpers/MockFileSystem.h"

#include "Common/Logging/ConsoleLoggingSetup.h"

#include <gtest/gtest.h>

#include <utility>

using namespace safestore::QuarantineManager;
using namespace safestore::SafeStoreWrapper;

class StateMonitorTests : public LogInitializedTests
{
public:
    void SetUp() override
    {
        auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
        appConfig.setData("SOPHOS_INSTALL", "/tmp");
        appConfig.setData("PLUGIN_INSTALL", "/tmp/av");
        m_mockSafeStoreWrapper = std::make_unique<StrictMock<MockISafeStoreWrapper>>();
    }

    std::unique_ptr<StrictMock<MockISafeStoreWrapper>> m_mockSafeStoreWrapper;
};

class TestableStateMonitor : public StateMonitor
{
public:
    explicit TestableStateMonitor(std::shared_ptr<IQuarantineManager> quarantineManager) :
        StateMonitor(std::move(quarantineManager))
    {
    }

    void callInnerRun()
    {
        innerRun();
    }

    void callRun()
    {
        run();
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

    // This should only be called once on the quarantineManager->initialise() call and then the statemonitor should
    // not try to re-initialise quarantineManager.
    EXPECT_CALL(*m_mockSafeStoreWrapper, initialise("/tmp/av/var/safestore_db", "safestore.db", "password"))
        .Times(1)
        .WillOnce(Return(InitReturnCode::OK));
    std::shared_ptr<IQuarantineManager> quarantineManager =
        std::make_shared<QuarantineManagerImpl>(std::move(m_mockSafeStoreWrapper));
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

    EXPECT_CALL(*m_mockSafeStoreWrapper, initialise("/tmp/av/var/safestore_db", "safestore.db", "password"))
        .Times(1)
        .WillOnce(Return(InitReturnCode::OK));
    std::shared_ptr<IQuarantineManager> quarantineManager =
        std::make_shared<QuarantineManagerImpl>(std::move(m_mockSafeStoreWrapper));
    TestableStateMonitor stateMonitor = TestableStateMonitor(quarantineManager);
    ASSERT_EQ(quarantineManager->getState(), QuarantineManagerState::STARTUP);
    stateMonitor.callInnerRun();
    ASSERT_EQ(quarantineManager->getState(), QuarantineManagerState::INITIALISED);
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
    EXPECT_CALL(
        *filesystemMock,
        writeFileAtomically("/tmp/av/var/safestore_dormant_flag", "SafeStore database uninitialised", "/tmp/tmp"))
        .Times(2);
    EXPECT_CALL(
        *filesystemMock,
        writeFileAtomically("/tmp/av/var/safestore_dormant_flag", "SafeStore database corrupt", "/tmp/tmp"));

    EXPECT_CALL(*filesystemMock, isFile("/tmp/av/var/safestore_db/safestore.pw")).WillRepeatedly(Return(true));
    EXPECT_CALL(*filesystemMock, isFile("/tmp/av/var/safestore_db/safestore.pw")).WillRepeatedly(Return(true));
    EXPECT_CALL(*filesystemMock, readFile("/tmp/av/var/safestore_db/safestore.pw")).WillRepeatedly(Return("password"));
    EXPECT_CALL(*filesystemMock, removeFile("/tmp/av/var/safestore_dormant_flag", true)).WillOnce(Return());

    EXPECT_CALL(*filesystemMock, exists("/tmp/av/var/safestore_db")).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, removeFilesInDirectory("/tmp/av/var/safestore_db")).Times(1);

    EXPECT_CALL(*m_mockSafeStoreWrapper, initialise("/tmp/av/var/safestore_db", "safestore.db", "password"))
        .WillOnce(Return(InitReturnCode::DB_ERROR))
        .WillOnce(Return(InitReturnCode::OK));
    std::shared_ptr<IQuarantineManager> quarantineManager =
        std::make_shared<QuarantineManagerImpl>(std::move(m_mockSafeStoreWrapper));

    TestableStateMonitor stateMonitor = TestableStateMonitor(quarantineManager);
    ASSERT_EQ(quarantineManager->getState(), QuarantineManagerState::STARTUP);
    quarantineManager->initialise();
    ASSERT_EQ(quarantineManager->getState(), QuarantineManagerState::CORRUPT);
    stateMonitor.callInnerRun();
    ASSERT_EQ(quarantineManager->getState(), QuarantineManagerState::INITIALISED);
}

TEST_F(StateMonitorTests, testStateMonitorExitsOnDestructDuringWait)
{
    testing::internal::CaptureStderr();

    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };

    std::shared_ptr<IQuarantineManager> quarantineManager =
        std::make_shared<QuarantineManagerImpl>(std::move(m_mockSafeStoreWrapper));

    EXPECT_CALL(*filesystemMock, exists("/tmp/av/var/persist-safeStoreDbErrorThreshold")).WillOnce(Return(false));
    EXPECT_CALL(*filesystemMock, writeFile("/tmp/av/var/persist-safeStoreDbErrorThreshold", "1"));

    //bool called = false;
    {
        TestableStateMonitor stateMonitor = TestableStateMonitor(quarantineManager);
        stateMonitor.callRun();
    }

    std::string logMessage = internal::GetCapturedStderr();
    ASSERT_THAT(logMessage, ::testing::HasSubstr("State Monitor stop requested"));



}