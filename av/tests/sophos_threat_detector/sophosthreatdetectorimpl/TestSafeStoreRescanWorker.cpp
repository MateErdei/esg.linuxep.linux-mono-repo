// Copyright 2022-2023 Sophos Limited. All rights reserved.

#include "sophos_threat_detector/sophosthreatdetectorimpl/SafeStoreRescanWorker.h"

#include "Common/FileSystem/IFileSystemException.h"
#include "Common/Logging/ConsoleLoggingSetup.h"

// test includes
#include "tests/common/MemoryAppender.h"
#include "Common/Helpers/FileSystemReplaceAndRestore.h"
#include "Common/Helpers/MockFileSystem.h"

#include <gtest/gtest.h>

using namespace testing;

namespace
{
    class TestSafeStoreRescanWorker : public MemoryAppenderUsingTests
    {
    public:
        TestSafeStoreRescanWorker() :  MemoryAppenderUsingTests("SophosThreatDetectorImpl")
        {}

        void SetUp() override
        {
            auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
            appConfig.setData("PLUGIN_INSTALL", "/tmp/TestSafeStoreRescanWorker");

            m_mockFileSystem = std::make_unique<StrictMock<MockFileSystem>>();
        }

        std::unique_ptr<StrictMock<MockFileSystem>> m_mockFileSystem;
    };

    class TestableWorker : public SafeStoreRescanWorker
    {
    public:
        explicit TestableWorker(const fs::path& safeStoreRescanSocket, bool& rescanSent) :
            SafeStoreRescanWorker(safeStoreRescanSocket), m_rescanSent(rescanSent)
        {
        }

        void sendRescanRequest(std::unique_lock<std::mutex>&) override
        {
            m_rescanSent = true;
        }

        bool& m_rescanSent;
    };
} // namespace

TEST_F(TestSafeStoreRescanWorker, testExitsOnDestructDuringWait) 
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    EXPECT_CALL(*m_mockFileSystem, exists(_)).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, readFile(_)).WillOnce(Return("10"));

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::move(m_mockFileSystem) };

    bool haveIBeenCalled = false;
    {
        TestableWorker testWorker("no socket", haveIBeenCalled);
        testWorker.start();
        sleep(1);
    }

    EXPECT_TRUE(appenderContains("SafeStore Rescan Worker interval setting file found -- attempting to parse."));
    EXPECT_TRUE(appenderContains("Setting rescan interval to: 10 seconds"));
    EXPECT_TRUE(appenderContains("SafeStore Rescan socket path: \"no socket\""));
    EXPECT_TRUE(appenderContains("SafeStore Rescan interval: 10"));
    EXPECT_TRUE(appenderContains("SafeStoreRescanWorker stop requested"));
    EXPECT_TRUE(appenderContains("Exiting SafeStoreRescanWorker"));

    EXPECT_FALSE(haveIBeenCalled);
}

TEST_F(TestSafeStoreRescanWorker, testTriggerRescan) 
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    EXPECT_CALL(*m_mockFileSystem, exists(_)).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, readFile(_)).WillOnce(Return("10"));

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::move(m_mockFileSystem) };

    bool haveIBeenCalled = false;
    TestableWorker testWorker("no socket", haveIBeenCalled);
    testWorker.start();
    testWorker.triggerRescan();
    sleep(1);

    EXPECT_TRUE(appenderContains("SafeStore Rescan Worker interval setting file found -- attempting to parse."));
    EXPECT_TRUE(appenderContains("Setting rescan interval to: 10 seconds"));
    EXPECT_TRUE(appenderContains("SafeStore Rescan socket path: \"no socket\""));
    EXPECT_TRUE(appenderContains("SafeStore Rescan interval: 10"));

    EXPECT_TRUE(haveIBeenCalled);
}

TEST_F(TestSafeStoreRescanWorker, timeOutTriggersRescan) 
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    EXPECT_CALL(*m_mockFileSystem, exists(_)).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, readFile(_)).WillOnce(Return("1"));

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::move(m_mockFileSystem) };

    bool haveIBeenCalled = false;
    TestableWorker testWorker("no socket", haveIBeenCalled);
    testWorker.start();
    sleep(3);

    EXPECT_TRUE(appenderContains("SafeStore Rescan Worker interval setting file found -- attempting to parse."));
    EXPECT_TRUE(appenderContains("Setting rescan interval to: 1 seconds"));
    EXPECT_TRUE(appenderContains("SafeStore Rescan socket path: \"no socket\""));
    EXPECT_TRUE(appenderContains("SafeStore Rescan interval: 1"));

    EXPECT_TRUE(haveIBeenCalled);
}

