/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "avscanner/avscannerimpl/BaseFileWalkCallbacks.h"
#include "datatypes/sophos_filesystem.h"
#include "tests/common/MemoryAppender.h"

#include <common/AbortScanException.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <fstream>
#include <utility>

namespace fs = sophos_filesystem;

using namespace ::testing;
using namespace avscanner::avscannerimpl;
using namespace common;

namespace
{
    class TestBaseFileWalkCallbacks : public MemoryAppenderUsingTests
    {
    public:
        TestBaseFileWalkCallbacks() : MemoryAppenderUsingTests("NamedScanRunner") {}

    protected:
        void SetUp() override
        {
            const ::testing::TestInfo* const test_info = ::testing::UnitTest::GetInstance()->current_test_info();
            m_testDir = fs::temp_directory_path();
            m_testDir /= test_info->test_case_name();
            m_testDir /= test_info->name();
            fs::remove_all(m_testDir);
            fs::create_directories(m_testDir);
            fs::current_path(m_testDir);
        }

        void TearDown() override
        {
            fs::current_path(fs::temp_directory_path());
            fs::remove_all(m_testDir);
        }

        fs::path m_testDir;
    };

    class MockScanClient : public IScanClient
    {
    public:
        MOCK_METHOD(void, scanError, (const std::ostringstream& error, std::error_code errorCode), (override));
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
    EXPECT_EQ(callback.returnCode(), E_CLEAN_SUCCESS);
}

TEST_F(TestBaseFileWalkCallbacks, scansFile) // NOLINT
{
    fs::path testfile = "/testfile";
    auto scanClient = std::make_shared<StrictMock<MockScanClient>>();
    std::vector<fs::path> mountExclusions {};
    std::vector<Exclusion> currentExclusions {};
    std::vector<Exclusion> userDefinedExclusions {};

    CallbackImpl callback(scanClient, mountExclusions, currentExclusions, userDefinedExclusions);

    InSequence seq;
    EXPECT_CALL(callback, logScanningLine("/testfile")).Times(1);
    EXPECT_CALL(*scanClient, scan(testfile, false)).Times(1);

    callback.processFile(testfile, false);

    EXPECT_EQ(callback.returnCode(), E_CLEAN_SUCCESS);
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
    EXPECT_CALL(*scanClient, scanError(_, _));

    try
    {
        callback.processFile(fs::path("/testfile"), false);
        FAIL() << "processFile didn't throw";
    }
    catch (common::AbortScanException& ex)
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
    EXPECT_EQ(callback.returnCode(), E_CLEAN_SUCCESS);
}

TEST_F(TestBaseFileWalkCallbacks, includeDirectory) // NOLINT
{
    auto scanClient = std::make_shared<StrictMock<MockScanClient>>();
    std::vector<fs::path> mountExclusions {};
    std::vector<Exclusion> currentExclusions {};
    std::vector<Exclusion> userDefinedExclusions {};

    CallbackImpl callback(scanClient, mountExclusions, currentExclusions, userDefinedExclusions);

    EXPECT_TRUE(callback.includeDirectory(fs::path("/testdir")));

    EXPECT_EQ(callback.returnCode(), E_CLEAN_SUCCESS);
}

TEST_F(TestBaseFileWalkCallbacks, excludeDirectoryByMount) // NOLINT
{
    auto scanClient = std::make_shared<StrictMock<MockScanClient>>();
    std::vector<fs::path> mountExclusions {};
    Exclusion exclusion(fs::path("/testdir/"));
    std::vector<Exclusion> currentExclusions { exclusion };
    std::vector<Exclusion> userDefinedExclusions {};

    CallbackImpl callback(scanClient, mountExclusions, currentExclusions, userDefinedExclusions);

    EXPECT_FALSE(callback.includeDirectory(fs::path("/testdir")));

    EXPECT_EQ(callback.returnCode(), E_CLEAN_SUCCESS);
}

TEST_F(TestBaseFileWalkCallbacks, excludeSubdirectoryByMount) // NOLINT
{
    auto scanClient = std::make_shared<StrictMock<MockScanClient>>();
    std::vector<fs::path> mountExclusions {};
    Exclusion exclusion(fs::path("/testdir/"));
    std::vector<Exclusion> currentExclusions { exclusion };
    std::vector<Exclusion> userDefinedExclusions {};

    CallbackImpl callback(scanClient, mountExclusions, currentExclusions, userDefinedExclusions);

    EXPECT_FALSE(callback.includeDirectory(fs::path("/testdir/subdir")));

    EXPECT_EQ(callback.returnCode(), E_CLEAN_SUCCESS);
}

TEST_F(TestBaseFileWalkCallbacks, excludeDirectoryByMountIncludesOtherDirs) // NOLINT
{
    auto scanClient = std::make_shared<StrictMock<MockScanClient>>();
    std::vector<fs::path> mountExclusions {};
    Exclusion exclusion(fs::path("/testdir/"));
    std::vector<Exclusion> currentExclusions { exclusion };
    std::vector<Exclusion> userDefinedExclusions {};

    CallbackImpl callback(scanClient, mountExclusions, currentExclusions, userDefinedExclusions);

    EXPECT_TRUE(callback.includeDirectory(fs::path("/")));
    EXPECT_TRUE(callback.includeDirectory(fs::path("/test")));
    EXPECT_TRUE(callback.includeDirectory(fs::path("/testdir2")));
    EXPECT_TRUE(callback.includeDirectory(fs::path("/testdir2/dir")));
    EXPECT_TRUE(callback.includeDirectory(fs::path("/1testdir")));
    EXPECT_TRUE(callback.includeDirectory(fs::path("/dir/testdir")));
    EXPECT_TRUE(callback.includeDirectory(fs::path("/TestDir")));

    EXPECT_EQ(callback.returnCode(), E_CLEAN_SUCCESS);
}

TEST_F(TestBaseFileWalkCallbacks, excludeDirectoryByUser) // NOLINT
{
    auto scanClient = std::make_shared<StrictMock<MockScanClient>>();
    std::vector<fs::path> mountExclusions {};
    std::vector<Exclusion> currentExclusions {};
    Exclusion exclusion(fs::path("testdir/"));
    std::vector<Exclusion> userDefinedExclusions { exclusion };

    CallbackImpl callback(scanClient, mountExclusions, currentExclusions, userDefinedExclusions);

    EXPECT_FALSE(callback.includeDirectory(fs::path("/testdir")));

    EXPECT_EQ(callback.returnCode(), E_CLEAN_SUCCESS);
}

TEST_F(TestBaseFileWalkCallbacks, includeStartingDir) // NOLINT
{
    auto scanClient = std::make_shared<StrictMock<MockScanClient>>();
    std::vector<fs::path> mountExclusions {};
    std::vector<Exclusion> currentExclusions {};
    std::vector<Exclusion> userDefinedExclusions {};

    CallbackImpl callback(scanClient, mountExclusions, currentExclusions, userDefinedExclusions);

    EXPECT_FALSE(callback.userDefinedExclusionCheck(fs::path("/testdir"), false));

    EXPECT_EQ(callback.returnCode(), E_CLEAN_SUCCESS);
}

TEST_F(TestBaseFileWalkCallbacks, excludeStartingDir) // NOLINT
{
    auto scanClient = std::make_shared<StrictMock<MockScanClient>>();
    std::vector<fs::path> mountExclusions {};
    std::vector<Exclusion> currentExclusions {};
    Exclusion exclusion(fs::path("testdir/"));
    std::vector<Exclusion> userDefinedExclusions { exclusion };

    CallbackImpl callback(scanClient, mountExclusions, currentExclusions, userDefinedExclusions);

    EXPECT_TRUE(callback.userDefinedExclusionCheck(fs::path("/testdir"), false));

    EXPECT_EQ(callback.returnCode(), E_CLEAN_SUCCESS);
}

TEST_F(TestBaseFileWalkCallbacks, scansSymlinkToFile) // NOLINT
{
    fs::path testfile("testfile");
    std::ofstream(testfile).close();
    fs::path symlink("symlink");
    fs::create_symlink(testfile, symlink);

    auto scanClient = std::make_shared<StrictMock<MockScanClient>>();
    std::vector<fs::path> mountExclusions {};
    std::vector<Exclusion> currentExclusions {};
    std::vector<Exclusion> userDefinedExclusions {};

    CallbackImpl callback(scanClient, mountExclusions, currentExclusions, userDefinedExclusions);

    InSequence seq;
    EXPECT_CALL(callback, logScanningLine("symlink")).Times(1);
    EXPECT_CALL(*scanClient, scan(symlink, true)).Times(1);

    callback.processFile(symlink, true);

    EXPECT_EQ(callback.returnCode(), E_CLEAN_SUCCESS);
}

TEST_F(TestBaseFileWalkCallbacks, excludeFileSymlinkTargetByUser) // NOLINT
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    fs::path testfile("testfile");
    std::ofstream(testfile).close();
    fs::path symlink("symlink");
    fs::create_symlink(testfile, symlink);

