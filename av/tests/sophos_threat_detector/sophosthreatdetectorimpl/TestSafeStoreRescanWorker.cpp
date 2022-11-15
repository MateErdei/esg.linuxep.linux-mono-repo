// Copyright 2022, Sophos Limited.  All rights reserved.

#include "../../common/LogInitializedTests.h"
#include "sophos_threat_detector/sophosthreatdetectorimpl/SafeStoreRescanWorker.h"

#include "Common/Helpers/FileSystemReplaceAndRestore.h"
#include "Common/FileSystem/IFileSystemException.h"
#include "Common/Logging/ConsoleLoggingSetup.h"
#include "Common/Helpers/MockFileSystem.h"

#include <gtest/gtest.h>

using namespace testing;

namespace
{
    class TestSafeStoreRescanWorker : public LogInitializedTests
    {
        void SetUp() override
        {
            auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
            appConfig.setData("PLUGIN_INSTALL", "/tmp/TestSafeStoreRescanWorker");
        }
    };

    class TestableWorker : public SafeStoreRescanWorker
    {
    public:
        explicit TestableWorker(const fs::path& safeStoreRescanSocket, bool& rescanSent) :
            SafeStoreRescanWorker(safeStoreRescanSocket), m_rescanSent(rescanSent)
        {
        }

        void sendRescanRequest() override
        {
            m_rescanSent = true;
        }

        bool& m_rescanSent;
    };
} // namespace

TEST_F(TestSafeStoreRescanWorker, testExitsOnDestructDuringWait) // NOLINT
{
    testing::internal::CaptureStderr();
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };

    // Mocking Persistent value filesystem calls
    EXPECT_CALL(*filesystemMock, exists(_)).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(_)).WillOnce(Return("10"));

    bool haveIBeenCalled = false;
    {
        TestableWorker testWorker("no socket", haveIBeenCalled);
        testWorker.start();
        sleep(1);
    }

    std::string logMessage = internal::GetCapturedStderr();
    ASSERT_THAT(logMessage, ::testing::HasSubstr("SafeStore Rescan Worker interval setting file found -- attempting to parse."));
    ASSERT_THAT(logMessage, ::testing::HasSubstr("Setting rescan interval to: 10 seconds"));
    ASSERT_THAT(logMessage, ::testing::HasSubstr("SafeStore Rescan socket path: \"no socket\""));
    ASSERT_THAT(logMessage, ::testing::HasSubstr("SafeStore Rescan interval: 10"));
    ASSERT_THAT(logMessage, ::testing::HasSubstr("SafeStoreRescanWorker stop requested"));
    ASSERT_THAT(logMessage, ::testing::HasSubstr("Exiting SafeStoreRescanWorker"));
    ASSERT_FALSE(haveIBeenCalled);
}

TEST_F(TestSafeStoreRescanWorker, testTriggerRescan) // NOLINT
{
    testing::internal::CaptureStderr();
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };

    // Mocking Persistent value filesystem calls
    EXPECT_CALL(*filesystemMock, exists(_)).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(_)).WillOnce(Return("10"));

    bool haveIBeenCalled = false;
    TestableWorker testWorker("no socket", haveIBeenCalled);
    testWorker.start();
    testWorker.triggerRescan();
    sleep(1);

    std::string logMessage = internal::GetCapturedStderr();
    ASSERT_THAT(logMessage, ::testing::HasSubstr("SafeStore Rescan Worker interval setting file found -- attempting to parse."));
    ASSERT_THAT(logMessage, ::testing::HasSubstr("Setting rescan interval to: 10 seconds"));
    ASSERT_THAT(logMessage, ::testing::HasSubstr("SafeStore Rescan socket path: \"no socket\""));
    ASSERT_THAT(logMessage, ::testing::HasSubstr("SafeStore Rescan interval: 10"));
    ASSERT_TRUE(testWorker.m_rescanSent);
}

TEST_F(TestSafeStoreRescanWorker, timeOutTriggersRescan) // NOLINT
{
    testing::internal::CaptureStderr();
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };

    // Mocking Persistent value filesystem calls
    EXPECT_CALL(*filesystemMock, exists(_)).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(_)).WillOnce(Return("1"));

    bool haveIBeenCalled = false;
    TestableWorker testWorker("no socket", haveIBeenCalled);
    testWorker.start();
    sleep(2);

    std::string logMessage = internal::GetCapturedStderr();
    ASSERT_THAT(logMessage, ::testing::HasSubstr("SafeStore Rescan Worker interval setting file found -- attempting to parse."));
    ASSERT_THAT(logMessage, ::testing::HasSubstr("Setting rescan interval to: 1 seconds"));
    ASSERT_THAT(logMessage, ::testing::HasSubstr("SafeStore Rescan socket path: \"no socket\""));
    ASSERT_THAT(logMessage, ::testing::HasSubstr("SafeStore Rescan interval: 1"));
    ASSERT_TRUE(testWorker.m_rescanSent);
}

