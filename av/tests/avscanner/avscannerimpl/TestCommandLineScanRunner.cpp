/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gtest/gtest.h>

#include "RecordingMockSocket.h"
#include "ScanRunnerMemoryAppenderUsingTests.h"

#include "avscanner/avscannerimpl/BaseFileWalkCallbacks.h"
#include "avscanner/avscannerimpl/CommandLineScanRunner.h"
#include "datatypes/sophos_filesystem.h"


#include <fstream>

using namespace avscanner::avscannerimpl;
namespace fs = sophos_filesystem;

namespace
{
    class TestCommandLineScanRunner : public ScanRunnerMemoryAppenderUsingTests
    {
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
}

TEST_F(TestCommandLineScanRunner, construction) // NOLINT
{
    std::vector<std::string> paths;
    std::vector<std::string> exclusions;
    Options options(false, paths, exclusions, false);
    CommandLineScanRunner runner(options);
}

TEST_F(TestCommandLineScanRunner, constructionWithScanArchives) // NOLINT
{
    std::vector<std::string> paths;
    std::vector<std::string> exclusions;
    Options options(false, paths, exclusions, true);
    CommandLineScanRunner runner(options);
}

TEST_F(TestCommandLineScanRunner, scanRelativePath) // NOLINT
{
    fs::create_directories("sandbox/a/b/d/e");
    std::ofstream("sandbox/a/b/file1.txt");

    fs::path startingpoint = fs::absolute("sandbox");

    std::vector<std::string> paths;
    paths.emplace_back(startingpoint);
    std::vector<std::string> exclusions;
    Options options(false, paths, exclusions, false);
    CommandLineScanRunner runner(options);

    auto socket = std::make_shared<RecordingMockSocket>();
    runner.setSocket(socket);
    runner.run();

    ASSERT_EQ(socket->m_paths.size(), 1);
    EXPECT_EQ(socket->m_paths.at(0), fs::absolute("sandbox/a/b/file1.txt").string());
}

TEST_F(TestCommandLineScanRunner, scanNonCanonicalPath) // NOLINT
{
    fs::create_directories("sandbox/a/b/d/e");
    std::ofstream("sandbox/a/b/file1.txt");

    std::vector<std::string> paths;
    paths.emplace_back(fs::absolute("./sandbox/"));
    paths.emplace_back(fs::absolute( "sandbox/../sandbox/"));
    paths.emplace_back(fs::absolute( "sandbox/a/.."));
    std::vector<std::string> exclusions;
    Options options(false, paths, exclusions, false);
    CommandLineScanRunner runner(options);

    auto socket = std::make_shared<RecordingMockSocket>();
    runner.setSocket(socket);
    runner.run();

    ASSERT_EQ(socket->m_paths.size(), 3);
    EXPECT_EQ(socket->m_paths.at(0), fs::absolute("sandbox/a/b/file1.txt").string());
    EXPECT_EQ(socket->m_paths.at(1), fs::absolute("sandbox/a/b/file1.txt").string());
    EXPECT_EQ(socket->m_paths.at(2), fs::absolute("sandbox/a/b/file1.txt").string());
}

TEST_F(TestCommandLineScanRunner, scanAbsolutePath) // NOLINT
{
    fs::create_directories("sandbox/a/b/d/e");
    std::ofstream("sandbox/a/b/file1.txt");

    fs::path startingpoint = fs::absolute("sandbox");

    std::vector<std::string> paths;
    paths.emplace_back(startingpoint);
    std::vector<std::string> exclusions;
    Options options(false, paths, exclusions, false);
    avscanner::avscannerimpl::CommandLineScanRunner runner(options);

    auto socket = std::make_shared<RecordingMockSocket>();
    runner.setSocket(socket);
    runner.run();

    ASSERT_EQ(socket->m_paths.size(), 1);
    EXPECT_EQ(socket->m_paths.at(0), fs::absolute("sandbox/a/b/file1.txt").string());
}

TEST_F(TestCommandLineScanRunner, scanRelativeDirectory) // NOLINT
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    fs::create_directories("sandbox/a/b/d/e");
    std::ofstream("sandbox/a/b/file1.txt");

    std::vector<std::string> paths;
    paths.emplace_back("sandbox/");
    std::vector<std::string> exclusions;
    Options options(false, paths, exclusions, false);
    avscanner::avscannerimpl::CommandLineScanRunner runner(options);

