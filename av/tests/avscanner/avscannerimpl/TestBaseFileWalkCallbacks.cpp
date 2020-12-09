/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "avscanner/avscannerimpl/BaseFileWalkCallbacks.h"
#include "datatypes/sophos_filesystem.h"
#include "tests/common/MemoryAppender.h"

#include <common/AbortScanException.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <utility>

namespace fs = sophos_filesystem;

using namespace ::testing;
using namespace avscanner::avscannerimpl;

namespace
{
    class TestBaseFileWalkCallbacks : public MemoryAppenderUsingTests
    {
    public:
        TestBaseFileWalkCallbacks() : MemoryAppenderUsingTests("NamedScanRunner") {}
    };

    class MockScanClient : public IScanClient
    {
    public:
        MOCK_METHOD1(scanError, void(const std::ostringstream& error));
        MOCK_METHOD2(scan, scan_messages::ScanResponse(const fs::path& fileToScanPath, bool isSymlink));
    };

    class CallbackImpl : public BaseFileWalkCallbacks
    {
    public:
        CallbackImpl(
            std::shared_ptr<IScanClient> scanner,
            std::vector<fs::path> mountExclusions,
            std::vector<Exclusion> currentExclusions,
            std::vector<Exclusion> userDefinedExclusions) :
            avscanner::avscannerimpl::BaseFileWalkCallbacks(std::move(scanner))
        {
            m_mountExclusions = std::move(mountExclusions);
            m_currentExclusions = std::move(currentExclusions);
            m_userDefinedExclusions = std::move(userDefinedExclusions);
        }

        MOCK_METHOD1(logScanningLine, void(std::string escapedPath));
    };
} // namespace

TEST_F(TestBaseFileWalkCallbacks, noActions) // NOLINT
{
    auto scanClient = std::make_shared<StrictMock<MockScanClient>>();
    std::vector<fs::path> mountExclusions {};
    std::vector<Exclusion> currentExclusions {};
    std::vector<Exclusion> userDefinedExclusions {};

    CallbackImpl callback(scanClient, mountExclusions, currentExclusions, userDefinedExclusions);
    EXPECT_EQ(callback.returnCode(), E_CLEAN);
}

TEST_F(TestBaseFileWalkCallbacks, scansFile) // NOLINT
{
    auto scanClient = std::make_shared<StrictMock<MockScanClient>>();
    std::vector<fs::path> mountExclusions {};
    std::vector<Exclusion> currentExclusions {};
    std::vector<Exclusion> userDefinedExclusions {};

    CallbackImpl callback(scanClient, mountExclusions, currentExclusions, userDefinedExclusions);

    InSequence seq;
    EXPECT_CALL(callback, logScanningLine(_)).Times(1);
    EXPECT_CALL(*scanClient, scan(_, _)).Times(1);

    callback.processFile(fs::path("/testfile"), false);

    EXPECT_EQ(callback.returnCode(), E_CLEAN);
}

TEST_F(TestBaseFileWalkCallbacks, scanThrows) // NOLINT
{
    auto scanClient = std::make_shared<StrictMock<MockScanClient>>();
    std::vector<fs::path> mountExclusions {};
    std::vector<Exclusion> currentExclusions {};
    std::vector<Exclusion> userDefinedExclusions {};

    CallbackImpl callback(scanClient, mountExclusions, currentExclusions, userDefinedExclusions);

    InSequence seq;
    EXPECT_CALL(callback, logScanningLine(_)).Times(1);
    AbortScanException abortScanException("exception");
    EXPECT_CALL(*scanClient, scan(_, _)).WillOnce(Throw(abortScanException));
    EXPECT_CALL(*scanClient, scanError(_));

    try
    {
        callback.processFile(fs::path("/testfile"), false);
        FAIL() << "processFile didn't throw";
    }
    catch (AbortScanException& ex)
    {
        EXPECT_STREQ(abortScanException.what(), ex.what());
    }

    EXPECT_EQ(callback.returnCode(), E_GENERIC_FAILURE);
}

