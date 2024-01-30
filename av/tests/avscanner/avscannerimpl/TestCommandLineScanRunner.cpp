// Copyright 2020-2023 Sophos Limited. All rights reserved.

#include "ScanRunnerMemoryAppenderUsingTests.h"

#include "avscanner/avscannerimpl/BaseFileWalkCallbacks.h"
#include "avscanner/avscannerimpl/CommandLineScanRunner.h"
#include "datatypes/sophos_filesystem.h"
#include "tests/common/MockMountPoint.h"
#include "tests/common/RecordingMockSocket.h"

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
#include "Common/Helpers/FileSystemReplaceAndRestore.h"
#include "Common/Helpers/MockFileSystem.h"

#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>

#include <fstream>

namespace fs = sophos_filesystem;

using namespace avscanner::avscannerimpl;
using namespace mount_monitor::mountinfo;
using namespace common;

using namespace testing;

namespace
{
    class TestCommandLineScanRunner : public ScanRunnerMemoryAppenderUsingTests
    {
    protected:
        void SetUp() override
        {
            auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
            appConfig.setData("PLUGIN_INSTALL", "/tmp/TestCommandLineScanRunner");

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
}

TEST_F(TestCommandLineScanRunner, construction)
{
    std::vector<std::string> paths;
    std::vector<std::string> exclusions;
    Options options(false, paths, exclusions, false, false, false, false);
    CommandLineScanRunner runner(options);
}

TEST_F(TestCommandLineScanRunner, constructionWithScanArchives)
{
    std::vector<std::string> paths;
    std::vector<std::string> exclusions;
    Options options(false, paths, exclusions, true, false, false, false);
    CommandLineScanRunner runner(options);
}

TEST_F(TestCommandLineScanRunner, constructionWithScanImages)
{
    std::vector<std::string> paths;
    std::vector<std::string> exclusions;
    Options options(false, paths, exclusions, false, true, false, false);
    CommandLineScanRunner runner(options);
}

TEST_F(TestCommandLineScanRunner, constructionWithCustomInstallLocationFromAVScannerPath)
{
    auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
    appConfig.clearData("PLUGIN_INSTALL");
    auto mockIFileSystemPtr = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockIFileSystemPtr, readlink("/usr/local/bin/avscanner")).WillOnce(Return("/usr/lib64/sophos-spl/plugins/av/bin/avscanner"));

    std::vector<std::string> paths;
    std::vector<std::string> exclusions;
    Options options(false, paths, exclusions, false, false, false, false);
    Tests::ScopedReplaceFileSystem replacer(std::move(mockIFileSystemPtr));
    CommandLineScanRunner runner(options);

    EXPECT_EQ(Common::ApplicationConfiguration::applicationConfiguration().getData("PLUGIN_INSTALL"), "/usr/lib64/sophos-spl/plugins/av");
}

TEST_F(TestCommandLineScanRunner, constructionWithCustomInstallLocationFromProcSelf)
{
    auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
    appConfig.clearData("PLUGIN_INSTALL");
    auto mockIFileSystemPtr = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockIFileSystemPtr, readlink("/usr/local/bin/avscanner")).WillOnce(Return(std::nullopt));
    EXPECT_CALL(*mockIFileSystemPtr, readlink("/proc/self/exe")).WillOnce(Return("/usr/lib64/sophos-spl/plugins/av/bin/avscanner"));

    std::vector<std::string> paths;
    std::vector<std::string> exclusions;
    Options options(false, paths, exclusions, false, false, false, false);
    Tests::ScopedReplaceFileSystem replacer(std::move(mockIFileSystemPtr));
    CommandLineScanRunner runner(options);

    EXPECT_EQ(Common::ApplicationConfiguration::applicationConfiguration().getData("PLUGIN_INSTALL"), "/usr/lib64/sophos-spl/plugins/av");
}

TEST_F(TestCommandLineScanRunner, clsThrowsWhenInstallLocationCannotBeFound)
{
    auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
    appConfig.clearData("PLUGIN_INSTALL");
    auto mockIFileSystemPtr = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockIFileSystemPtr, readlink("/usr/local/bin/avscanner")).WillOnce(Return(std::nullopt));
    EXPECT_CALL(*mockIFileSystemPtr, readlink("/proc/self/exe")).WillOnce(Return(std::nullopt));
    EXPECT_CALL(*mockIFileSystemPtr, exists("/opt/sophos-spl/plugins/av")).WillOnce(Return(false));

    std::vector<std::string> paths;
    std::vector<std::string> exclusions;
    Tests::ScopedReplaceFileSystem replacer(std::move(mockIFileSystemPtr));
    Options options(false, paths, exclusions, false, false, false, false);
    EXPECT_THROW(CommandLineScanRunner runner(options), AbortScanException);
}

TEST_F(TestCommandLineScanRunner, scanRelativePath)
{
    fs::create_directories("sandbox/a/b/d/e");
    std::ofstream("sandbox/a/b/file1.txt").close();

    fs::path startingpoint = fs::absolute("sandbox");

    std::vector<std::string> paths;
    paths.emplace_back(startingpoint);
    std::vector<std::string> exclusions;
    Options options(false, paths, exclusions, false, false, false, false);
    CommandLineScanRunner runner(options);

    auto socket = std::make_shared<RecordingMockSocket>();
    runner.setSocket(socket);
    runner.run();

    ASSERT_EQ(socket->m_paths.size(), 1);
    EXPECT_EQ(socket->m_paths.at(0), fs::absolute("sandbox/a/b/file1.txt").string());
}

TEST_F(TestCommandLineScanRunner, scanSameDirectoryTwice)
{
    fs::create_directories("sandbox/a/b/d/e");
    std::ofstream("sandbox/a/b/file1.txt").close();

    fs::path startingpoint = fs::absolute("sandbox");

    std::vector<std::string> paths;
    paths.emplace_back(startingpoint);
    paths.emplace_back(startingpoint);
    std::vector<std::string> exclusions;
    Options options(false, paths, exclusions, false, false, false, false);
    CommandLineScanRunner runner(options);

    auto socket = std::make_shared<RecordingMockSocket>();
    runner.setSocket(socket);
    runner.run();

    ASSERT_EQ(socket->m_paths.size(), 1);
    EXPECT_EQ(socket->m_paths.at(0), fs::absolute("sandbox/a/b/file1.txt").string());
}

TEST_F(TestCommandLineScanRunner, scanSymlinkedPath)
{
    fs::create_directories("sandbox/a/b/d/e");
    fs::create_directories("sandbox/f");
    fs::create_symlink(fs::absolute("sandbox/a"), "sandbox/f/a");
    std::ofstream("sandbox/a/b/file1.txt").close();

    fs::path startingpoint = fs::absolute("sandbox/f");

    std::vector<std::string> paths;
    paths.emplace_back(startingpoint);
    std::vector<std::string> exclusions;
    Options options(false, paths, exclusions, false, false, false, true);
    CommandLineScanRunner runner(options);

    auto socket = std::make_shared<RecordingMockSocket>();
    runner.setSocket(socket);
    runner.run();

    ASSERT_EQ(socket->m_paths.size(), 1);
    EXPECT_EQ(socket->m_paths.at(0), fs::absolute("sandbox/f/a/b/file1.txt").string());
}