TEST_F(TestSafeStoreRescanWorker, parseIntervalConfigUseDefaultIfConfigDoesntExist) // NOLINT
{
    testing::internal::CaptureStderr();
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };

    // Mocking Persistent value filesystem calls
    EXPECT_CALL(*filesystemMock, exists(_)).WillOnce(Return(false));

    bool haveIBeenCalled = false;
    TestableWorker testWorker("no socket", haveIBeenCalled);
    testWorker.start();
    sleep(1);

    std::string logMessage = internal::GetCapturedStderr();
    ASSERT_THAT(logMessage, Not(::testing::HasSubstr("SafeStore Rescan Worker interval setting file found -- attempting to parse.")));
    ASSERT_THAT(logMessage, ::testing::HasSubstr("SafeStore Rescan socket path: \"no socket\""));
    ASSERT_THAT(logMessage, ::testing::HasSubstr("SafeStore Rescan interval: 14400"));
    ASSERT_FALSE(testWorker.m_rescanSent);
}

TEST_F(TestSafeStoreRescanWorker, parseIntervalConfigHandlesFileReadException) // NOLINT
{
    testing::internal::CaptureStderr();
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };

    // Mocking Persistent value filesystem calls
    EXPECT_CALL(*filesystemMock, exists(_)).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(_)).WillOnce(Throw(Common::FileSystem::IFileSystemException("TEST")));

    bool haveIBeenCalled = false;
    TestableWorker testWorker("no socket", haveIBeenCalled);
    testWorker.start();
    sleep(1);

    std::string logMessage = internal::GetCapturedStderr();
    ASSERT_THAT(logMessage, ::testing::HasSubstr("SafeStore Rescan Worker interval setting file found -- attempting to parse."));
    ASSERT_THAT(logMessage, ::testing::HasSubstr("Could not read rescanInterval file due to: TEST"));
    ASSERT_THAT(logMessage, ::testing::HasSubstr("Setting rescan interval to default 4 hours"));
    ASSERT_THAT(logMessage, ::testing::HasSubstr("SafeStore Rescan socket path: \"no socket\""));
    ASSERT_THAT(logMessage, ::testing::HasSubstr("SafeStore Rescan interval: 14400"));
    ASSERT_FALSE(testWorker.m_rescanSent);
}

TEST_F(TestSafeStoreRescanWorker, parseIntervalConfigHandlesNonIntegerContent) // NOLINT
{
    testing::internal::CaptureStderr();
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };

    // Mocking Persistent value filesystem calls
    EXPECT_CALL(*filesystemMock, exists(_)).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(_)).WillOnce(Return("One second please"));

    bool haveIBeenCalled = false;
    TestableWorker testWorker("no socket", haveIBeenCalled);
    testWorker.start();
    sleep(1);

    std::string logMessage = internal::GetCapturedStderr();
    ASSERT_THAT(logMessage, ::testing::HasSubstr("SafeStore Rescan Worker interval setting file found -- attempting to parse."));
    ASSERT_THAT(logMessage, ::testing::HasSubstr("Error parsing integer for rescan interval setting: One second please"));
    ASSERT_THAT(logMessage, ::testing::HasSubstr("Setting rescan interval to default 4 hours"));
    ASSERT_THAT(logMessage, ::testing::HasSubstr("SafeStore Rescan socket path: \"no socket\""));
    ASSERT_THAT(logMessage, ::testing::HasSubstr("SafeStore Rescan interval: 14400"));
    ASSERT_FALSE(testWorker.m_rescanSent);
}

TEST_F(TestSafeStoreRescanWorker, parseIntervalConfigHandlesLessThanOneInteger) // NOLINT
{
    testing::internal::CaptureStderr();
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };

    // Mocking Persistent value filesystem calls
    EXPECT_CALL(*filesystemMock, exists(_)).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(_)).WillOnce(Return("-1"));

    bool haveIBeenCalled = false;
    TestableWorker testWorker("no socket", haveIBeenCalled);
    testWorker.start();
    sleep(1);

    std::string logMessage = internal::GetCapturedStderr();
    ASSERT_THAT(logMessage, ::testing::HasSubstr("SafeStore Rescan Worker interval setting file found -- attempting to parse."));
    ASSERT_THAT(logMessage, ::testing::HasSubstr("Rescan interval cannot be less than 1. Value found: -1"));
    ASSERT_THAT(logMessage, ::testing::HasSubstr("Setting rescan interval to default 4 hours"));
    ASSERT_THAT(logMessage, ::testing::HasSubstr("SafeStore Rescan socket path: \"no socket\""));
    ASSERT_THAT(logMessage, ::testing::HasSubstr("SafeStore Rescan interval: 14400"));
    ASSERT_FALSE(testWorker.m_rescanSent);
}