    auto socket = std::make_shared<RecordingMockSocket>();
    runner.setSocket(socket);
    runner.run();

    EXPECT_TRUE(appenderContains("Archive scanning enabled: no"));

    ASSERT_EQ(socket->m_paths.size(), 1);
    EXPECT_EQ(socket->m_paths.at(0), fs::absolute("sandbox/a/b/file1.txt").string());
}

TEST_F(TestCommandLineScanRunner, scanAbsoluteDirectory) // NOLINT
{
    fs::create_directories("sandbox/a/b/d/e");
    std::ofstream("sandbox/a/b/file1.txt");

    std::vector<std::string> paths;
    paths.emplace_back(fs::absolute("sandbox/"));
    std::vector<std::string> exclusions;
    Options options(false, paths, exclusions, false);
    avscanner::avscannerimpl::CommandLineScanRunner runner(options);

    auto socket = std::make_shared<RecordingMockSocket>();
    runner.setSocket(socket);
    runner.run();

    ASSERT_EQ(socket->m_paths.size(), 1);
    EXPECT_EQ(socket->m_paths.at(0), fs::absolute("sandbox/a/b/file1.txt").string());
}

TEST_F(TestCommandLineScanRunner, scanRelativeDirectoryWithScanArchives) // NOLINT
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    fs::create_directories("sandbox/a/b/d/e");
    std::ofstream("sandbox/a/b/file1.txt");

    std::vector<std::string> paths;
    paths.emplace_back("sandbox");
    std::vector<std::string> exclusions;
    Options options(false, paths, exclusions, true);
    avscanner::avscannerimpl::CommandLineScanRunner runner(options);

    auto socket = std::make_shared<RecordingMockSocket>();
    runner.setSocket(socket);
    runner.run();

    EXPECT_TRUE(appenderContains("Archive scanning enabled: yes"));

    ASSERT_EQ(socket->m_paths.size(), 1);
    EXPECT_EQ(socket->m_paths.at(0), fs::absolute("sandbox/a/b/file1.txt").string());
}

TEST_F(TestCommandLineScanRunner, scanAbsoluteDirectoryWithFilenameExclusion) // NOLINT
{
    fs::create_directories("sandbox/a/b/d/e");
    std::ofstream("sandbox/a/b/file1.txt");
    std::ofstream("sandbox/a/b/file2.txt");

    std::vector<std::string> paths;
    paths.emplace_back(fs::absolute("sandbox"));
    std::vector<std::string> exclusions;
    exclusions.emplace_back("file1.txt");
    Options options(false, paths, exclusions, true);
    avscanner::avscannerimpl::CommandLineScanRunner runner(options);

    auto socket = std::make_shared<RecordingMockSocket>();
    runner.setSocket(socket);
    runner.run();

    ASSERT_EQ(socket->m_paths.size(), 1);
    EXPECT_EQ(socket->m_paths.at(0), fs::absolute("sandbox/a/b/file2.txt"));
}

TEST_F(TestCommandLineScanRunner, exclusionIsFileToScan) // NOLINT
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    fs::create_directories("sandbox/a/b/d/e");
    std::ofstream("sandbox/a/b/file1.txt");

    std::vector<std::string> paths;
    paths.emplace_back(fs::absolute("sandbox/a/b/file1.txt"));
    std::vector<std::string> exclusions;
    exclusions.emplace_back(fs::absolute("sandbox/a/b/file1.txt"));
    Options options(false, paths, exclusions, true);
    avscanner::avscannerimpl::CommandLineScanRunner runner(options);

    auto socket = std::make_shared<RecordingMockSocket>();
    runner.setSocket(socket);
    runner.run();

    EXPECT_TRUE(appenderContains("Excluding file: " + fs::absolute("sandbox/a/b/file1.txt").string()));
    ASSERT_EQ(socket->m_paths.size(), 0);
}