TEST_F(TestCommandLineScanRunner, doNotScanSymlinkedPath)
{
    fs::create_directories("sandbox/a/b/d/e");
    fs::create_directories("sandbox/f");
    fs::create_symlink(fs::absolute("sandbox/a"), "sandbox/f/a");
    std::ofstream("sandbox/a/b/file1.txt").close();

    fs::path startingpoint = fs::absolute("sandbox/f");

    std::vector<std::string> paths;
    paths.emplace_back(startingpoint);
    std::vector<std::string> exclusions;
    Options options(false, paths, exclusions, false, false, false, false);
    CommandLineScanRunner runner(options);

    auto socket = std::make_shared<RecordingMockSocket>();
    runner.setSocket(socket);
    runner.run();

    ASSERT_EQ(socket->m_paths.size(), 0);
}

TEST_F(TestCommandLineScanRunner, scanDirectoryAndSymlinkToDirectory)
{
    fs::create_directories("sandbox/a/b/d/e");
    fs::create_directories("sandbox/f");
    fs::create_symlink(fs::absolute("sandbox/a"), "sandbox/f/a");
    std::ofstream("sandbox/a/b/file1.txt").close();

    fs::path symlinkStartingPoint = fs::absolute("sandbox/f");
    fs::path startingpoint = fs::absolute("sandbox/a");

    std::vector<std::string> paths;
    paths.emplace_back(symlinkStartingPoint);
    paths.emplace_back(startingpoint);
    std::vector<std::string> exclusions;
    Options options(false, paths, exclusions, false, false, false, true);
    CommandLineScanRunner runner(options);

    auto socket = std::make_shared<RecordingMockSocket>();
    runner.setSocket(socket);
    runner.run();

    ASSERT_EQ(socket->m_paths.size(), 1);
    EXPECT_EQ(socket->m_paths.at(0), fs::absolute("sandbox/f/a/b/file1.txt").string());
}

TEST_F(TestCommandLineScanRunner, scanNonCanonicalPath)
{
    fs::create_directories("sandbox/a/b/d/e");
    fs::create_directories("sandbox/b/d/e");
    fs::create_directories("sandbox/d/e");
    std::ofstream("sandbox/a/b/file1.txt").close();
    std::ofstream("sandbox/b/d/file1.txt").close();
    std::ofstream("sandbox/d/e/file1.txt").close();

    std::vector<std::string> paths;
    paths.emplace_back(fs::absolute("./sandbox/a"));
    paths.emplace_back(fs::absolute( "sandbox/b/../b"));
    paths.emplace_back(fs::absolute( "sandbox/d/.."));
    std::vector<std::string> exclusions;
    Options options(false, paths, exclusions, false, false, false, false);
    CommandLineScanRunner runner(options);

    auto socket = std::make_shared<RecordingMockSocket>();
    runner.setSocket(socket);
    runner.run();

    ASSERT_EQ(socket->m_paths.size(), 3);
    EXPECT_EQ(socket->m_paths.at(0), fs::absolute("sandbox/a/b/file1.txt").string());
    EXPECT_EQ(socket->m_paths.at(1), fs::absolute("sandbox/b/d/file1.txt").string());
    EXPECT_EQ(socket->m_paths.at(2), fs::absolute("sandbox/d/e/file1.txt").string());
}

TEST_F(TestCommandLineScanRunner, scanAbsolutePath)
{
    fs::create_directories("sandbox/a/b/d/e");
    std::ofstream("sandbox/a/b/file1.txt").close();

    fs::path startingpoint = fs::absolute("sandbox");

    std::vector<std::string> paths;
    paths.emplace_back(startingpoint);
    std::vector<std::string> exclusions;
    Options options(false, paths, exclusions, false, false, false, false);
    avscanner::avscannerimpl::CommandLineScanRunner runner(options);

    auto socket = std::make_shared<RecordingMockSocket>();
    runner.setSocket(socket);
    runner.run();

    ASSERT_EQ(socket->m_paths.size(), 1);
    EXPECT_EQ(socket->m_paths.at(0), fs::absolute("sandbox/a/b/file1.txt").string());
}

TEST_F(TestCommandLineScanRunner, scanRelativeDirectory)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    fs::create_directories("sandbox/a/b/d/e");
    std::ofstream("sandbox/a/b/file1.txt").close();

    std::vector<std::string> paths;
    paths.emplace_back("sandbox/");
    std::vector<std::string> exclusions;
    Options options(false, paths, exclusions, false, false, false, false);
    avscanner::avscannerimpl::CommandLineScanRunner runner(options);

    auto socket = std::make_shared<RecordingMockSocket>();
    runner.setSocket(socket);
    runner.run();

    EXPECT_TRUE(appenderContains("Archive scanning enabled: no"));

    ASSERT_EQ(socket->m_paths.size(), 1);
    EXPECT_EQ(socket->m_paths.at(0), fs::absolute("sandbox/a/b/file1.txt").string());
}

TEST_F(TestCommandLineScanRunner, scanAbsoluteDirectory)
{
    fs::create_directories("sandbox/a/b/d/e");
    std::ofstream("sandbox/a/b/file1.txt").close();

    std::vector<std::string> paths;
    paths.emplace_back(fs::absolute("sandbox/"));
    std::vector<std::string> exclusions;
    Options options(false, paths, exclusions, false, false, false, false);
    avscanner::avscannerimpl::CommandLineScanRunner runner(options);

    auto socket = std::make_shared<RecordingMockSocket>();
    runner.setSocket(socket);
    runner.run();

    ASSERT_EQ(socket->m_paths.size(), 1);
    EXPECT_EQ(socket->m_paths.at(0), fs::absolute("sandbox/a/b/file1.txt").string());
}

TEST_F(TestCommandLineScanRunner, scanRelativeDirectoryWithScanArchives)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    fs::create_directories("sandbox/a/b/d/e");
    std::ofstream("sandbox/a/b/file1.txt").close();

    std::vector<std::string> paths;
    paths.emplace_back("sandbox");
    std::vector<std::string> exclusions;
    Options options(false, paths, exclusions, true, false, false, false);
    avscanner::avscannerimpl::CommandLineScanRunner runner(options);

    auto socket = std::make_shared<RecordingMockSocket>();
    runner.setSocket(socket);
    runner.run();

    EXPECT_TRUE(appenderContains("Archive scanning enabled: yes"));
    EXPECT_TRUE(appenderContains("Image scanning enabled: no"));

    ASSERT_EQ(socket->m_paths.size(), 1);
    EXPECT_EQ(socket->m_paths.at(0), fs::absolute("sandbox/a/b/file1.txt").string());
}