    auto scanClient = std::make_shared<StrictMock<MockScanClient>>();
    std::vector<fs::path> mountExclusions {};
    std::vector<Exclusion> currentExclusions {};
    Exclusion exclusion(testfile);
    std::vector<Exclusion> userDefinedExclusions { exclusion };

    CallbackImpl callback(scanClient, mountExclusions, currentExclusions, userDefinedExclusions);

    EXPECT_CALL(callback, logScanningLine(_)).Times(0);
    EXPECT_CALL(*scanClient, scan(_, _)).Times(0);

    callback.processFile(fs::path(".") / symlink, true);

    EXPECT_TRUE(appenderContains("Skipping the scanning of symlink target"));
    EXPECT_TRUE(appenderContains("which is excluded by user defined exclusion: testfile"));
    EXPECT_EQ(callback.returnCode(), E_CLEAN_SUCCESS);
}

TEST_F(TestBaseFileWalkCallbacks, excludeFileSymlinkTargetByMount) // NOLINT
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    fs::path testfile("testfile");
    std::ofstream(testfile).close();
    fs::path symlink("symlink");
    fs::create_symlink(testfile, symlink);

    auto scanClient = std::make_shared<StrictMock<MockScanClient>>();
    std::vector<fs::path> mountExclusions { fs::current_path().string() + "/" };
    std::vector<Exclusion> currentExclusions {};
    std::vector<Exclusion> userDefinedExclusions {};

    CallbackImpl callback(scanClient, mountExclusions, currentExclusions, userDefinedExclusions);

    EXPECT_CALL(callback, logScanningLine(_)).Times(0);
    EXPECT_CALL(*scanClient, scan(_, _)).Times(0);

    callback.processFile(fs::path(".") / symlink, true);

    EXPECT_TRUE(appenderContains("Skipping the scanning of symlink target"));
    EXPECT_TRUE(appenderContains("which is on excluded mount point: "));
    EXPECT_EQ(callback.returnCode(), E_CLEAN_SUCCESS);
}