TEST_F(TestCommandLineScanRunner, anEmptyExclusionProvided) // NOLINT
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    fs::create_directories("sandbox/a/b/d/e");
    std::ofstream("sandbox/a/b/file1.txt");

    std::vector<std::string> paths;
    paths.emplace_back(fs::absolute("sandbox/a/b/file1.txt"));
    std::vector<std::string> exclusions;
    exclusions.emplace_back(fs::absolute("sandbox/a/b/file1.txt"));
    exclusions.emplace_back("");
    Options options(false, paths, exclusions, true);
    avscanner::avscannerimpl::CommandLineScanRunner runner(options);

    auto socket = std::make_shared<RecordingMockSocket>();
    runner.setSocket(socket);
    runner.run();

    EXPECT_TRUE(appenderContains("Excluding file: " + fs::absolute("sandbox/a/b/file1.txt").string()));
    EXPECT_TRUE(appenderContains("Refusing to exclude empty path"));
    ASSERT_EQ(socket->m_paths.size(), 0);
}

TEST_F(TestCommandLineScanRunner, exclusionIsDirectoryToScan) // NOLINT
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    fs::create_directories("sandbox/a/b/d/e");
    std::ofstream("sandbox/a/b/file1.txt");
    std::ofstream("sandbox/a/b/d/file2.txt");

    std::vector<std::string> paths;
    paths.emplace_back(fs::absolute("sandbox/a/b/"));
    std::vector<std::string> exclusions;
    exclusions.emplace_back(fs::absolute("sandbox/a/b/"));
    Options options(false, paths, exclusions, true);
    avscanner::avscannerimpl::CommandLineScanRunner runner(options);

    auto socket = std::make_shared<RecordingMockSocket>();
    runner.setSocket(socket);
    runner.run();

    EXPECT_TRUE(appenderContains("Excluding directory: " + fs::absolute("sandbox/a/b/").string()));
    EXPECT_FALSE(appenderContains("Excluding file: " + fs::absolute("sandbox/a/b/file1.txt").string()));
    EXPECT_FALSE(appenderContains("Excluding file: " + fs::absolute("sandbox/a/b/d/file2.txt").string()));

    ASSERT_EQ(socket->m_paths.size(), 0);
}

TEST_F(TestCommandLineScanRunner, scanDirectoryExcludedAsFilePath) // NOLINT
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    fs::create_directories("sandbox/a/b/d/e");
    std::ofstream("sandbox/a/b/file1.txt");

    std::vector<std::string> paths;
    paths.emplace_back("sandbox");
    std::vector<std::string> exclusions;
    exclusions.emplace_back(fs::absolute("sandbox"));
    exclusions.emplace_back("sandbox");
    Options options(false, paths, exclusions, false);
    avscanner::avscannerimpl::CommandLineScanRunner runner(options);

    auto socket = std::make_shared<RecordingMockSocket>();
    runner.setSocket(socket);
    runner.run();

    EXPECT_TRUE(appenderContains("Archive scanning enabled: no"));

    ASSERT_EQ(socket->m_paths.size(), 1);
    EXPECT_EQ(socket->m_paths.at(0), fs::absolute("sandbox/a/b/file1.txt").string());
}

TEST_F(TestCommandLineScanRunner, scanFileExcludedAsDirectoryPath) // NOLINT
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    fs::create_directories("sandbox/a/b/d/e");
    std::ofstream("sandbox/a/b/file1.txt");

    std::vector<std::string> paths;
    paths.emplace_back("sandbox/a/b/file1.txt");
    std::vector<std::string> exclusions;
    exclusions.emplace_back(fs::absolute("sandbox/a/b/file1.txt/"));
    exclusions.emplace_back("sandbox/a/b/file1.txt/");
    Options options(false, paths, exclusions, false);
    avscanner::avscannerimpl::CommandLineScanRunner runner(options);

    auto socket = std::make_shared<RecordingMockSocket>();
    runner.setSocket(socket);
    runner.run();

    EXPECT_TRUE(appenderContains("Archive scanning enabled: no"));

    ASSERT_EQ(socket->m_paths.size(), 1);
    EXPECT_EQ(socket->m_paths.at(0), fs::absolute("sandbox/a/b/file1.txt").string());
}

TEST_F(TestCommandLineScanRunner, scanAbsoluteDirectoryWithStemExclusion) // NOLINT
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    fs::create_directories("sandbox/a/b/d/e");
    fs::create_directories("sandbox/a/f");
    std::ofstream("sandbox/a/b/file1.txt");
    std::ofstream("sandbox/a/f/file2.txt");

    std::vector<std::string> paths;
    paths.emplace_back(fs::absolute("sandbox"));
    std::vector<std::string> exclusions;
    exclusions.emplace_back(fs::absolute("sandbox/a/b/"));
    Options options(false, paths, exclusions, true);
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