TEST_F(TestCommandLineScanRunner, scanRelativeDirectoryWithScanImages)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    fs::create_directories("sandbox/a/b/d/e");
    std::ofstream("sandbox/a/b/file1.iso").close();

    std::vector<std::string> paths;
    paths.emplace_back("sandbox");
    std::vector<std::string> exclusions;
    Options options(false, paths, exclusions, false, true, false, false);
    avscanner::avscannerimpl::CommandLineScanRunner runner(options);

    auto socket = std::make_shared<RecordingMockSocket>();
    runner.setSocket(socket);
    runner.run();

    EXPECT_TRUE(appenderContains("Archive scanning enabled: no"));
    EXPECT_TRUE(appenderContains("Image scanning enabled: yes"));

    ASSERT_EQ(socket->m_paths.size(), 1);
    EXPECT_EQ(socket->m_paths.at(0), fs::absolute("sandbox/a/b/file1.iso").string());
}

TEST_F(TestCommandLineScanRunner, scanAbsoluteDirectoryWithFilenameExclusion)
{
    fs::create_directories("sandbox/a/b/d/e");
    std::ofstream("sandbox/a/b/file1.txt").close();
    std::ofstream("sandbox/a/b/file2.txt").close();

    std::vector<std::string> paths;
    paths.emplace_back(fs::absolute("sandbox"));
    std::vector<std::string> exclusions;
    exclusions.emplace_back("file1.txt");
    Options options(false, paths, exclusions, true, false, false, false);
    avscanner::avscannerimpl::CommandLineScanRunner runner(options);

    auto socket = std::make_shared<RecordingMockSocket>();
    runner.setSocket(socket);
    runner.run();

    ASSERT_EQ(socket->m_paths.size(), 1);
    EXPECT_EQ(socket->m_paths.at(0), fs::absolute("sandbox/a/b/file2.txt"));
}

TEST_F(TestCommandLineScanRunner, exclusionIsFileToScan)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    fs::create_directories("sandbox/a/b/d/e");
    std::ofstream("sandbox/a/b/file1.txt").close();

    std::vector<std::string> paths;
    paths.emplace_back(fs::absolute("sandbox/a/b/file1.txt"));
    std::vector<std::string> exclusions;
    exclusions.emplace_back(fs::absolute("sandbox/a/b/file1.txt"));
    Options options(false, paths, exclusions, true, false, false, false);
    avscanner::avscannerimpl::CommandLineScanRunner runner(options);

    auto socket = std::make_shared<RecordingMockSocket>();
    runner.setSocket(socket);
    runner.run();

    EXPECT_TRUE(appenderContains("Excluding file: " + fs::absolute("sandbox/a/b/file1.txt").string()));
    ASSERT_EQ(socket->m_paths.size(), 0);
}

TEST_F(TestCommandLineScanRunner, anEmptyExclusionProvided)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    fs::create_directories("sandbox/a/b/d/e");
    std::ofstream("sandbox/a/b/file1.txt").close();

    std::vector<std::string> paths;
    paths.emplace_back(fs::absolute("sandbox/a/b/file1.txt"));
    std::vector<std::string> exclusions;
    exclusions.emplace_back(fs::absolute("sandbox/a/b/file1.txt"));
    exclusions.emplace_back("");
    Options options(false, paths, exclusions, true, false, false, false);
    avscanner::avscannerimpl::CommandLineScanRunner runner(options);

    auto socket = std::make_shared<RecordingMockSocket>();
    runner.setSocket(socket);
    runner.run();

    EXPECT_TRUE(appenderContains("Excluding file: " + fs::absolute("sandbox/a/b/file1.txt").string()));
    EXPECT_TRUE(appenderContains("Refusing to exclude empty path"));
    ASSERT_EQ(socket->m_paths.size(), 0);
}

TEST_F(TestCommandLineScanRunner, exclusionIsDirectoryToScan)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    fs::create_directories("sandbox/a/b/d/e");
    std::ofstream("sandbox/a/b/file1.txt").close();
    std::ofstream("sandbox/a/b/d/file2.txt").close();

    std::vector<std::string> paths;
    paths.emplace_back(fs::absolute("sandbox/a/b/"));
    std::vector<std::string> exclusions;
    exclusions.emplace_back(fs::absolute("sandbox/a/b/"));
    Options options(false, paths, exclusions, true, false, false, false);
    avscanner::avscannerimpl::CommandLineScanRunner runner(options);

    auto socket = std::make_shared<RecordingMockSocket>();
    runner.setSocket(socket);
    runner.run();

    EXPECT_TRUE(appenderContains("Excluding directory: " + fs::absolute("sandbox/a/b/").string()));
    EXPECT_FALSE(appenderContains("Excluding file: " + fs::absolute("sandbox/a/b/file1.txt").string()));
    EXPECT_FALSE(appenderContains("Excluding file: " + fs::absolute("sandbox/a/b/d/file2.txt").string()));

    ASSERT_EQ(socket->m_paths.size(), 0);
}

TEST_F(TestCommandLineScanRunner, scanDirectoryExcludedAsFilePath)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    fs::create_directories("sandbox/a/b/d/e");
    std::ofstream("sandbox/a/b/file1.txt").close();

    std::vector<std::string> paths;
    paths.emplace_back("sandbox");
    std::vector<std::string> exclusions;
    exclusions.emplace_back(fs::absolute("sandbox"));
    exclusions.emplace_back("sandbox");
    Options options(false, paths, exclusions, false, false, false, false);
    avscanner::avscannerimpl::CommandLineScanRunner runner(options);

    auto socket = std::make_shared<RecordingMockSocket>();
    runner.setSocket(socket);
    runner.run();

    EXPECT_TRUE(appenderContains("Archive scanning enabled: no"));
    EXPECT_TRUE(appenderContains("Image scanning enabled: no"));

    ASSERT_EQ(socket->m_paths.size(), 1);
    EXPECT_EQ(socket->m_paths.at(0), fs::absolute("sandbox/a/b/file1.txt").string());
}

TEST_F(TestCommandLineScanRunner, scanFileExcludedAsDirectoryPath)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    fs::create_directories("sandbox/a/b/d/e");
    std::ofstream("sandbox/a/b/file1.txt").close();

    std::vector<std::string> paths;
    paths.emplace_back("sandbox/a/b/file1.txt");
    std::vector<std::string> exclusions;
    exclusions.emplace_back(fs::absolute("sandbox/a/b/file1.txt/"));
    exclusions.emplace_back("sandbox/a/b/file1.txt/");
    Options options(false, paths, exclusions, false, false, false, false);
    avscanner::avscannerimpl::CommandLineScanRunner runner(options);

    auto socket = std::make_shared<RecordingMockSocket>();
    runner.setSocket(socket);
    runner.run();

    EXPECT_TRUE(appenderContains("Archive scanning enabled: no"));
    EXPECT_TRUE(appenderContains("Image scanning enabled: no"));

    ASSERT_EQ(socket->m_paths.size(), 1);
    EXPECT_EQ(socket->m_paths.at(0), fs::absolute("sandbox/a/b/file1.txt").string());
}

