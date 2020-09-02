/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gtest/gtest.h>

#include "RecordingMockSocket.h"
#include "CommandLineScanRunnerMemoryAppenderUsingTests.h"

#include "avscanner/avscannerimpl/BaseFileWalkCallbacks.h"
#include "avscanner/avscannerimpl/CommandLineScanRunner.h"
#include "datatypes/sophos_filesystem.h"


#include <fstream>

using namespace avscanner::avscannerimpl;
namespace fs = sophos_filesystem;

namespace
{
    class TestCommandLineScanRunner : public CommandLineScannerMemoryAppenderUsingTests
    {
    };
}

TEST(CommandLineScanRunner, construction) // NOLINT
{
    std::vector<std::string> paths;
    std::vector<std::string> exclusions;
    Options options(false, paths, exclusions, false);
    CommandLineScanRunner runner(options);
}

TEST(CommandLineScanRunner, constructionWithScanArchives) // NOLINT
{
    std::vector<std::string> paths;
    std::vector<std::string> exclusions;
    Options options(false, paths, exclusions, true);
    CommandLineScanRunner runner(options);
}

TEST(CommandLineScanRunner, scanRelativePath) // NOLINT
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

    fs::remove_all("sandbox");

    ASSERT_EQ(socket->m_paths.size(), 1);
    EXPECT_EQ(socket->m_paths.at(0), fs::absolute("sandbox/a/b/file1.txt").string());
}

TEST(CommandLineScanRunner, scanAbsolutePath) // NOLINT
{
    fs::create_directories("/tmp/sandbox/a/b/d/e");
    std::ofstream("/tmp/sandbox/a/b/file1.txt");

    fs::path startingpoint = fs::absolute("/tmp/sandbox");

    std::vector<std::string> paths;
    paths.emplace_back(startingpoint);
    std::vector<std::string> exclusions;
    Options options(false, paths, exclusions, false);
    avscanner::avscannerimpl::CommandLineScanRunner runner(options);

    auto socket = std::make_shared<RecordingMockSocket>();
    runner.setSocket(socket);
    runner.run();

    fs::remove_all("/tmp/sandbox");

    ASSERT_EQ(socket->m_paths.size(), 1);
    EXPECT_EQ(socket->m_paths.at(0), fs::absolute("/tmp/sandbox/a/b/file1.txt").string());
}

TEST(CommandLineScanRunner, scanRelativeDirectory) // NOLINT
{
    fs::create_directories("sandbox/a/b/d/e");
    std::ofstream("sandbox/a/b/file1.txt");

    std::vector<std::string> paths;
    paths.emplace_back("sandbox");
    std::vector<std::string> exclusions;
    Options options(false, paths, exclusions, false);
    avscanner::avscannerimpl::CommandLineScanRunner runner(options);

    auto socket = std::make_shared<RecordingMockSocket>();
    runner.setSocket(socket);
    runner.run();

    fs::remove_all("sandbox");

    ASSERT_EQ(socket->m_paths.size(), 1);
    EXPECT_EQ(socket->m_paths.at(0), fs::absolute("sandbox/a/b/file1.txt").string());
}

TEST(CommandLineScanRunner, scanAbsoluteDirectory) // NOLINT
{
    fs::create_directories("/tmp/sandbox/a/b/d/e");
    std::ofstream("/tmp/sandbox/a/b/file1.txt");

    std::vector<std::string> paths;
    paths.emplace_back("/tmp/sandbox");
    std::vector<std::string> exclusions;
    Options options(false, paths, exclusions, false);
    avscanner::avscannerimpl::CommandLineScanRunner runner(options);

    auto socket = std::make_shared<RecordingMockSocket>();
    runner.setSocket(socket);
    runner.run();

    fs::remove_all("/tmp/sandbox");

    ASSERT_EQ(socket->m_paths.size(), 1);
    EXPECT_EQ(socket->m_paths.at(0), fs::absolute("/tmp/sandbox/a/b/file1.txt").string());
}

TEST(CommandLineScanRunner, scanRelativeDirectoryWithScanArchives) // NOLINT
{
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

    fs::remove_all("sandbox");

    ASSERT_EQ(socket->m_paths.size(), 1);
    EXPECT_EQ(socket->m_paths.at(0), fs::absolute("sandbox/a/b/file1.txt").string());
}