TEST_F(TestSafeStoreRescanWorker, parseIntervalConfigUseDefaultIfConfigDoesntExist) 
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    EXPECT_CALL(*m_mockFileSystem, exists(_)).WillOnce(Return(false));

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::move(m_mockFileSystem) };

    bool haveIBeenCalled = false;
    TestableWorker testWorker("no socket", haveIBeenCalled);
    testWorker.start();
    sleep(1);

    EXPECT_FALSE(appenderContains("SafeStore Rescan Worker interval setting file found -- attempting to parse."));
    EXPECT_TRUE(appenderContains("SafeStore Rescan socket path: \"no socket\""));
    EXPECT_TRUE(appenderContains("SafeStore Rescan interval: 14400"));

    EXPECT_FALSE(haveIBeenCalled);
}

TEST_F(TestSafeStoreRescanWorker, parseIntervalConfigHandlesFileReadException) 
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    EXPECT_CALL(*m_mockFileSystem, exists(_)).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, readFile(_)).WillOnce(Throw(Common::FileSystem::IFileSystemException("TEST")));

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::move(m_mockFileSystem) };

    bool haveIBeenCalled = false;
    TestableWorker testWorker("no socket", haveIBeenCalled);
    testWorker.start();
    sleep(1);

    EXPECT_TRUE(appenderContains("SafeStore Rescan Worker interval setting file found -- attempting to parse."));
    EXPECT_TRUE(appenderContains("Could not read rescanInterval file due to: TEST"));
    EXPECT_TRUE(appenderContains("Setting rescan interval to default 4 hours"));
    EXPECT_TRUE(appenderContains("SafeStore Rescan socket path: \"no socket\""));
    EXPECT_TRUE(appenderContains("SafeStore Rescan interval: 14400"));

    EXPECT_FALSE(haveIBeenCalled);
}

TEST_F(TestSafeStoreRescanWorker, parseIntervalConfigHandlesNonIntegerContent) 
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    EXPECT_CALL(*m_mockFileSystem, exists(_)).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, readFile(_)).WillOnce(Return("One second please"));

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::move(m_mockFileSystem) };

    bool haveIBeenCalled = false;
    TestableWorker testWorker("no socket", haveIBeenCalled);
    testWorker.start();
    sleep(1);

    EXPECT_TRUE(appenderContains("SafeStore Rescan Worker interval setting file found -- attempting to parse."));
    EXPECT_TRUE(appenderContains("Error parsing integer for rescan interval setting: One second please"));
    EXPECT_TRUE(appenderContains("Setting rescan interval to default 4 hours"));
    EXPECT_TRUE(appenderContains("SafeStore Rescan socket path: \"no socket\""));
    EXPECT_TRUE(appenderContains("SafeStore Rescan interval: 14400"));

    EXPECT_FALSE(haveIBeenCalled);
}

TEST_F(TestSafeStoreRescanWorker, parseIntervalConfigHandlesLessThanOneInteger) 
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    EXPECT_CALL(*m_mockFileSystem, exists(_)).WillOnce(Return(true));
    EXPECT_CALL(*m_mockFileSystem, readFile(_)).WillOnce(Return("-1"));

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem { std::move(m_mockFileSystem) };

    bool haveIBeenCalled = false;
    TestableWorker testWorker("no socket", haveIBeenCalled);
    testWorker.start();
    sleep(1);

    EXPECT_TRUE(appenderContains("SafeStore Rescan Worker interval setting file found -- attempting to parse."));
    EXPECT_TRUE(appenderContains("Rescan interval cannot be less than 1. Value found: -1"));
    EXPECT_TRUE(appenderContains("Setting rescan interval to default 4 hours"));
    EXPECT_TRUE(appenderContains("SafeStore Rescan socket path: \"no socket\""));
    EXPECT_TRUE(appenderContains("SafeStore Rescan interval: 14400"));

    EXPECT_FALSE(haveIBeenCalled);
}