TEST_F(TestCommandLineScanRunner, scanAbsoluteDirectoryWithStemExclusion)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    fs::create_directories("sandbox/a/b/d/e");
    fs::create_directories("sandbox/a/f");
    std::ofstream("sandbox/a/b/file1.txt").close();
    std::ofstream("sandbox/a/f/file2.txt").close();

    std::vector<std::string> paths;
    paths.emplace_back(fs::absolute("sandbox"));
    std::vector<std::string> exclusions;
    exclusions.emplace_back(fs::absolute("sandbox/a/b/"));
    Options options(false, paths, exclusions, false, false, false, false);
    avscanner::avscannerimpl::CommandLineScanRunner runner(options);

    auto socket = std::make_shared<RecordingMockSocket>();
    runner.setSocket(socket);
    runner.run();


    EXPECT_TRUE(appenderContains("Excluding directory: " + fs::absolute("sandbox/a/b/").string()));
    EXPECT_TRUE(appenderContains("Scanning " + fs::absolute("sandbox/a/f/file2.txt").string()));
    EXPECT_FALSE(appenderContains("Excluding file: " + fs::absolute("sandbox/a/b/file1.txt").string()));

    ASSERT_EQ(socket->m_paths.size(), 1);
    EXPECT_EQ(socket->m_paths.at(0), fs::absolute("sandbox/a/f/file2.txt").string());
}

TEST_F(TestCommandLineScanRunner, scanAbsoluteDirectoryWithFullPathExclusion)
{
    fs::create_directories("sandbox/a/b/d/e");
    std::ofstream("sandbox/a/b/file1.txt").close();
    std::ofstream("sandbox/a/b/file2.txt").close();

    std::vector<std::string> paths;
    paths.emplace_back(fs::absolute("sandbox"));
    std::vector<std::string> exclusions;
    exclusions.emplace_back(fs::absolute("sandbox/a/b/file2.txt"));
    Options options(false, paths, exclusions, true, false, false, false);
    avscanner::avscannerimpl::CommandLineScanRunner runner(options);

    auto socket = std::make_shared<RecordingMockSocket>();
    runner.setSocket(socket);
    runner.run();

    ASSERT_EQ(socket->m_paths.size(), 1);
    EXPECT_EQ(socket->m_paths.at(0), fs::absolute("sandbox/a/b/file1.txt"));
}

TEST_F(TestCommandLineScanRunner, scanAbsoluteDirectoryWithGlobExclusion)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    fs::create_directories("sandbox/a/b/d/e");
    fs::create_directories("sandbox/a/f");
    std::ofstream("sandbox/a/b/file1.txt").close();
    std::ofstream("sandbox/a/f/file2.txt").close();

    std::vector<std::string> paths;
    paths.emplace_back(fs::absolute("sandbox"));
    std::vector<std::string> exclusions;
    exclusions.emplace_back("sandbox/a/b/");
    Options options(false, paths, exclusions, false, false, false, false);
    avscanner::avscannerimpl::CommandLineScanRunner runner(options);

    auto socket = std::make_shared<RecordingMockSocket>();
    runner.setSocket(socket);
    runner.run();

    EXPECT_TRUE(appenderContains("Excluding directory: " + fs::absolute("sandbox/a/b/").string()));
    EXPECT_TRUE(appenderContains("Scanning " + fs::absolute("sandbox/a/f/file2.txt").string()));
    EXPECT_FALSE(appenderContains("Excluding file: " + fs::absolute("sandbox/a/b/file1.txt").string()));

    ASSERT_EQ(socket->m_paths.size(), 1);
    EXPECT_EQ(socket->m_paths.at(0), fs::absolute("sandbox/a/f/file2.txt"));
}

TEST_F(TestCommandLineScanRunner, nonCanonicalExclusions)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    fs::create_directories("sandbox/a/b/d/e");
    fs::create_directories("sandbox/a/f");
    fs::create_directories("sandbox/a/g");
    std::ofstream("sandbox/a/b/file1.txt").close();
    std::ofstream("sandbox/a/f/file2.txt").close();

    std::vector<std::string> paths;
    paths.emplace_back(fs::absolute("sandbox"));
    std::vector<std::string> exclusions;
    exclusions.emplace_back(fs::absolute("sandbox/./a/f/"));
    exclusions.emplace_back(fs::absolute("sandbox/a/g/."));
    exclusions.emplace_back(fs::absolute("sandbox/../sandbox/a/b/"));
    Options options(false, paths, exclusions, true, false, false, false);
    avscanner::avscannerimpl::CommandLineScanRunner runner(options);

    auto socket = std::make_shared<RecordingMockSocket>();
    runner.setSocket(socket);
    runner.run();

    std::string expected = "Exclusions: ";
    expected += fs::absolute("sandbox/a/f/").string();
    expected += ", " + fs::absolute("sandbox/a/g/").string();
    expected += ", " + fs::absolute("sandbox/a/b/").string();
    EXPECT_TRUE(appenderContains(expected)) << "Did not contain: \"" << expected << "\"";

    EXPECT_TRUE(appenderContains("Excluding directory: " + fs::absolute("sandbox/a/b/").string()));
    EXPECT_TRUE(appenderContains("Excluding directory: " + fs::absolute("sandbox/a/f/").string()));
    EXPECT_TRUE(appenderContains("Excluding directory: " + fs::absolute("sandbox/a/g/").string()));
    EXPECT_FALSE(appenderContains("Scanning " + fs::absolute("sandbox/a/b/file1.txt").string()));
    EXPECT_FALSE(appenderContains("Scanning " + fs::absolute("sandbox/a/f/file2.txt").string()));
    EXPECT_FALSE(appenderContains("Excluding file: " + fs::absolute("sandbox/a/b/file1.txt").string()));
    EXPECT_FALSE(appenderContains("Excluding file: " + fs::absolute("sandbox/a/f/file2.txt").string()));

    EXPECT_EQ(socket->m_paths.size(), 0);
}

