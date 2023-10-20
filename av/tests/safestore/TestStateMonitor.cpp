// Copyright 2022-2023 Sophos Limited. All rights reserved.

#include "MockISafeStoreWrapper.h"
#include "MockSafeStoreResources.h"

#include "tests/common/MemoryAppender.h"
#include "common/ApplicationPaths.h"
#include "safestore/QuarantineManager/QuarantineManagerImpl.h"
#include "safestore/QuarantineManager/StateMonitor.h"
#include "safestore/SafeStoreWrapper/SafeStoreWrapperImpl.h"

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
#include "Common/Helpers/FileSystemReplaceAndRestore.h"
#include "Common/Helpers/MockFilePermissions.h"
#include "Common/Helpers/MockFileSystem.h"
#include "Common/Helpers/MockSysCalls.h"
#include "Common/Logging/ConsoleLoggingSetup.h"

#include <gtest/gtest.h>

#include <utility>

using namespace safestore::QuarantineManager;
using namespace safestore::SafeStoreWrapper;

class StateMonitorTests : public MemoryAppenderUsingTests
{
public:
    StateMonitorTests() : MemoryAppenderUsingTests("safestore")
    {
    }

    void SetUp() override
    {
        auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
        appConfig.setData("SOPHOS_INSTALL", "/tmp");
        appConfig.setData("PLUGIN_INSTALL", "/tmp/av");
        m_mockSafeStoreWrapper = std::make_unique<StrictMock<MockISafeStoreWrapper>>();
    }

    std::shared_ptr<QuarantineManagerImpl> createQuarantineManager()
    {
        return std::make_shared<QuarantineManagerImpl>(
            std::move(m_mockSafeStoreWrapper), std::move(m_mockSysCallWrapper), mockSafeStoreResources_);
    }

    std::unique_ptr<StrictMock<MockISafeStoreWrapper>> m_mockSafeStoreWrapper;
    std::shared_ptr<StrictMock<MockSystemCallWrapper>> m_mockSysCallWrapper =
        std::make_shared<StrictMock<MockSystemCallWrapper>>();
    MockSafeStoreResources mockSafeStoreResources_;
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

    std::chrono::seconds getCurrentBackoffTime()
    {
        return m_reinitialiseBackoff;
    }

    std::chrono::seconds getMaxBackoffTime()
    {
        return m_maxReinitialiseBackoff;
    }

    void testableBackoffIncrease()
    {
        increaseBackOff();
    }
};

TEST_F(StateMonitorTests, stateMonitorDoesNotReinitialiseQuarantineManagerWhenAlreadyInitialised)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };

    EXPECT_CALL(*filesystemMock, removeFile(Plugin::getSafeStoreDormantFlagPath(), true)).WillOnce(Return());
    EXPECT_CALL(*filesystemMock, exists("/tmp/av/var/persist-safeStoreDbErrorThreshold")).WillOnce(Return(false));
    EXPECT_CALL(*filesystemMock, writeFile("/tmp/av/var/persist-safeStoreDbErrorThreshold", "10"));
    EXPECT_CALL(*filesystemMock, isFile(Plugin::getSafeStorePasswordFilePath())).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(Plugin::getSafeStorePasswordFilePath())).WillOnce(Return("password"));
    EXPECT_CALL(*filesystemMock, isFile(Plugin::getSafeStoreConfigPath())).WillOnce(Return(false));

    // This should only be called once on the quarantineManager->initialise() call and then the statemonitor should
    // not try to re-initialise quarantineManager.
    EXPECT_CALL(
        *m_mockSafeStoreWrapper,
        initialise(Plugin::getSafeStoreDbDirPath(), Plugin::getSafeStoreDbFileName(), "password"))
        .Times(1)
        .WillOnce(Return(InitReturnCode::OK));
    auto quarantineManager = createQuarantineManager();
    quarantineManager->initialise();
    TestableStateMonitor stateMonitor = TestableStateMonitor(quarantineManager);
    stateMonitor.callInnerRun();
}