TEST(CommandLineScanRunner, scanAbsoluteDirectoryWithFilenameExclusion) // NOLINT
{
    fs::create_directories("/tmp/sandbox/a/b/d/e");
    std::ofstream("/tmp/sandbox/a/b/file1.txt");
    std::ofstream("/tmp/sandbox/a/b/file2.txt");

    std::vector<std::string> paths;
    paths.emplace_back("/tmp/sandbox");
    std::vector<std::string> exclusions;
    exclusions.push_back("file1.txt");
    Options options(false, paths, exclusions, true);
    avscanner::avscannerimpl::CommandLineScanRunner runner(options);

    auto socket = std::make_shared<RecordingMockSocket>();
    runner.setSocket(socket);
    runner.run();

    fs::remove_all("/tmp/sandbox");

    ASSERT_EQ(socket->m_paths.size(), 1);
    EXPECT_EQ(socket->m_paths.at(0), "/tmp/sandbox/a/b/file2.txt");
}

TEST_F(TestCommandLineScanRunner, exclusionIsFileToScan) // NOLINT
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    fs::create_directories("/tmp/sandbox/a/b/d/e");
    std::ofstream("/tmp/sandbox/a/b/file1.txt");

    std::vector<std::string> paths;
    paths.emplace_back("/tmp/sandbox/a/b/file1.txt");
    std::vector<std::string> exclusions;
    exclusions.push_back("/tmp/sandbox/a/b/file1.txt");
    Options options(false, paths, exclusions, true);
    avscanner::avscannerimpl::CommandLineScanRunner runner(options);

    auto socket = std::make_shared<RecordingMockSocket>();
    runner.setSocket(socket);
    runner.run();

    fs::remove_all("/tmp/sandbox");

    ASSERT_TRUE(appenderContains("Excluding file: \"/tmp/sandbox/a/b/file1.txt\""));
    ASSERT_EQ(socket->m_paths.size(), 0);
}

TEST_F(TestCommandLineScanRunner, exclusionIsDirectoryToScan) // NOLINT
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    fs::create_directories("/tmp/sandbox/a/b/d/e");
    std::ofstream("/tmp/sandbox/a/b/file1.txt");
    std::ofstream("/tmp/sandbox/a/b/d/file2.txt");

    std::vector<std::string> paths;
    paths.emplace_back("/tmp/sandbox/a/b/");
    std::vector<std::string> exclusions;
    exclusions.push_back("/tmp/sandbox/a/b/");
    Options options(false, paths, exclusions, true);
    avscanner::avscannerimpl::CommandLineScanRunner runner(options);

    auto socket = std::make_shared<RecordingMockSocket>();
    runner.setSocket(socket);
    runner.run();

    fs::remove_all("/tmp/sandbox");

    ASSERT_TRUE(appenderContains("Excluding folder: \"/tmp/sandbox/a/b/\""));
    ASSERT_FALSE(appenderContains("Excluding file: /tmp/sandbox/a/b/file1.txt"));
    ASSERT_FALSE(appenderContains("Excluding file: \"/tmp/sandbox/a/b/d/file2.txt\""));

    ASSERT_EQ(socket->m_paths.size(), 0);
}

TEST(CommandLineScanRunner, scanAbsoluteDirectoryWithStemExclusion) // NOLINT
{
    fs::create_directories("/tmp/sandbox/a/b/d/e");
    fs::create_directories("/tmp/sandbox/a/f");
    std::ofstream("/tmp/sandbox/a/b/file1.txt");
    std::ofstream("/tmp/sandbox/a/f/file2.txt");

    std::vector<std::string> paths;
    paths.emplace_back("/tmp/sandbox");
    std::vector<std::string> exclusions;
    exclusions.push_back("/tmp/sandbox/a/b/");
    Options options(false, paths, exclusions, true);
    avscanner::avscannerimpl::CommandLineScanRunner runner(options);

    auto socket = std::make_shared<RecordingMockSocket>();
    runner.setSocket(socket);
    runner.run();

    fs::remove_all("/tmp/sandbox");

    ASSERT_EQ(socket->m_paths.size(), 1);
    EXPECT_EQ(socket->m_paths.at(0), "/tmp/sandbox/a/f/file2.txt");
}