TEST_F(TestCommandLineScanRunner, nonCanonicalNonExistentExclusions)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    fs::create_directories("sandbox/a/b/d/e");
    fs::create_directories("sandbox/a/f");
    std::ofstream("sandbox/a/b/file1.txt").close();
    std::ofstream("sandbox/a/f/file2.txt").close();

    std::vector<std::string> paths;
    paths.emplace_back(fs::absolute("sandbox"));
    std::vector<std::string> exclusions;
    exclusions.emplace_back(fs::absolute("sandbox/./a/f/"));
    exclusions.emplace_back(fs::absolute("does_not_exist/./a/f/"));
    exclusions.emplace_back(fs::absolute("sandbox/a/f/."));
    exclusions.emplace_back(fs::absolute(".does_not_exist/a/f/"));
    exclusions.emplace_back(fs::absolute("sandbox/../sandbox/a/b/"));
    Options options(false, paths, exclusions, true, false, false, false);
    avscanner::avscannerimpl::CommandLineScanRunner runner(options);

    auto socket = std::make_shared<RecordingMockSocket>();
    runner.setSocket(socket);
    runner.run();

    std::string expected = "Exclusions: ";
    expected += fs::absolute("sandbox/a/f/").string();
    expected += ", " + fs::absolute("does_not_exist/a/f/").string();
    expected += ", " + fs::absolute("sandbox/a/f/").string();
    expected += ", " + fs::absolute(".does_not_exist/a/f/").string();
    expected += ", " + fs::absolute("sandbox/a/b/").string();
    EXPECT_TRUE(appenderContains(expected)) << "Did not contain: \"" << expected << "\"";

    EXPECT_TRUE(appenderContains("Excluding directory: " + fs::absolute("sandbox/a/b/").string()));
    EXPECT_TRUE(appenderContains("Excluding directory: " + fs::absolute("sandbox/a/b/").string()));
    EXPECT_TRUE(appenderContains("Excluding directory: " + fs::absolute("sandbox/a/f/").string()));
    EXPECT_FALSE(appenderContains("Scanning " + fs::absolute("sandbox/a/b/file1.txt").string()));
    EXPECT_FALSE(appenderContains("Scanning " + fs::absolute("sandbox/a/f/file2.txt").string()));
    EXPECT_FALSE(appenderContains("Excluding file: " + fs::absolute("sandbox/a/b/file1.txt").string()));
    EXPECT_FALSE(appenderContains("Excluding file: " + fs::absolute("sandbox/a/f/file2.txt").string()));

    ASSERT_EQ(socket->m_paths.size(), 0);
}

TEST_F(TestCommandLineScanRunner, nonCanonicalExclusionsRootExclusion)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    fs::create_directories("sandbox/a/b/d/e");
    fs::create_directories("sandbox/a/f");
    std::ofstream("sandbox/a/b/file1.txt").close();
    std::ofstream("sandbox/a/f/file2.txt").close();

    std::vector<std::string> paths;
    paths.emplace_back(fs::absolute("sandbox"));
    std::vector<std::string> exclusions;
    exclusions.emplace_back("/.");

    Options options(false, paths, exclusions, true, false, false, false);
    avscanner::avscannerimpl::CommandLineScanRunner runner(options);

    auto socket = std::make_shared<RecordingMockSocket>();
    runner.setSocket(socket);
    runner.run();

    EXPECT_TRUE(appenderContains("Exclusions: /"));
    EXPECT_TRUE(appenderContains("Excluding directory: " + fs::absolute("sandbox/").string()));
    EXPECT_FALSE(appenderContains("Scanning " + fs::absolute("sandbox/a/b/file1.txt").string()));
    EXPECT_FALSE(appenderContains("Scanning " + fs::absolute("sandbox/a/f/file2.txt").string()));
    EXPECT_FALSE(appenderContains("Excluding file: " + fs::absolute("sandbox/a/b/file1.txt").string()));
    EXPECT_FALSE(appenderContains("Excluding file: " + fs::absolute("sandbox/a/f/file2.txt").string()));

    ASSERT_EQ(socket->m_paths.size(), 0);
}

TEST_F(TestCommandLineScanRunner, nonCanonicalExclusionsWithFilename)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    fs::create_directories("sandbox/a/b/d/e");
    fs::create_directories("sandbox/a/f");
    std::ofstream("sandbox/a/b/file1.txt").close();
    std::ofstream("sandbox/a/f/file2.txt").close();

    std::vector<std::string> paths;
    paths.emplace_back(fs::absolute("sandbox"));
    std::vector<std::string> exclusions;
    exclusions.emplace_back(fs::absolute("sandbox/./a/f/file2.txt"));
    exclusions.emplace_back(fs::absolute("sandbox/../sandbox/a/b/file1.txt"));
    Options options(false, paths, exclusions, true, false, false, false);
    avscanner::avscannerimpl::CommandLineScanRunner runner(options);

    auto socket = std::make_shared<RecordingMockSocket>();
    runner.setSocket(socket);
    runner.run();

    std::string expected = "Exclusions: ";
    expected += fs::absolute("sandbox/a/f/file2.txt").string();
    expected += ", " + fs::absolute("sandbox/a/b/file1.txt").string();
    EXPECT_TRUE(appenderContains(expected)) << "Did not contain: \"" << expected << "\"";

    EXPECT_TRUE(appenderContains("Excluding file: " + fs::absolute("sandbox/a/b/file1.txt").string()));
    EXPECT_TRUE(appenderContains("Excluding file: " + fs::absolute("sandbox/a/f/file2.txt").string()));

    ASSERT_EQ(socket->m_paths.size(), 0);
}

TEST_F(TestCommandLineScanRunner, scanSymlinkWithAbsoluteTargetExclusion)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    fs::create_directories("symlink_sandbox");
    std::ofstream("symlink_sandbox/file1.txt").close();

    fs::path startingPoint = fs::path("symlink_to_sandbox_file");
    fs::create_symlink("symlink_sandbox/file1.txt", fs::absolute(startingPoint));

    std::vector<std::string> paths;
    paths.emplace_back(fs::absolute(startingPoint));
    std::vector<std::string> exclusions;
    exclusions.emplace_back(fs::absolute("symlink_sandbox/file1.txt"));
    Options options(false, paths, exclusions, true, false, false, false);
    avscanner::avscannerimpl::CommandLineScanRunner runner(options);

    auto socket = std::make_shared<RecordingMockSocket>();
    runner.setSocket(socket);
    runner.run();

    EXPECT_TRUE(appenderContains("Skipping the scanning of symlink target (\"" + fs::absolute("symlink_sandbox/file1.txt").string()));
    EXPECT_TRUE(appenderContains("which is excluded by user defined exclusion: " + fs::absolute("symlink_sandbox/file1.txt").string()));
    ASSERT_EQ(socket->m_paths.size(), 0);
}

TEST_F(TestCommandLineScanRunner, scanSymlinkWithRelativeTargetExclusion)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    fs::create_directories("symlink_sandbox");
    std::ofstream("symlink_sandbox/file1.txt").close();

    fs::path startingPoint = fs::path("symlink_to_sandbox_file");
    fs::create_symlink("symlink_sandbox/file1.txt", fs::absolute(startingPoint));

    std::vector<std::string> paths;
    paths.emplace_back(fs::absolute(startingPoint));
    std::vector<std::string> exclusions;
    exclusions.emplace_back("symlink_sandbox/file1.txt");
    Options options(false, paths, exclusions, true, false, false, false);

    auto originalWD = fs::current_path();
    fs::current_path("../");
    avscanner::avscannerimpl::CommandLineScanRunner runner(options);

    auto socket = std::make_shared<RecordingMockSocket>();
    runner.setSocket(socket);
    runner.run();
    fs::current_path(originalWD);

    EXPECT_TRUE(appenderContains("Skipping the scanning of symlink target (\"" + fs::absolute("symlink_sandbox/file1.txt").string()));
    EXPECT_TRUE(appenderContains("which is excluded by user defined exclusion: symlink_sandbox/file1.txt"));
    ASSERT_EQ(socket->m_paths.size(), 0);
}