TEST_F(StateMonitorTests, stateMonitorInitialisesQuarantineManagerWhenQuarantineManagerIsNotInitialised)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };

    EXPECT_CALL(*filesystemMock, exists("/tmp/av/var/persist-safeStoreDbErrorThreshold")).WillOnce(Return(false));
    EXPECT_CALL(*filesystemMock, writeFile("/tmp/av/var/persist-safeStoreDbErrorThreshold", "10"));
    EXPECT_CALL(*filesystemMock, isFile(Plugin::getSafeStorePasswordFilePath())).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, isFile(Plugin::getSafeStoreConfigPath())).WillOnce(Return(false));

    EXPECT_CALL(*filesystemMock, readFile(Plugin::getSafeStorePasswordFilePath())).WillOnce(Return("password"));
    EXPECT_CALL(*filesystemMock, removeFile(Plugin::getSafeStoreDormantFlagPath(), true)).WillOnce(Return());

    EXPECT_CALL(
        *m_mockSafeStoreWrapper,
        initialise(Plugin::getSafeStoreDbDirPath(), Plugin::getSafeStoreDbFileName(), "password"))
        .Times(1)
        .WillOnce(Return(InitReturnCode::OK));
    auto quarantineManager = createQuarantineManager();
    TestableStateMonitor stateMonitor = TestableStateMonitor(quarantineManager);
    ASSERT_EQ(quarantineManager->getState(), QuarantineManagerState::STARTUP);
    stateMonitor.callInnerRun();
    ASSERT_EQ(quarantineManager->getState(), QuarantineManagerState::INITIALISED);
}

TEST_F(StateMonitorTests, stateMonitorReinitialisesQuarantineManagerWhenQuarantineManagerIsCorrupt)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };

    // Force the m_dbErrorCountThreshold variable to be set to 1 for tests so that we only have to see one error
    // before the database is reported to be corrupt.
    EXPECT_CALL(*filesystemMock, exists("/tmp/av/var/persist-safeStoreDbErrorThreshold")).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, writeFile("/tmp/av/var/persist-safeStoreDbErrorThreshold", "1"));
    EXPECT_CALL(*filesystemMock, readFile("/tmp/av/var/persist-safeStoreDbErrorThreshold")).WillOnce(Return("1"));
    EXPECT_CALL(
        *filesystemMock,
        writeFileAtomically(Plugin::getSafeStoreDormantFlagPath(), "SafeStore database uninitialised", "/tmp/tmp"))
        .Times(2);

    // 2nd one here because we call init twice when deleting the DB so each of those will set state which writes out
    // the dormant file
    EXPECT_CALL(
        *filesystemMock,
        writeFileAtomically(Plugin::getSafeStoreDormantFlagPath(), "SafeStore database corrupt", "/tmp/tmp"))
        .Times(1);

    EXPECT_CALL(*filesystemMock, isFile(Plugin::getSafeStorePasswordFilePath())).WillRepeatedly(Return(true));
    EXPECT_CALL(*filesystemMock, isFile(Plugin::getSafeStorePasswordFilePath())).WillRepeatedly(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(Plugin::getSafeStorePasswordFilePath())).WillRepeatedly(Return("password"));
    EXPECT_CALL(*filesystemMock, isFile(Plugin::getSafeStoreConfigPath())).WillRepeatedly(Return(false));
    EXPECT_CALL(*filesystemMock, removeFile(Plugin::getSafeStoreDormantFlagPath(), true)).WillOnce(Return());
    EXPECT_CALL(*filesystemMock, exists(Plugin::getSafeStoreDbDirPath())).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, exists(Plugin::getSafeStoreDbLockDirPath())).WillOnce(Return(false));
    EXPECT_CALL(*filesystemMock, removeFileOrDirectory(Plugin::getSafeStoreDbDirPath())).Times(1);
    EXPECT_CALL(*filesystemMock, makedirs(Plugin::getSafeStoreDbDirPath())).Times(1);

    auto* filePermissionsMock = new StrictMock<MockFilePermissions>();
    Tests::ScopedReplaceFilePermissions scopedReplaceFilePermissions{
        std::unique_ptr<Common::FileSystem::IFilePermissions>(filePermissionsMock)
    };

    EXPECT_CALL(*filePermissionsMock, chown(Plugin::getSafeStoreDbDirPath(), "root", "root")).WillOnce(Return());
    EXPECT_CALL(*filePermissionsMock, chmod(Plugin::getSafeStoreDbDirPath(), S_IRUSR | S_IWUSR | S_IXUSR))
        .WillOnce(Return());

    EXPECT_CALL(
        *m_mockSafeStoreWrapper,
        initialise(Plugin::getSafeStoreDbDirPath(), Plugin::getSafeStoreDbFileName(), "password"))
        .WillOnce(Return(InitReturnCode::DB_ERROR))
        .WillOnce(Return(InitReturnCode::OK));
    auto quarantineManager = createQuarantineManager();

    TestableStateMonitor stateMonitor = TestableStateMonitor(quarantineManager);
    ASSERT_EQ(quarantineManager->getState(), QuarantineManagerState::STARTUP);
    quarantineManager->initialise();
    ASSERT_EQ(quarantineManager->getState(), QuarantineManagerState::CORRUPT);
    stateMonitor.callInnerRun();
    ASSERT_EQ(quarantineManager->getState(), QuarantineManagerState::INITIALISED);
}

