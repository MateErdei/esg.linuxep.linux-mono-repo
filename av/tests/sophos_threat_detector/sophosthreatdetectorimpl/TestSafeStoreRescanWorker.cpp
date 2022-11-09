// Copyright 2022, Sophos Limited.  All rights reserved.

#include "pluginapi/tests/include/Common/Helpers/LogInitializedTests.h"
#include "sophos_threat_detector/sophosthreatdetectorimpl/SafeStoreRescanWorker.h"

#include "Common/Helpers/FileSystemReplaceAndRestore.h"
#include "Common/Logging/ConsoleLoggingSetup.h"

#include <Common/Helpers/MockFileSystem.h>
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
    EXPECT_CALL(*filesystemMock, writeFile(_, "10"));

    bool haveIBeenCalled = false;
    {
        TestableWorker testWorker("no socket", haveIBeenCalled);
        testWorker.start();
        sleep(1);
    }
    std::string logMessage = internal::GetCapturedStderr();
    ASSERT_THAT(logMessage, ::testing::HasSubstr("SafeStoreRescanWorker stop requested"));
    ASSERT_THAT(logMessage, ::testing::HasSubstr("Exiting SafeStoreRescanWorker"));
}

TEST_F(TestSafeStoreRescanWorker, testTriggerRescan) // NOLINT
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };

    // Mocking Persistent value filesystem calls
    EXPECT_CALL(*filesystemMock, exists(_)).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(_)).WillOnce(Return("10"));
    EXPECT_CALL(*filesystemMock, writeFile(_, "10"));

    bool haveIBeenCalled = false;
    TestableWorker testWorker("no socket", haveIBeenCalled);
    testWorker.start();
    testWorker.triggerRescan();
    sleep(1);

    ASSERT_TRUE(testWorker.m_rescanSent);
}

TEST_F(TestSafeStoreRescanWorker, timeOutTriggersRescan) // NOLINT
{
    auto* filesystemMock = new StrictMock<MockFileSystem>();
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::unique_ptr<Common::FileSystem::IFileSystem>(
        filesystemMock) };

    // Mocking Persistent value filesystem calls
    EXPECT_CALL(*filesystemMock, exists(_)).WillOnce(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(_)).WillOnce(Return("1"));
    EXPECT_CALL(*filesystemMock, writeFile(_, "1"));

    bool haveIBeenCalled = false;
    TestableWorker testWorker("no socket", haveIBeenCalled);
    testWorker.start();
    sleep(2);

    ASSERT_TRUE(testWorker.m_rescanSent);
}