TEST_F(TestCommandLineScanRunner, scanSymlinkWithAbsoluteDirectExclusion)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    fs::create_directories("symlink_sandbox");
    std::ofstream("symlink_sandbox/file1.txt").close();

    fs::path startingPoint = fs::path("symlink_to_sandbox_file");
    fs::create_symlink("symlink_sandbox/file1.txt", fs::absolute(startingPoint));

    std::vector<std::string> paths;
    paths.emplace_back(fs::absolute(startingPoint));
    std::vector<std::string> exclusions;
    exclusions.emplace_back(fs::absolute(startingPoint));
    Options options(false, paths, exclusions, true, false, false, false);

    avscanner::avscannerimpl::CommandLineScanRunner runner(options);

    auto socket = std::make_shared<RecordingMockSocket>();
    runner.setSocket(socket);
    runner.run();

    EXPECT_TRUE(appenderContains("Excluding symlinked file: /tmp/TestCommandLineScanRunner/scanSymlinkWithAbsoluteDirectExclusion/symlink_to_sandbox_file"));
    ASSERT_EQ(socket->m_paths.size(), 0);
}

TEST_F(TestCommandLineScanRunner, scanSymlinkWithRelativeDirectExclusion)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    fs::create_directories("symlink_sandbox");
    std::ofstream("symlink_sandbox/file1.txt").close();

    fs::path startingPoint = fs::path("symlink_to_sandbox_file");
    fs::create_symlink("symlink_sandbox/file1.txt", fs::absolute(startingPoint));

    std::vector<std::string> paths;
    paths.emplace_back(fs::absolute(startingPoint));
    std::vector<std::string> exclusions;
    exclusions.emplace_back(startingPoint);
    Options options(false, paths, exclusions, true, false, false, false);

    auto originalWD = fs::current_path();
    fs::current_path("../");
    avscanner::avscannerimpl::CommandLineScanRunner runner(options);

    auto socket = std::make_shared<RecordingMockSocket>();
    runner.setSocket(socket);
    runner.run();
    fs::current_path(originalWD);

    EXPECT_TRUE(appenderContains("Excluding symlinked file: /tmp/TestCommandLineScanRunner/scanSymlinkWithRelativeDirectExclusion/symlink_to_sandbox_file"));
    ASSERT_EQ(socket->m_paths.size(), 0);
}

TEST_F(TestCommandLineScanRunner, scanSymlinkedDirectoryWithRelativeTargetExclusion)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    fs::create_directories("symlink_sandbox/a/b/c");
    std::ofstream("symlink_sandbox/a/b/c/file1.txt").close();

    fs::path startingPoint = fs::path("symlink_to_sandbox_directory");
    fs::create_symlink("symlink_sandbox/", fs::absolute(startingPoint));

    std::vector<std::string> paths;
    paths.emplace_back(fs::absolute(startingPoint));
    std::vector<std::string> exclusions;
    exclusions.emplace_back("symlink_sandbox/");
    Options options(false, paths, exclusions, true, false, false, false);

    auto originalWD = fs::current_path();
    fs::current_path("../");
    avscanner::avscannerimpl::CommandLineScanRunner runner(options);

    auto socket = std::make_shared<RecordingMockSocket>();
    runner.setSocket(socket);
    runner.run();
    fs::current_path(originalWD);

    EXPECT_TRUE(appenderContains("Skipping the scanning of symlink target (\"" + fs::absolute("symlink_sandbox").string()));
    EXPECT_TRUE(appenderContains("which is excluded by user defined exclusion: symlink_sandbox/"));
    ASSERT_EQ(socket->m_paths.size(), 0);
}

TEST_F(TestCommandLineScanRunner, scanSymlinkedDirectoryWithAbsoluteTargetExclusion)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    fs::create_directories("symlink_sandbox/a/b/c");
    std::ofstream("symlink_sandbox/a/b/c/file1.txt").close();

    fs::path startingPoint = fs::path("symlink_to_sandbox_directory");
    fs::create_symlink("symlink_sandbox/", fs::absolute(startingPoint));

    std::vector<std::string> paths;
    paths.emplace_back(fs::absolute(startingPoint));
    std::vector<std::string> exclusions;
    exclusions.emplace_back(fs::absolute("symlink_sandbox/"));
    Options options(false, paths, exclusions, true, false, false, false);

    avscanner::avscannerimpl::CommandLineScanRunner runner(options);

    auto socket = std::make_shared<RecordingMockSocket>();
    runner.setSocket(socket);
    runner.run();

    EXPECT_TRUE(appenderContains("Skipping the scanning of symlink target (\"" + fs::absolute("symlink_sandbox").string()));
    EXPECT_TRUE(appenderContains("which is excluded by user defined exclusion: /tmp/TestCommandLineScanRunner/scanSymlinkedDirectoryWithAbsoluteTargetExclusion/symlink_sandbox/"));
    ASSERT_EQ(socket->m_paths.size(), 0);
}

TEST_F(TestCommandLineScanRunner, scanSymlinkedDirectoryWithAbsoluteDirectExclusion)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    fs::create_directories("symlink_sandbox/a/b/c");
    std::ofstream("symlink_sandbox/a/b/c/file1.txt").close();

    fs::path startingPoint = fs::path("symlink_to_sandbox_directory");
    fs::create_symlink("symlink_sandbox/", fs::absolute(startingPoint));

    std::vector<std::string> paths;
    paths.emplace_back(fs::absolute(startingPoint));
    std::vector<std::string> exclusions;
    exclusions.emplace_back(fs::absolute(startingPoint.string() + "/"));
    Options options(false, paths, exclusions, true, false, false, false);

    avscanner::avscannerimpl::CommandLineScanRunner runner(options);

    auto socket = std::make_shared<RecordingMockSocket>();
    runner.setSocket(socket);
    runner.run();

    EXPECT_TRUE(appenderContains("Skipping the scanning of symlink target (\"" + fs::canonical(startingPoint).string()));
    EXPECT_TRUE(appenderContains("which is excluded by user defined exclusion: /tmp/TestCommandLineScanRunner/scanSymlinkedDirectoryWithAbsoluteDirectExclusion/symlink_to_sandbox_directory/"));
    ASSERT_EQ(socket->m_paths.size(), 0);
}