TEST_F(TestBaseFileWalkCallbacks, excludeFileSymlinkTargetByMountNotPrefix) // NOLINT
{
    fs::path testdir = "testdir";
    fs::path testdir2 = "testdir2";
    fs::create_directory(testdir2);
    fs::path testfile = testdir2 / "testfile";
    std::ofstream(testfile).close();
    fs::path symlink = "symlink";
    fs::create_symlink(testfile, symlink);

    auto scanClient = std::make_shared<StrictMock<MockScanClient>>();
    std::string exclusionPath = fs::absolute(testdir).string() + "/";
    std::vector<fs::path> mountExclusions { exclusionPath };
    Exclusion exclusion(exclusionPath);
    std::vector<Exclusion> currentExclusions { exclusion };
    std::vector<Exclusion> userDefinedExclusions {};

    CallbackImpl callback(scanClient, mountExclusions, currentExclusions, userDefinedExclusions);

    InSequence seq;
    EXPECT_CALL(callback, logScanningLine("symlink")).Times(1);
    EXPECT_CALL(*scanClient, scan(symlink, true)).Times(1);

    callback.processFile(symlink, true);

    EXPECT_EQ(callback.returnCode(), E_CLEAN_SUCCESS);
}

TEST_F(TestBaseFileWalkCallbacks, excludeFileSymlinkByUser) // NOLINT
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    fs::path testfile("testfile");
    std::ofstream(testfile).close();
    fs::path symlink("symlink");
    fs::create_symlink(testfile, symlink);

    auto scanClient = std::make_shared<StrictMock<MockScanClient>>();
    std::vector<fs::path> mountExclusions {};
    std::vector<Exclusion> currentExclusions {};
    Exclusion exclusion(symlink);
    std::vector<Exclusion> userDefinedExclusions { exclusion };

    CallbackImpl callback(scanClient, mountExclusions, currentExclusions, userDefinedExclusions);

    EXPECT_CALL(callback, logScanningLine(_)).Times(0);
    EXPECT_CALL(*scanClient, scan(_, _)).Times(0);

    callback.processFile(fs::path(".") / symlink, true);

    EXPECT_TRUE(appenderContains("Excluding symlinked file: ./symlink"));
    EXPECT_EQ(callback.returnCode(), E_CLEAN_SUCCESS);
}