TEST(CommandLineScanRunner, scanAbsoluteDirectoryWithFullPathExclusion) // NOLINT
{
    fs::create_directories("/tmp/sandbox/a/b/d/e");
    std::ofstream("/tmp/sandbox/a/b/file1.txt");
    std::ofstream("/tmp/sandbox/a/b/file2.txt");

    std::vector<std::string> paths;
    paths.emplace_back("/tmp/sandbox");
    std::vector<std::string> exclusions;
    exclusions.push_back("/tmp/sandbox/a/b/file2.txt");
    Options options(false, paths, exclusions, true);
    avscanner::avscannerimpl::CommandLineScanRunner runner(options);

    auto socket = std::make_shared<RecordingMockSocket>();
    runner.setSocket(socket);
    runner.run();

    fs::remove_all("/tmp/sandbox");

    ASSERT_EQ(socket->m_paths.size(), 1);
    EXPECT_EQ(socket->m_paths.at(0), "/tmp/sandbox/a/b/file1.txt");
}

TEST(CommandLineScanRunner, scanAbsoluteDirectoryWithGlobExclusion) // NOLINT
{
    fs::create_directories("/tmp/sandbox/a/b/d/e");
    fs::create_directories("/tmp/sandbox/a/f");
    std::ofstream("/tmp/sandbox/a/b/file1.txt");
    std::ofstream("/tmp/sandbox/a/f/file2.txt");

    std::vector<std::string> paths;
    paths.emplace_back("/tmp/sandbox");
    std::vector<std::string> exclusions;
    exclusions.push_back("sandbox/a/b/");
    Options options(false, paths, exclusions, true);
    avscanner::avscannerimpl::CommandLineScanRunner runner(options);

    auto socket = std::make_shared<RecordingMockSocket>();
    runner.setSocket(socket);
    runner.run();

    fs::remove_all("/tmp/sandbox");

    ASSERT_EQ(socket->m_paths.size(), 1);
    EXPECT_EQ(socket->m_paths.at(0), "/tmp/sandbox/a/f/file2.txt");
}

TEST(CommandLineScanRunner, nonCanonicalExclusions) // NOLINT
{
    fs::create_directories("/tmp/sandbox/a/b/d/e");
    fs::create_directories("/tmp/sandbox/a/f");
    std::ofstream("/tmp/sandbox/a/b/file1.txt");
    std::ofstream("/tmp/sandbox/a/f/file2.txt");

    std::vector<std::string> paths;
    paths.emplace_back("/tmp/sandbox");
    std::vector<std::string> exclusions;
    exclusions.push_back("/tmp/sandbox/./a/f/");
    exclusions.push_back("/tmp/sandbox/../sandbox/a/b/");
    Options options(false, paths, exclusions, true);
    avscanner::avscannerimpl::CommandLineScanRunner runner(options);

    auto socket = std::make_shared<RecordingMockSocket>();
    runner.setSocket(socket);
    runner.run();

    fs::remove_all("/tmp/sandbox");

    ASSERT_EQ(socket->m_paths.size(), 0);
}

TEST(CommandLineScanRunner, nonCanonicalExclusionsWithFilename) // NOLINT
{
    fs::create_directories("/tmp/sandbox/a/b/d/e");
    fs::create_directories("/tmp/sandbox/a/f");
    std::ofstream("/tmp/sandbox/a/b/file1.txt");
    std::ofstream("/tmp/sandbox/a/f/file2.txt");

    std::vector<std::string> paths;
    paths.emplace_back("/tmp/sandbox");
    std::vector<std::string> exclusions;
    exclusions.push_back("/tmp/sandbox/./a/f/file2.txt");
    exclusions.push_back("/tmp/sandbox/../sandbox/a/b/file1.txt");
    Options options(false, paths, exclusions, true);
    avscanner::avscannerimpl::CommandLineScanRunner runner(options);

    auto socket = std::make_shared<RecordingMockSocket>();
    runner.setSocket(socket);
    runner.run();

    fs::remove_all("/tmp/sandbox");

    ASSERT_EQ(socket->m_paths.size(), 0);
}

TEST(CommandLineScanRunner, excludeSpecialMounts) // NOLINT
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

TEST(CommandLineScanRunner, optionsButNoPathProvided) // NOLINT
{
    std::vector<std::string> emptyPathList;
    std::vector<std::string> exclusionList;
    exclusionList.push_back("/proc");
    Options options(false, emptyPathList, exclusionList, true);
    CommandLineScanRunner runner(options);

    EXPECT_EQ(runner.run(), E_GENERIC_FAILURE);
}

TEST(CommandLineScanRunner, noPathProvided) // NOLINT
{
    std::vector<std::string> emptyPathList;
    std::vector<std::string> emptyExclusionList;
    Options options(false, emptyPathList, emptyExclusionList, false);
    CommandLineScanRunner runner(options);

    EXPECT_EQ(runner.run(), E_GENERIC_FAILURE);
}