TEST_F(TestCommandLineScanRunner, scanSymlinkedDirectoryWithRelativeDirectExclusion)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    fs::create_directories("symlink_sandbox/a/b/c");
    std::ofstream("symlink_sandbox/a/b/c/file1.txt").close();

    fs::path startingPoint = fs::path("symlink_to_sandbox_directory");
    fs::create_symlink("symlink_sandbox/", fs::absolute(startingPoint));

    std::vector<std::string> paths;
    paths.emplace_back(fs::absolute(startingPoint));
    std::vector<std::string> exclusions;
    exclusions.emplace_back(startingPoint.string() + "/");
    Options options(false, paths, exclusions, true, false, false, false);

    avscanner::avscannerimpl::CommandLineScanRunner runner(options);

    auto originalWD = fs::current_path();
    fs::current_path("../");
    auto socket = std::make_shared<RecordingMockSocket>();
    runner.setSocket(socket);
    runner.run();
    fs::current_path(originalWD);

    EXPECT_TRUE(appenderContains("Skipping the scanning of symlink target (\"" + fs::canonical(startingPoint).string()));
    EXPECT_TRUE(appenderContains("which is excluded by user defined exclusion: symlink_to_sandbox_directory/"));
    ASSERT_EQ(socket->m_paths.size(), 0);
}

TEST_F(TestCommandLineScanRunner, scanRelativeSymlinkedDirectoryWithAbsoluteTargetExclusion)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    fs::create_directories("a/b");
    fs::create_directories("symlink_sandbox");
    std::ofstream("symlink_sandbox/file1.txt").close();

    fs::create_symlink(fs::absolute("symlink_sandbox"), fs::absolute("a/b/symlink_to_sandbox_dir"));

    fs::path startingPoint = fs::path("a/b/../b/symlink_to_sandbox_dir");
    fs::path startingPoint2 = fs::path("a/b/./symlink_to_sandbox_dir");
    fs::path startingPoint3 = fs::path("./a/b/symlink_to_sandbox_dir");
    fs::path startingPoint4 = fs::path("../scanRelativeSymlinkedDirectoryWithAbsoluteTargetExclusion/a/b/symlink_to_sandbox_dir");

    std::vector<std::string> paths;
    paths.emplace_back(fs::absolute(startingPoint));
    paths.emplace_back(fs::absolute(startingPoint2));
    paths.emplace_back(fs::absolute(startingPoint3));
    paths.emplace_back(fs::absolute(startingPoint4));
    std::vector<std::string> exclusions;
    exclusions.emplace_back(fs::absolute("a/b/symlink_to_sandbox_dir/"));
    Options options(false, paths, exclusions, true, false, false, false);
    avscanner::avscannerimpl::CommandLineScanRunner runner(options);

    auto socket = std::make_shared<RecordingMockSocket>();
    runner.setSocket(socket);
    runner.run();

    EXPECT_TRUE(appenderContains("Skipping the scanning of symlink target (\"" + fs::absolute("symlink_sandbox").string()));
    EXPECT_TRUE(appenderContains("which is excluded by user defined exclusion: " + fs::absolute("a/b/symlink_to_sandbox_dir/").string()));

    EXPECT_FALSE(appenderContains("/."));

    ASSERT_EQ(socket->m_paths.size(), 0);
}

TEST_F(TestCommandLineScanRunner, scanSymlinkedDirectoryWithFileExclusion)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    fs::create_directories("a/b");
    fs::create_directories("symlink_sandbox");
    std::ofstream("symlink_sandbox/file1.txt").close();

    fs::path startingPoint = fs::path("a/b/symlink_to_sandbox_dir");
    fs::create_symlink(fs::absolute("symlink_sandbox"), fs::absolute(startingPoint));

    std::vector<std::string> paths;
    paths.emplace_back(fs::absolute("a/b/symlink_to_sandbox_dir"));
    std::vector<std::string> exclusions;
    // File exclusion shouldn't block this scan
    exclusions.emplace_back(fs::absolute("a/b/symlink_to_sandbox_dir"));
    Options options(false, paths, exclusions, true, false, false, false);
    avscanner::avscannerimpl::CommandLineScanRunner runner(options);

    auto socket = std::make_shared<RecordingMockSocket>();
    runner.setSocket(socket);
    runner.run();

    EXPECT_FALSE(appenderContains("Skipping the scanning of symlink target (\"" + fs::absolute("symlink_sandbox").string()));
    EXPECT_FALSE(appenderContains("which is excluded by user defined exclusion: " + fs::absolute("a/b/symlink_to_sandbox_dir").string()));
    ASSERT_EQ(socket->m_paths.size(), 1);
}

TEST_F(TestCommandLineScanRunner, noSymlinkIsScannedWhenNotExplicitlyCalled)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    fs::create_directories("symlink_sandbox");
    std::ofstream("symlink_sandbox/file1.txt").close();

    fs::path startingPoint = fs::path("symlink_to_sandbox_file");
    fs::create_symlink("symlink_sandbox/file1.txt", startingPoint);

    std::vector<std::string> paths;
    paths.emplace_back(fs::current_path());
    std::vector<std::string> exclusions;
    exclusions.emplace_back("");
    Options options(false, paths, exclusions, true, false, false, false);

    avscanner::avscannerimpl::CommandLineScanRunner runner(options);

    auto socket = std::make_shared<RecordingMockSocket>();
    runner.setSocket(socket);
    runner.run();

    ASSERT_FALSE(appenderContains("Scanning " + fs::absolute(startingPoint).string()));
    ASSERT_EQ(socket->m_paths.size(), 1);
}

TEST_F(TestCommandLineScanRunner, excludeNamedFolders)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    fs::create_directories("sandbox/a/b/d/e");
    fs::create_directories("sandbox/a/f");
    std::ofstream("sandbox/a/b/file1.txt").close();
    std::ofstream("sandbox/a/f/file2.txt").close();

    std::vector<std::string> paths;
    paths.emplace_back(fs::absolute("sandbox"));
    std::vector<std::string> exclusions;
    exclusions.emplace_back("*/");
    Options options(false, paths, exclusions, true, false, false, false);
    avscanner::avscannerimpl::CommandLineScanRunner runner(options);

    auto socket = std::make_shared<RecordingMockSocket>();
    runner.setSocket(socket);
    EXPECT_NO_THROW( runner.run());

    EXPECT_TRUE(appenderContains("Excluding directory: " + fs::absolute("sandbox/").string()));
    EXPECT_FALSE(appenderContains("Excluding directory: " + fs::absolute("sandbox/a/b/").string()));
    EXPECT_FALSE(appenderContains("Excluding directory: " + fs::absolute("sandbox/a/f/").string()));
    EXPECT_FALSE(appenderContains("Excluding file: " + fs::absolute("sandbox/a/b/file1.txt").string()));
    EXPECT_FALSE(appenderContains("Excluding file: " + fs::absolute("sandbox/a/f/file2.txt").string()));
    EXPECT_FALSE(appenderContains("Scanning " + fs::absolute("sandbox/a/b/file1.txt").string()));
    EXPECT_FALSE(appenderContains("Scanning " + fs::absolute("sandbox/a/f/file2.txt").string()));

    ASSERT_EQ(socket->m_paths.size(), 0);
}