TEST_F(TestBaseFileWalkCallbacks, includeFileSymlinkWithUserExclusions) // NOLINT
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    fs::path testfile("testfile");
    std::ofstream(testfile).close();
    fs::path symlink("symlink");
    fs::create_symlink(testfile, symlink);

    std::vector<fs::path> exclusionPaths =
        {
            "/testfile",
            "/symlink",
            "/testfile/",
            "/symlink/",
            "test",
            "sym",
            "file",
            "link",
            "test/",
            "sym/",
            "file/",
            "link/",
            "testfile/",
            "symlink/"
        };

    auto scanClient = std::make_shared<StrictMock<MockScanClient>>();
    std::vector<fs::path> mountExclusions {};
    std::vector<Exclusion> currentExclusions {};
    std::vector<Exclusion> userDefinedExclusions { };
    for (const auto& exclusionPath : exclusionPaths)
    {
        Exclusion exclusion { exclusionPath };
        userDefinedExclusions.push_back(exclusion);
    }

    CallbackImpl callback(scanClient, mountExclusions, currentExclusions, userDefinedExclusions);

    InSequence seq;
    EXPECT_CALL(callback, logScanningLine("symlink")).Times(1);
    EXPECT_CALL(*scanClient, scan(symlink, true)).Times(1);

    callback.processFile(symlink, true);

    EXPECT_EQ(callback.returnCode(), E_CLEAN_SUCCESS);
}

// Simulate the situation where a symlink disappears during scanning.
// implies the need for "TestFileWalker, handlesExceptionFromProcessFileInWalk"
TEST_F(TestBaseFileWalkCallbacks, scansMissingFileAsSymlink) // NOLINT
{
    fs::path testfile = "/testfile";
    auto scanClient = std::make_shared<StrictMock<MockScanClient>>();
    std::vector<fs::path> mountExclusions {};
    std::vector<Exclusion> currentExclusions {};
    std::vector<Exclusion> userDefinedExclusions {};

    CallbackImpl callback(scanClient, mountExclusions, currentExclusions, userDefinedExclusions);

    EXPECT_THROW(callback.processFile(testfile, true), std::exception);

    EXPECT_EQ(callback.returnCode(), E_CLEAN_SUCCESS);
}

TEST_F(TestBaseFileWalkCallbacks, scanBrokenSymlink) // NOLINT
{
    fs::path testfile("testfile");
    fs::path symlink("symlink");
    fs::create_symlink(testfile, symlink);

    auto scanClient = std::make_shared<StrictMock<MockScanClient>>();
    std::vector<fs::path> mountExclusions {};
    std::vector<Exclusion> currentExclusions {};
    std::vector<Exclusion> userDefinedExclusions {};

    CallbackImpl callback(scanClient, mountExclusions, currentExclusions, userDefinedExclusions);

    EXPECT_THROW(callback.processFile(symlink, true), std::exception);

    EXPECT_EQ(callback.returnCode(), E_CLEAN_SUCCESS);
}

TEST_F(TestBaseFileWalkCallbacks, scansFileViaSymlinkDir) // NOLINT
{
    fs::path testdir("testdir");
    fs::create_directory(testdir);
    fs::path testfile(testdir / "testfile");
    std::ofstream(testfile).close();
    fs::path symlink("symlink");
    fs::create_symlink(testdir, symlink);
    fs::path indirectFile = symlink / testfile.filename();

    auto scanClient = std::make_shared<StrictMock<MockScanClient>>();
    std::vector<fs::path> mountExclusions {};
    std::vector<Exclusion> currentExclusions {};
    std::vector<Exclusion> userDefinedExclusions {};

    CallbackImpl callback(scanClient, mountExclusions, currentExclusions, userDefinedExclusions);

    InSequence seq;
    EXPECT_CALL(callback, logScanningLine(indirectFile.string())).Times(1);
    EXPECT_CALL(*scanClient, scan(indirectFile, true)).Times(1);

    callback.processFile(indirectFile, true);

    EXPECT_EQ(callback.returnCode(), E_CLEAN_SUCCESS);
}