TEST_F(StateMonitorTests, stateMonitorIncrementsErrorCountOnInternalError)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };

    // Force the m_dbErrorCountThreshold variable to be set to 1 for tests so that we only have to see one error
    // before the database is reported to be corrupt.
    EXPECT_CALL(*filesystemMock, exists("/tmp/av/var/persist-safeStoreDbErrorThreshold")).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, writeFile("/tmp/av/var/persist-safeStoreDbErrorThreshold", "1"));
    EXPECT_CALL(*filesystemMock, readFile("/tmp/av/var/persist-safeStoreDbErrorThreshold")).WillOnce(Return("1"));
    EXPECT_CALL(
        *filesystemMock,
        writeFileAtomically(Plugin::getSafeStoreDormantFlagPath(), "SafeStore database uninitialised", "/tmp/tmp"))
        .Times(2);
    EXPECT_CALL(
        *filesystemMock,
        writeFileAtomically(Plugin::getSafeStoreDormantFlagPath(), "SafeStore database corrupt", "/tmp/tmp"))
        .Times(1);

    EXPECT_CALL(*filesystemMock, exists(Plugin::getSafeStoreDbLockDirPath())).WillOnce(Return(false));
    EXPECT_CALL(*filesystemMock, isFile(Plugin::getSafeStorePasswordFilePath())).WillRepeatedly(Return(true));
    EXPECT_CALL(*filesystemMock, isFile(Plugin::getSafeStorePasswordFilePath())).WillRepeatedly(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(Plugin::getSafeStorePasswordFilePath())).WillRepeatedly(Return("password"));
    EXPECT_CALL(*filesystemMock, isFile(Plugin::getSafeStoreConfigPath())).WillRepeatedly(Return(false));

    EXPECT_CALL(*filesystemMock, removeFile(Plugin::getSafeStoreDormantFlagPath(), true)).WillOnce(Return());

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

    EXPECT_CALL(
        *m_mockSafeStoreWrapper,
        initialise(Plugin::getSafeStoreDbDirPath(), Plugin::getSafeStoreDbFileName(), "password"))
        .WillOnce(Return(InitReturnCode::FAILED))
        .WillOnce(Return(InitReturnCode::OK));
    auto quarantineManager = createQuarantineManager();

    TestableStateMonitor stateMonitor = TestableStateMonitor(quarantineManager);
    ASSERT_EQ(quarantineManager->getState(), QuarantineManagerState::STARTUP);
    quarantineManager->initialise();
    ASSERT_EQ(quarantineManager->getState(), QuarantineManagerState::CORRUPT);
    stateMonitor.callInnerRun();
    ASSERT_EQ(quarantineManager->getState(), QuarantineManagerState::INITIALISED);
}