TEST_F(TestCommandLineScanRunner, scanAbsoluteDirectoryWithFullPathExclusion) // NOLINT
{
    fs::create_directories("sandbox/a/b/d/e");
    std::ofstream("sandbox/a/b/file1.txt");
    std::ofstream("sandbox/a/b/file2.txt");

    std::vector<std::string> paths;
    paths.emplace_back(fs::absolute("sandbox"));
    std::vector<std::string> exclusions;
    exclusions.emplace_back(fs::absolute("sandbox/a/b/file2.txt"));
    Options options(false, paths, exclusions, true);
    avscanner::avscannerimpl::CommandLineScanRunner runner(options);

    auto socket = std::make_shared<RecordingMockSocket>();
    runner.setSocket(socket);
    runner.run();

    ASSERT_EQ(socket->m_paths.size(), 1);
    EXPECT_EQ(socket->m_paths.at(0), fs::absolute("sandbox/a/b/file1.txt"));
}

TEST_F(TestCommandLineScanRunner, scanAbsoluteDirectoryWithGlobExclusion) // NOLINT
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    fs::create_directories("sandbox/a/b/d/e");
    fs::create_directories("sandbox/a/f");
    std::ofstream("sandbox/a/b/file1.txt");
    std::ofstream("sandbox/a/f/file2.txt");

    std::vector<std::string> paths;
    paths.emplace_back(fs::absolute("sandbox"));
    std::vector<std::string> exclusions;
    exclusions.emplace_back("sandbox/a/b/");
    Options options(false, paths, exclusions, true);
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

TEST_F(TestCommandLineScanRunner, nonCanonicalExclusions) // NOLINT
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    fs::create_directories("sandbox/a/b/d/e");
    fs::create_directories("sandbox/a/f");
    fs::create_directories("sandbox/a/g");
    std::ofstream("sandbox/a/b/file1.txt");
    std::ofstream("sandbox/a/f/file2.txt");

    std::vector<std::string> paths;
    paths.emplace_back(fs::absolute("sandbox"));
    std::vector<std::string> exclusions;
    exclusions.emplace_back(fs::absolute("sandbox/./a/f/"));
    exclusions.emplace_back(fs::absolute("sandbox/a/g/."));
    exclusions.emplace_back(fs::absolute("sandbox/../sandbox/a/b/"));
    Options options(false, paths, exclusions, true);
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

TEST_F(TestCommandLineScanRunner, nonCanonicalNonExistentExclusions) // NOLINT
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    fs::create_directories("sandbox/a/b/d/e");
    fs::create_directories("sandbox/a/f");
    std::ofstream("sandbox/a/b/file1.txt");
    std::ofstream("sandbox/a/f/file2.txt");

    std::vector<std::string> paths;
    paths.emplace_back(fs::absolute("sandbox"));
    std::vector<std::string> exclusions;
    exclusions.emplace_back(fs::absolute("sandbox/./a/f/"));
    exclusions.emplace_back(fs::absolute("does_not_exist/./a/f/"));
    exclusions.emplace_back(fs::absolute("sandbox/a/f/."));
    exclusions.emplace_back(fs::absolute(".does_not_exist/a/f/"));
    exclusions.emplace_back(fs::absolute("sandbox/../sandbox/a/b/"));
    Options options(false, paths, exclusions, true);
    avscanner::avscannerimpl::CommandLineScanRunner runner(options);

    auto socket = std::make_shared<RecordingMockSocket>();
    runner.setSocket(socket);
    runner.run();

    std::string expected = "Exclusions: ";
    expected += fs::absolute("sandbox/a/f/").string();
    expected += ", " + fs::absolute("does_not_exist/./a/f/").string();
    expected += ", " + fs::absolute("sandbox/a/f/").string();
    expected += ", " + fs::absolute(".does_not_exist/a/f/").string();
    expected += ", " + fs::absolute("sandbox/a/b/").string();
    EXPECT_TRUE(appenderContains(expected)) << "Did not contain: \"" << expected << "\"";

    EXPECT_TRUE(appenderContains("Excluding directory: " + fs::absolute("sandbox/a/b/").string()));
    EXPECT_TRUE(appenderContains("Cannot canonicalize: " + fs::absolute("does_not_exist/./a/f/").string()));
    EXPECT_TRUE(appenderContains("Excluding directory: " + fs::absolute("sandbox/a/b/").string()));
    EXPECT_TRUE(appenderContains("Cannot canonicalize: " + fs::absolute(".does_not_exist/a/f/").string()));
    EXPECT_TRUE(appenderContains("Excluding directory: " + fs::absolute("sandbox/a/f/").string()));
    EXPECT_FALSE(appenderContains("Scanning " + fs::absolute("sandbox/a/b/file1.txt").string()));
    EXPECT_FALSE(appenderContains("Scanning " + fs::absolute("sandbox/a/f/file2.txt").string()));
    EXPECT_FALSE(appenderContains("Excluding file: " + fs::absolute("sandbox/a/b/file1.txt").string()));
    EXPECT_FALSE(appenderContains("Excluding file: " + fs::absolute("sandbox/a/f/file2.txt").string()));

    ASSERT_EQ(socket->m_paths.size(), 0);
}