TEST_F(TestBaseFileWalkCallbacks, excludeStartingDirSymlink) // NOLINT
{
    fs::path testdir = "testdir";
    fs::create_directory(testdir);
    std::string symlinkString = "symlink";
    fs::path symlink = symlinkString;
    fs::create_symlink(testdir, symlink);

    auto scanClient = std::make_shared<StrictMock<MockScanClient>>();
    std::vector<fs::path> mountExclusions {};
    std::vector<Exclusion> currentExclusions {};
    Exclusion exclusion(symlinkString +"/");
    std::vector<Exclusion> userDefinedExclusions { exclusion };

    CallbackImpl callback(scanClient, mountExclusions, currentExclusions, userDefinedExclusions);

    EXPECT_TRUE(callback.userDefinedExclusionCheck(fs::path(".") / symlink, true));

    EXPECT_EQ(callback.returnCode(), E_CLEAN_SUCCESS);
}

TEST_F(TestBaseFileWalkCallbacks, DISABLED_excludeStartingDirSymlinkTarget) // NOLINT
{
    std::string testdirString = "testdir";
    fs::path testdir = testdirString;
    fs::create_directory(testdir);
    fs::path symlink = "symlink";
    fs::create_symlink(testdir, symlink);

    auto scanClient = std::make_shared<StrictMock<MockScanClient>>();
    std::vector<fs::path> mountExclusions {};
    std::vector<Exclusion> currentExclusions {};
    Exclusion exclusion(testdirString + "/");
    std::vector<Exclusion> userDefinedExclusions { exclusion };

    CallbackImpl callback(scanClient, mountExclusions, currentExclusions, userDefinedExclusions);

    EXPECT_TRUE(callback.userDefinedExclusionCheck(fs::path(".") / symlink, true));

    EXPECT_EQ(callback.returnCode(), E_CLEAN_SUCCESS);
}

TEST_F(TestBaseFileWalkCallbacks, excludeDirSymlinkByMount) // NOLINT
{
    fs::path testdir = "testdir";
    fs::create_directory(testdir);
    fs::path symlink = "symlink";
    fs::create_symlink(testdir, symlink);

    auto scanClient = std::make_shared<StrictMock<MockScanClient>>();
    std::vector<fs::path> mountExclusions {};
    Exclusion exclusion(fs::absolute(symlink).string() + "/");
    std::vector<Exclusion> currentExclusions { exclusion };
    std::vector<Exclusion> userDefinedExclusions {};

    CallbackImpl callback(scanClient, mountExclusions, currentExclusions, userDefinedExclusions);

    EXPECT_FALSE(callback.includeDirectory(fs::absolute(symlink)));

    EXPECT_EQ(callback.returnCode(), E_CLEAN_SUCCESS);
}

TEST_F(TestBaseFileWalkCallbacks, excludeDirSymlinkTargetByMount) // NOLINT
{
    fs::path testdir = "testdir";
    fs::create_directory(testdir);
    fs::path symlink = "symlink";
    fs::create_symlink(testdir, symlink);

    auto scanClient = std::make_shared<StrictMock<MockScanClient>>();
    std::string exclusionPath = fs::absolute(testdir).string() + "/";
    std::vector<fs::path> mountExclusions { exclusionPath };
    std::vector<Exclusion> currentExclusions {};
    std::vector<Exclusion> userDefinedExclusions {};

    CallbackImpl callback(scanClient, mountExclusions, currentExclusions, userDefinedExclusions);

    EXPECT_FALSE(callback.includeDirectory(fs::absolute(symlink)));

    EXPECT_EQ(callback.returnCode(), E_CLEAN_SUCCESS);
}