TEST_F(TestCommandLineScanRunner, excludeSpecialMounts)
{
    fs::path startingpoint = fs::absolute("sandbox");

    fs::create_directories("sandbox/a/b/d/e");
    std::ofstream("sandbox/a/b/file1.txt").close();


    std::vector<std::string> paths;
    paths.emplace_back(startingpoint);
    std::vector<std::string> exclusions;

    Options options(false, paths, exclusions, false, false, false, false);
    avscanner::avscannerimpl::CommandLineScanRunner runner(options);

    auto socket = std::make_shared<RecordingMockSocket>();
    runner.setSocket(socket);

    std::shared_ptr<FakeMountInfo> mountInfo;
    mountInfo.reset(new FakeMountInfo());
    mountInfo->m_mountPoints.emplace_back(
            std::make_shared<FakeMountPoint>(startingpoint / "a/b", FakeMountPoint::type::special)
            );
    runner.setMountInfo(mountInfo);

    runner.run();

    fs::remove_all("sandbox");

    EXPECT_EQ(socket->m_paths.size(), 0);
}

TEST_F(TestCommandLineScanRunner, optionsButNoPathProvided)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    std::vector<std::string> emptyPathList;
    std::vector<std::string> exclusionList;
    exclusionList.emplace_back("/proc");
    Options options(false, emptyPathList, exclusionList, true, false, false, false);
    CommandLineScanRunner runner(options);

    EXPECT_EQ(runner.run(), E_GENERIC_FAILURE);
    EXPECT_TRUE(appenderContains("Missing a file path from the command line arguments."));
}

TEST_F(TestCommandLineScanRunner, noPathProvided)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    std::vector<std::string> emptyPathList;
    std::vector<std::string> emptyExclusionList;
    Options options(false, emptyPathList, emptyExclusionList, false, false, false, false);
    CommandLineScanRunner runner(options);

    EXPECT_EQ(runner.run(), E_GENERIC_FAILURE);
    EXPECT_TRUE(appenderContains("Missing a file path from the command line arguments."));
}

TEST_F(TestCommandLineScanRunner, anEmptyPathProvided)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    fs::create_directories("sandbox/a/b/d/e");
    std::ofstream("sandbox/a/b/file1.txt").close();

    fs::path startingpoint = fs::absolute("sandbox");

    std::vector<std::string> paths;
    paths.emplace_back(startingpoint);
    paths.emplace_back("");
    std::vector<std::string> exclusions;
    Options options(false, paths, exclusions, false, false, false, false);
    avscanner::avscannerimpl::CommandLineScanRunner runner(options);

    auto socket = std::make_shared<RecordingMockSocket>();
    runner.setSocket(socket);
    runner.run();

    ASSERT_EQ(socket->m_paths.size(), 1);
    EXPECT_EQ(socket->m_paths.at(0), fs::absolute("sandbox/a/b/file1.txt").string());

    EXPECT_TRUE(appenderContains("Refusing to scan empty path"));
}

TEST_F(TestCommandLineScanRunner, RelativePathDoesntExist)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    fs::create_directories("sandbox/a/b/d/e");
    std::ofstream("sandbox/a/b/file1.txt").close();

    std::vector<std::string> paths;
    fs::path nonexistentRelPath("notsandbox");
    paths.emplace_back(nonexistentRelPath);
    std::vector<std::string> exclusions;
    Options options(false, paths, exclusions, false, false, false, false);
    avscanner::avscannerimpl::CommandLineScanRunner runner(options);

    auto socket = std::make_shared<RecordingMockSocket>();
    runner.setSocket(socket);

    EXPECT_EQ(runner.run(), ENOENT);

    ASSERT_EQ(socket->m_paths.size(), 0);

    std::stringstream expectedErrMsg;
    expectedErrMsg << "Failed to completely scan " << nonexistentRelPath.string() << " due to an error: filesystem error: Failed to scan \"";
    expectedErrMsg << fs::absolute(nonexistentRelPath).string() << "\": file/folder does not exist: No such file or directory";
    EXPECT_TRUE(appenderContains(expectedErrMsg.str()));
    EXPECT_TRUE(appenderContains("Failed to scan one or more files due to an error"));
}

TEST_F(TestCommandLineScanRunner, AbsolutePathDoesntExist)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    fs::create_directories("sandbox/a/b/d/e");
    std::ofstream("sandbox/a/b/file1.txt").close();

    std::vector<std::string> paths;
    fs::path nonexistentAbsPath = fs::absolute("notsandbox");
    paths.emplace_back(nonexistentAbsPath);
    std::vector<std::string> exclusions;
    Options options(false, paths, exclusions, false, false, false, false);
    avscanner::avscannerimpl::CommandLineScanRunner runner(options);

    auto socket = std::make_shared<RecordingMockSocket>();
    runner.setSocket(socket);

    EXPECT_EQ(runner.run(), ENOENT);

    ASSERT_EQ(socket->m_paths.size(), 0);

    std::stringstream expectedErrMsg;
    expectedErrMsg << "Failed to completely scan " << nonexistentAbsPath.string() << " due to an error: filesystem error: Failed to scan \"";
    expectedErrMsg << nonexistentAbsPath.string() << "\": file/folder does not exist: No such file or directory";
    EXPECT_TRUE(appenderContains(expectedErrMsg.str()));
    EXPECT_TRUE(appenderContains("Failed to scan one or more files due to an error"));
}

TEST_F(TestCommandLineScanRunner, TestMissingPath)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    fs::path testDir1 = m_testDir / "scan/dir";
    fs::create_directories(testDir1);

    fs::path testfile1 = testDir1 / "file.txt";
    std::ofstream testfileStream1(testfile1.string());
    testfileStream1 << "scan this file";
    testfileStream1.close();

    fs::path testDir2 = m_testDir / "missing/dir";

    fs::path testDir3 = m_testDir / "scan/dir3";
    fs::create_directories(testDir3);

    fs::path testfile3 = testDir3 / "file.txt";
    std::ofstream testfileStream3(testfile3.string());
    testfileStream3 << "scan this file";
    testfileStream3.close();

    std::vector<std::string> paths = {
        testDir1,
        testDir2,
        testDir3
    };
    std::vector<std::string> exclusions;
    Options options(false, paths, exclusions, false, false, false, false);
    avscanner::avscannerimpl::CommandLineScanRunner runner(options);

    auto socket = std::make_shared<RecordingMockSocket>(false);
    runner.setSocket(socket);

    EXPECT_EQ(runner.run(), ENOENT);

    std::vector<std::string> filePaths = {
        testfile1,
        testfile3
    };
    EXPECT_THAT(socket->m_paths, ContainerEq(filePaths));

    std::stringstream expectedErrMsg;
    expectedErrMsg << "Failed to scan \"" << testDir2.string() << "\": file/folder does not exist";
    EXPECT_TRUE(appenderContains(expectedErrMsg.str()));
    EXPECT_TRUE(appenderContains("1 scan error encountered."));
}