TEST_F(TestCommandLineScanRunner, nonCanonicalExclusionsRootExclusion) // NOLINT
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    fs::create_directories("sandbox/a/b/d/e");
    fs::create_directories("sandbox/a/f");
    std::ofstream("sandbox/a/b/file1.txt");
    std::ofstream("sandbox/a/f/file2.txt");

    std::vector<std::string> paths;
    paths.emplace_back(fs::absolute("sandbox"));
    std::vector<std::string> exclusions;
    exclusions.emplace_back("/.");

    Options options(false, paths, exclusions, true);
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

TEST_F(TestCommandLineScanRunner, nonCanonicalExclusionsWithFilename) // NOLINT
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    fs::create_directories("sandbox/a/b/d/e");
    fs::create_directories("sandbox/a/f");
    std::ofstream("sandbox/a/b/file1.txt");
    std::ofstream("sandbox/a/f/file2.txt");

    std::vector<std::string> paths;
    paths.emplace_back(fs::absolute("sandbox"));
    std::vector<std::string> exclusions;
    exclusions.emplace_back(fs::absolute("sandbox/./a/f/file2.txt"));
    exclusions.emplace_back(fs::absolute("sandbox/../sandbox/a/b/file1.txt"));
    Options options(false, paths, exclusions, true);
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

TEST_F(TestCommandLineScanRunner, excludeNamedFolders) // NOLINT
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    fs::create_directories("sandbox/a/b/d/e");
    fs::create_directories("sandbox/a/f");
    std::ofstream("sandbox/a/b/file1.txt");
    std::ofstream("sandbox/a/f/file2.txt");

    std::vector<std::string> paths;
    paths.emplace_back(fs::absolute("sandbox"));
    std::vector<std::string> exclusions;
    exclusions.emplace_back("*/");
    Options options(false, paths, exclusions, true);
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

TEST_F(TestCommandLineScanRunner, excludeSpecialMounts) // NOLINT
{
    fs::path startingpoint = fs::absolute("sandbox");

    using namespace avscanner::avscannerimpl;
    using namespace avscanner::mountinfo;
    class MockMountPoint : public IMountPoint
    {
    public:
        std::string m_mountPoint;

        explicit MockMountPoint(const fs::path& startingpoint)
            : m_mountPoint(startingpoint / "a/b")
        {}

        [[nodiscard]] std::string device() const override
        {
            return std::__cxx11::string();
        }

        [[nodiscard]] std::string filesystemType() const override
        {
            return std::__cxx11::string();
        }

        [[nodiscard]] bool isHardDisc() const override
        {
            return false;
        }

        [[nodiscard]] bool isNetwork() const override
        {
            return false;
        }

        [[nodiscard]] bool isOptical() const override
        {
            return false;
        }

        [[nodiscard]] bool isRemovable() const override
        {
            return false;
        }

        [[nodiscard]] bool isSpecial() const override
        {
            return true;
        }

        [[nodiscard]] std::string mountPoint() const override
        {
            return m_mountPoint;
        }
    };

    class MockMountInfo : public IMountInfo
    {
    public:
        std::vector<std::shared_ptr<IMountPoint>> m_mountPoints;
        std::vector<std::shared_ptr<IMountPoint>> mountPoints() override
        {
            return m_mountPoints;
        }
    };

    fs::create_directories("sandbox/a/b/d/e");
    std::ofstream("sandbox/a/b/file1.txt");


    std::vector<std::string> paths;
    paths.emplace_back(startingpoint);
    std::vector<std::string> exclusions;

    Options options(false, paths, exclusions, false);
    avscanner::avscannerimpl::CommandLineScanRunner runner(options);

    auto socket = std::make_shared<RecordingMockSocket>();
    runner.setSocket(socket);

    std::shared_ptr<MockMountInfo> mountInfo;
    mountInfo.reset(new MockMountInfo());
    mountInfo->m_mountPoints.emplace_back(
            std::make_shared<MockMountPoint>(startingpoint)
            );
    runner.setMountInfo(mountInfo);

    runner.run();

    fs::remove_all("sandbox");

    EXPECT_EQ(socket->m_paths.size(), 0);
}