TEST_F(TestBaseFileWalkCallbacks, excludeDirSymlinkTargetByMountSubdir) // NOLINT
{
    fs::path testdir = "testdir";
    fs::create_directory(testdir);
    fs::path subdir = testdir / "subdir";
    fs::create_directory(subdir);
    fs::path symlink = "symlink";
    fs::create_symlink(subdir, symlink);

    auto scanClient = std::make_shared<StrictMock<MockScanClient>>();
    std::string exclusionPath = fs::absolute(testdir).string() + "/";
    std::vector<fs::path> mountExclusions { exclusionPath };
    std::vector<Exclusion> currentExclusions {};

    std::vector<Exclusion> userDefinedExclusions {};

    CallbackImpl callback(scanClient, mountExclusions, currentExclusions, userDefinedExclusions);

    EXPECT_FALSE(callback.includeDirectory(fs::absolute(symlink)));

    EXPECT_EQ(callback.returnCode(), E_CLEAN_SUCCESS);
}

TEST_F(TestBaseFileWalkCallbacks, excludeDirSymlinkTargetByMountNotPrefix) // NOLINT
{
    fs::path testdir = "testdir";
    fs::path testdir2 = "testdir2";
    fs::create_directory(testdir2);
    fs::path symlink = "symlink";
    fs::create_symlink(testdir2, symlink);

    auto scanClient = std::make_shared<StrictMock<MockScanClient>>();
    std::vector<fs::path> mountExclusions {};
    Exclusion exclusion(fs::absolute(testdir).string() + "/");
    std::vector<Exclusion> currentExclusions { exclusion };
    std::vector<Exclusion> userDefinedExclusions {};

    CallbackImpl callback(scanClient, mountExclusions, currentExclusions, userDefinedExclusions);

    EXPECT_TRUE(callback.includeDirectory(fs::absolute(symlink)));

    EXPECT_EQ(callback.returnCode(), E_CLEAN_SUCCESS);
}

TEST_F(TestBaseFileWalkCallbacks, excludeDirSymlinkByUser) // NOLINT
{
    fs::path testdir = "testdir";
    fs::create_directory(testdir);
    std::string symlinkString = "symlink";
    fs::path symlink = symlinkString;
    fs::create_symlink(testdir, symlink);

    auto scanClient = std::make_shared<StrictMock<MockScanClient>>();
    std::vector<fs::path> mountExclusions {};
    std::vector<Exclusion> currentExclusions {};
    Exclusion exclusion(symlinkString + "/");
    std::vector<Exclusion> userDefinedExclusions { exclusion };

    CallbackImpl callback(scanClient, mountExclusions, currentExclusions, userDefinedExclusions);

    EXPECT_FALSE(callback.includeDirectory(fs::path(".") / symlink));

    EXPECT_EQ(callback.returnCode(), E_CLEAN_SUCCESS);
}

TEST_F(TestBaseFileWalkCallbacks, excludeDirSymlinkTargetByUser) // NOLINT
{
    std::string testdirString = "testdir";
    fs::path testdir = testdirString;
    fs::create_directory(testdir);
    fs::path symlink = "symlink";
    fs::create_symlink(testdir, symlink);

    auto scanClient = std::make_shared<StrictMock<MockScanClient>>();
    std::vector<fs::path> mountExclusions {};
    std::vector<Exclusion> currentExclusions {};
    Exclusion exclusion(testdirString + "/");
    std::vector<Exclusion> userDefinedExclusions { exclusion };

    CallbackImpl callback(scanClient, mountExclusions, currentExclusions, userDefinedExclusions);

    EXPECT_FALSE(callback.includeDirectory(fs::path(".") / symlink));

    EXPECT_EQ(callback.returnCode(), E_CLEAN_SUCCESS);
}

// Simulate the situation where a symlink disappears during scanning.
// showing why we need test: "TestFileWalker, scanContinuesAfterIncludeDirectoryThrows"
TEST_F(TestBaseFileWalkCallbacks, brokenDirectorySymlink) // NOLINT
{
    fs::path testdir = "testdir";
    fs::path symlink = "symlink";
    fs::create_symlink(testdir, symlink);

    auto scanClient = std::make_shared<StrictMock<MockScanClient>>();
    std::vector<fs::path> mountExclusions {};
    std::vector<Exclusion> currentExclusions {};
    std::vector<Exclusion> userDefinedExclusions {};

    CallbackImpl callback(scanClient, mountExclusions, currentExclusions, userDefinedExclusions);

    EXPECT_THROW(callback.includeDirectory(fs::path(".") / symlink), std::exception);

    EXPECT_EQ(callback.returnCode(), E_CLEAN_SUCCESS);
}