TEST_F(TestBaseFileWalkCallbacks, excludeFile) // NOLINT
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto scanClient = std::make_shared<StrictMock<MockScanClient>>();
    std::vector<fs::path> mountExclusions {};
    std::vector<Exclusion> currentExclusions {};
    Exclusion exclusion(fs::path("testfile"));
    std::vector<Exclusion> userDefinedExclusions { exclusion };

    CallbackImpl callback(scanClient, mountExclusions, currentExclusions, userDefinedExclusions);

    EXPECT_CALL(callback, logScanningLine(_)).Times(0);
    EXPECT_CALL(*scanClient, scan(_, _)).Times(0);

    callback.processFile(fs::path("/testfile"), false);

    EXPECT_TRUE(appenderContains("Excluding file: /testfile"));
    EXPECT_EQ(callback.returnCode(), E_CLEAN);
}

TEST_F(TestBaseFileWalkCallbacks, includeDirectory) // NOLINT
{
    auto scanClient = std::make_shared<StrictMock<MockScanClient>>();
    std::vector<fs::path> mountExclusions {};
    std::vector<Exclusion> currentExclusions {};
    std::vector<Exclusion> userDefinedExclusions {};

    CallbackImpl callback(scanClient, mountExclusions, currentExclusions, userDefinedExclusions);

    EXPECT_TRUE(callback.includeDirectory(fs::path("/testfile")));

    EXPECT_EQ(callback.returnCode(), E_CLEAN);
}

TEST_F(TestBaseFileWalkCallbacks, excludeDirectoryByMount) // NOLINT
{
    auto scanClient = std::make_shared<StrictMock<MockScanClient>>();
    std::vector<fs::path> mountExclusions {};
    Exclusion exclusion(fs::path("/testfile"));
    std::vector<Exclusion> currentExclusions { exclusion };
    std::vector<Exclusion> userDefinedExclusions {};

    CallbackImpl callback(scanClient, mountExclusions, currentExclusions, userDefinedExclusions);

    EXPECT_FALSE(callback.includeDirectory(fs::path("/testfile")));

    EXPECT_EQ(callback.returnCode(), E_CLEAN);
}

TEST_F(TestBaseFileWalkCallbacks, excludeDirectoryByUser) // NOLINT
{
    auto scanClient = std::make_shared<StrictMock<MockScanClient>>();
    std::vector<fs::path> mountExclusions {};
    std::vector<Exclusion> currentExclusions {};
    Exclusion exclusion(fs::path("testfile/"));
    std::vector<Exclusion> userDefinedExclusions { exclusion };

    CallbackImpl callback(scanClient, mountExclusions, currentExclusions, userDefinedExclusions);

    EXPECT_FALSE(callback.includeDirectory(fs::path("/testfile")));

    EXPECT_EQ(callback.returnCode(), E_CLEAN);
}

TEST_F(TestBaseFileWalkCallbacks, includeStartingDir) // NOLINT
{
    auto scanClient = std::make_shared<StrictMock<MockScanClient>>();
    std::vector<fs::path> mountExclusions {};
    std::vector<Exclusion> currentExclusions {};
    std::vector<Exclusion> userDefinedExclusions {};

    CallbackImpl callback(scanClient, mountExclusions, currentExclusions, userDefinedExclusions);

    EXPECT_FALSE(callback.userDefinedExclusionCheck(fs::path("/testfile"), false));

    EXPECT_EQ(callback.returnCode(), E_CLEAN);
}

TEST_F(TestBaseFileWalkCallbacks, excludeStartingDir) // NOLINT
{
    auto scanClient = std::make_shared<StrictMock<MockScanClient>>();
    std::vector<fs::path> mountExclusions {};
    std::vector<Exclusion> currentExclusions {};
    Exclusion exclusion(fs::path("testfile/"));
    std::vector<Exclusion> userDefinedExclusions { exclusion };

    CallbackImpl callback(scanClient, mountExclusions, currentExclusions, userDefinedExclusions);

    EXPECT_TRUE(callback.userDefinedExclusionCheck(fs::path("/testfile"), false));

    EXPECT_EQ(callback.returnCode(), E_CLEAN);
}