TEST_F(StateMonitorTests, stateMonitorIgnoresSpecificInitErrorCodes)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };

    // Force the m_dbErrorCountThreshold variable to be set to 1 for tests so that we only have to see one error
    // before the database is reported to be corrupt.
    EXPECT_CALL(*filesystemMock, exists("/tmp/av/var/persist-safeStoreDbErrorThreshold")).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, writeFile("/tmp/av/var/persist-safeStoreDbErrorThreshold", "1"));
    EXPECT_CALL(*filesystemMock, readFile("/tmp/av/var/persist-safeStoreDbErrorThreshold")).WillOnce(Return("1"));
    EXPECT_CALL(*filesystemMock, exists(Plugin::getSafeStoreDbLockDirPath())).WillOnce(Return(false));
    EXPECT_CALL(
        *filesystemMock,
        writeFileAtomically(Plugin::getSafeStoreDormantFlagPath(), "SafeStore database uninitialised", "/tmp/tmp"))
        .Times(2);
    EXPECT_CALL(
        *filesystemMock,
        writeFileAtomically(Plugin::getSafeStoreDormantFlagPath(), "SafeStore database corrupt", "/tmp/tmp"))
        .Times(1);

    EXPECT_CALL(*filesystemMock, isFile(Plugin::getSafeStorePasswordFilePath())).WillRepeatedly(Return(true));
    EXPECT_CALL(*filesystemMock, isFile(Plugin::getSafeStorePasswordFilePath())).WillRepeatedly(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(Plugin::getSafeStorePasswordFilePath())).WillRepeatedly(Return("password"));
    EXPECT_CALL(*filesystemMock, isFile(Plugin::getSafeStoreConfigPath())).WillRepeatedly(Return(false));

    EXPECT_CALL(*filesystemMock, removeFile(Plugin::getSafeStoreDormantFlagPath(), true)).WillOnce(Return());

    EXPECT_CALL(*filesystemMock, exists(Plugin::getSafeStoreDbDirPath())).WillRepeatedly(Return(true));
    EXPECT_CALL(*filesystemMock, removeFileOrDirectory(Plugin::getSafeStoreDbDirPath())).Times(1);
    EXPECT_CALL(*filesystemMock, makedirs(Plugin::getSafeStoreDbDirPath())).Times(1);

    auto* filePermissionsMock = new StrictMock<MockFilePermissions>();
    Tests::ScopedReplaceFilePermissions scopedReplaceFilePermissions{
        std::unique_ptr<Common::FileSystem::IFilePermissions>(filePermissionsMock)
    };

    EXPECT_CALL(*filePermissionsMock, chown(Plugin::getSafeStoreDbDirPath(), "root", "root")).WillOnce(Return());
    EXPECT_CALL(*filePermissionsMock, chmod(Plugin::getSafeStoreDbDirPath(), S_IRUSR | S_IWUSR | S_IXUSR))
        .WillOnce(Return());

    EXPECT_CALL(
        *m_mockSafeStoreWrapper,
        initialise(Plugin::getSafeStoreDbDirPath(), Plugin::getSafeStoreDbFileName(), "password"))
        .WillOnce(Return(InitReturnCode::INVALID_ARG))
        .WillOnce(Return(InitReturnCode::UNSUPPORTED_OS))
        .WillOnce(Return(InitReturnCode::UNSUPPORTED_VERSION))
        .WillOnce(Return(InitReturnCode::OUT_OF_MEMORY))
        .WillOnce(Return(InitReturnCode::DB_ERROR))
        .WillOnce(Return(InitReturnCode::OK));
    auto quarantineManager = createQuarantineManager();

    TestableStateMonitor stateMonitor = TestableStateMonitor(quarantineManager);
    EXPECT_EQ(quarantineManager->getState(), QuarantineManagerState::STARTUP);
    quarantineManager->initialise();
    EXPECT_EQ(quarantineManager->getState(), QuarantineManagerState::UNINITIALISED);
    stateMonitor.callInnerRun();
    EXPECT_EQ(quarantineManager->getState(), QuarantineManagerState::UNINITIALISED);
    stateMonitor.callInnerRun();
    EXPECT_EQ(quarantineManager->getState(), QuarantineManagerState::UNINITIALISED);
    stateMonitor.callInnerRun();
    EXPECT_EQ(quarantineManager->getState(), QuarantineManagerState::UNINITIALISED);
    stateMonitor.callInnerRun();
    EXPECT_EQ(quarantineManager->getState(), QuarantineManagerState::CORRUPT);
    stateMonitor.callInnerRun();
    EXPECT_EQ(quarantineManager->getState(), QuarantineManagerState::INITIALISED);
}

TEST_F(StateMonitorTests, testStateMonitorExitsOnDestructDuringWait)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto* filesystemMock = new StrictMock<MockFileSystem>();
    EXPECT_CALL(*filesystemMock, writeFile("/tmp/av/var/persist-safeStoreDbErrorThreshold", "10"));
    EXPECT_CALL(*filesystemMock, exists(_)).WillOnce(Return(false));

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };
    auto quarantineManager = createQuarantineManager();

    {
        TestableStateMonitor stateMonitor = TestableStateMonitor(quarantineManager);
        stateMonitor.start();
        EXPECT_TRUE(waitForLog("Starting Quarantine Manager state monitor"));
    }

    // Log4CPlus is slightly async, so we can't assume the log will always be there immediately
    // after the thread exits
    EXPECT_TRUE(waitForLog("State Monitor stop requested"));
}