TEST_F(TestCommandLineScanRunner, optionsButNoPathProvided) // NOLINT
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    std::vector<std::string> emptyPathList;
    std::vector<std::string> exclusionList;
    exclusionList.emplace_back("/proc");
    Options options(false, emptyPathList, exclusionList, true);
    CommandLineScanRunner runner(options);

    EXPECT_EQ(runner.run(), E_GENERIC_FAILURE);
    EXPECT_TRUE(appenderContains("Missing a file path from the command line arguments."));
}

TEST_F(TestCommandLineScanRunner, noPathProvided) // NOLINT
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    std::vector<std::string> emptyPathList;
    std::vector<std::string> emptyExclusionList;
    Options options(false, emptyPathList, emptyExclusionList, false);
    CommandLineScanRunner runner(options);

    EXPECT_EQ(runner.run(), E_GENERIC_FAILURE);
    EXPECT_TRUE(appenderContains("Missing a file path from the command line arguments."));
}

TEST_F(TestCommandLineScanRunner, anEmptyPathProvided) // NOLINT
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    fs::create_directories("sandbox/a/b/d/e");
    std::ofstream("sandbox/a/b/file1.txt");

    fs::path startingpoint = fs::absolute("sandbox");

    std::vector<std::string> paths;
    paths.emplace_back(startingpoint);
    paths.emplace_back("");
    std::vector<std::string> exclusions;
    Options options(false, paths, exclusions, false);
    avscanner::avscannerimpl::CommandLineScanRunner runner(options);

    auto socket = std::make_shared<RecordingMockSocket>();
    runner.setSocket(socket);
    runner.run();

    ASSERT_EQ(socket->m_paths.size(), 1);
    EXPECT_EQ(socket->m_paths.at(0), fs::absolute("sandbox/a/b/file1.txt").string());

    EXPECT_TRUE(appenderContains("Refusing to scan empty path"));
}

TEST_F(TestCommandLineScanRunner, RelativePathDoesntExist) // NOLINT
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    fs::create_directories("sandbox/a/b/d/e");
    std::ofstream("sandbox/a/b/file1.txt");

    std::vector<std::string> paths;
    paths.emplace_back("notsandbox");
    std::vector<std::string> exclusions;
    Options options(false, paths, exclusions, false);
    avscanner::avscannerimpl::CommandLineScanRunner runner(options);

    auto socket = std::make_shared<RecordingMockSocket>();
    runner.setSocket(socket);

    EXPECT_EQ(runner.run(), 2);

    ASSERT_EQ(socket->m_paths.size(), 0);
}

TEST_F(TestCommandLineScanRunner, AbsolutePathDoesntExist) // NOLINT
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    fs::create_directories("sandbox/a/b/d/e");
    std::ofstream("sandbox/a/b/file1.txt");

    std::vector<std::string> paths;
    paths.emplace_back(fs::absolute("notsandbox"));
    std::vector<std::string> exclusions;
    Options options(false, paths, exclusions, false);
    avscanner::avscannerimpl::CommandLineScanRunner runner(options);

    auto socket = std::make_shared<RecordingMockSocket>();
    runner.setSocket(socket);

    EXPECT_EQ(runner.run(), 2);

    ASSERT_EQ(socket->m_paths.size(), 0);
}