TEST_F(StateMonitorTests, testStateMonitorBackoffNeverExceedsMax)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    EXPECT_CALL(*filesystemMock, writeFile("/tmp/av/var/persist-safeStoreDbErrorThreshold", "10"));
    EXPECT_CALL(*filesystemMock, exists(_)).WillOnce(Return(false));

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };
    auto quarantineManager = createQuarantineManager();

    TestableStateMonitor stateMonitor = TestableStateMonitor(quarantineManager);
    stateMonitor.start();

    EXPECT_EQ(stateMonitor.getCurrentBackoffTime(), 60s);
    stateMonitor.testableBackoffIncrease();
    EXPECT_EQ(stateMonitor.getCurrentBackoffTime(), 120s);
    stateMonitor.testableBackoffIncrease();
    EXPECT_EQ(stateMonitor.getCurrentBackoffTime(), 240s);

    for (; stateMonitor.getCurrentBackoffTime() < stateMonitor.getMaxBackoffTime();
         stateMonitor.testableBackoffIncrease())
        ;
    EXPECT_EQ(stateMonitor.getCurrentBackoffTime(), stateMonitor.getMaxBackoffTime());
    stateMonitor.testableBackoffIncrease();
    EXPECT_EQ(stateMonitor.getCurrentBackoffTime(), stateMonitor.getMaxBackoffTime());
}

TEST_F(StateMonitorTests, stateMonitorTriesToRemoveFilesystemLockIfItExistsAndDbIsCorruptThenIfInitsOkDontDeleteDb)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };

    EXPECT_CALL(*filesystemMock, exists("/tmp/av/var/persist-safeStoreDbErrorThreshold")).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, writeFile("/tmp/av/var/persist-safeStoreDbErrorThreshold", "1"));
    EXPECT_CALL(*filesystemMock, readFile("/tmp/av/var/persist-safeStoreDbErrorThreshold")).WillOnce(Return("1"));
    EXPECT_CALL(
        *filesystemMock,
        writeFileAtomically(Plugin::getSafeStoreDormantFlagPath(), "SafeStore database uninitialised", "/tmp/tmp"))
        .Times(1);
    EXPECT_CALL(
        *filesystemMock,
        writeFileAtomically(Plugin::getSafeStoreDormantFlagPath(), "SafeStore database corrupt", "/tmp/tmp"))
        .Times(1);
    EXPECT_CALL(*filesystemMock, isFile(Plugin::getSafeStorePasswordFilePath())).WillRepeatedly(Return(true));
    EXPECT_CALL(*filesystemMock, isFile(Plugin::getSafeStorePasswordFilePath())).WillRepeatedly(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(Plugin::getSafeStorePasswordFilePath())).WillRepeatedly(Return("password"));
    EXPECT_CALL(*filesystemMock, isFile(Plugin::getSafeStoreConfigPath())).WillRepeatedly(Return(false));

    // Mock lock file being present
    EXPECT_CALL(*filesystemMock, exists(Plugin::getSafeStoreDbLockDirPath())).WillRepeatedly(Return(true));
    EXPECT_CALL(*filesystemMock, removeFileOrDirectory(Plugin::getSafeStoreDbLockDirPath()));

    EXPECT_CALL(*filesystemMock, removeFile(Plugin::getSafeStoreDormantFlagPath(), true)).WillOnce(Return());
    EXPECT_CALL(*filesystemMock, removeFileOrDirectory(Plugin::getSafeStoreDbDirPath())).Times(0);

    EXPECT_CALL(
        *m_mockSafeStoreWrapper,
        initialise(Plugin::getSafeStoreDbDirPath(), Plugin::getSafeStoreDbFileName(), "password"))
        .WillOnce(Return(InitReturnCode::FAILED))
        .WillOnce(Return(InitReturnCode::OK));
    auto quarantineManager = createQuarantineManager();

    TestableStateMonitor stateMonitor = TestableStateMonitor(quarantineManager);
    ASSERT_EQ(quarantineManager->getState(), QuarantineManagerState::STARTUP);
    quarantineManager->initialise();
    ASSERT_EQ(quarantineManager->getState(), QuarantineManagerState::CORRUPT);
    stateMonitor.callInnerRun();
    ASSERT_EQ(quarantineManager->getState(), QuarantineManagerState::INITIALISED);
}

TEST_F(
    StateMonitorTests,
    stateMonitorTriesToRemoveFilesystemLockIfItExistsAndDbIsCorruptThenIfStillCantInitTheDbIsDeleted)
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };

    EXPECT_CALL(*filesystemMock, exists("/tmp/av/var/persist-safeStoreDbErrorThreshold")).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, writeFile("/tmp/av/var/persist-safeStoreDbErrorThreshold", "1"));
    EXPECT_CALL(*filesystemMock, readFile("/tmp/av/var/persist-safeStoreDbErrorThreshold")).WillOnce(Return("1"));
    EXPECT_CALL(
        *filesystemMock,
        writeFileAtomically(Plugin::getSafeStoreDormantFlagPath(), "SafeStore database uninitialised", "/tmp/tmp"))
        .Times(3);
    EXPECT_CALL(
        *filesystemMock,
        writeFileAtomically(Plugin::getSafeStoreDormantFlagPath(), "SafeStore database corrupt", "/tmp/tmp"))
        .Times(2);
    EXPECT_CALL(*filesystemMock, isFile(Plugin::getSafeStorePasswordFilePath())).WillRepeatedly(Return(true));
    EXPECT_CALL(*filesystemMock, isFile(Plugin::getSafeStorePasswordFilePath())).WillRepeatedly(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(Plugin::getSafeStorePasswordFilePath())).WillRepeatedly(Return("password"));
    EXPECT_CALL(*filesystemMock, isFile(Plugin::getSafeStoreConfigPath())).WillRepeatedly(Return(false));
    EXPECT_CALL(*filesystemMock, exists(Plugin::getSafeStoreDbDirPath())).WillRepeatedly(Return(true));

    // Mock lock file being present and then being deleted
    EXPECT_CALL(*filesystemMock, exists(Plugin::getSafeStoreDbLockDirPath())).WillRepeatedly(Return(true));
    EXPECT_CALL(*filesystemMock, removeFileOrDirectory(Plugin::getSafeStoreDbLockDirPath()));

    // Mock DB being deleted and the dirs re-created
    EXPECT_CALL(*filesystemMock, removeFileOrDirectory(Plugin::getSafeStoreDbDirPath())).Times(1);
    EXPECT_CALL(*filesystemMock, makedirs(Plugin::getSafeStoreDbDirPath())).Times(1);

    auto* filePermissionsMock = new StrictMock<MockFilePermissions>();
    Tests::ScopedReplaceFilePermissions scopedReplaceFilePermissions{
        std::unique_ptr<Common::FileSystem::IFilePermissions>(filePermissionsMock)
    };

    EXPECT_CALL(*filePermissionsMock, chown(Plugin::getSafeStoreDbDirPath(), "root", "root")).WillOnce(Return());
    EXPECT_CALL(*filePermissionsMock, chmod(Plugin::getSafeStoreDbDirPath(), S_IRUSR | S_IWUSR | S_IXUSR))
        .WillOnce(Return());

    EXPECT_CALL(*filesystemMock, removeFile(Plugin::getSafeStoreDormantFlagPath(), true)).WillOnce(Return());

    EXPECT_CALL(
        *m_mockSafeStoreWrapper,
        initialise(Plugin::getSafeStoreDbDirPath(), Plugin::getSafeStoreDbFileName(), "password"))
        .WillOnce(Return(InitReturnCode::FAILED)) // Go into corrupt state
        .WillOnce(Return(InitReturnCode::FAILED)) // 2nd one for when we try to initialise after waiting for lock dir
        .WillOnce(Return(InitReturnCode::OK));    // Then "OK" for once we have deleted the DB
    auto quarantineManager = createQuarantineManager();

    TestableStateMonitor stateMonitor = TestableStateMonitor(quarantineManager);
    ASSERT_EQ(quarantineManager->getState(), QuarantineManagerState::STARTUP);
    quarantineManager->initialise();
    ASSERT_EQ(quarantineManager->getState(), QuarantineManagerState::CORRUPT);
    stateMonitor.callInnerRun();
    ASSERT_EQ(quarantineManager->getState(), QuarantineManagerState::INITIALISED);
}
