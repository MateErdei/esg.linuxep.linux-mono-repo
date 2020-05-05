/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gtest/gtest.h>

#include "RecordingMockSocket.h"

#include "avscanner/avscannerimpl/CommandLineScanRunner.h"
#include "datatypes/sophos_filesystem.h"
#include "datatypes/Print.h"

#include <fstream>

namespace fs = sophos_filesystem;


TEST(CommandLineScanRunner, construction) // NOLINT
{
    std::vector<std::string> paths;
    std::vector<std::string> exclusions;
    avscanner::avscannerimpl::CommandLineScanRunner runner(paths, false, exclusions);
}

TEST(CommandLineScanRunner, constructionWithScanArchives) // NOLINT
{
    std::vector<std::string> paths;
    std::vector<std::string> exclusions;
    avscanner::avscannerimpl::CommandLineScanRunner runner(paths, true, exclusions);
}

TEST(CommandLineScanRunner, scanRelativePath) // NOLINT
{
    fs::create_directories("sandbox/a/b/d/e");
    std::ofstream("sandbox/a/b/file1.txt");

    fs::path startingpoint = fs::absolute("sandbox");

    std::vector<std::string> paths;
    paths.emplace_back(startingpoint);
    std::vector<std::string> exclusions;
    avscanner::avscannerimpl::CommandLineScanRunner runner(paths, false, exclusions);

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
    avscanner::avscannerimpl::CommandLineScanRunner runner(paths, false, exclusions);

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
    avscanner::avscannerimpl::CommandLineScanRunner runner(paths, false, exclusions);

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
    avscanner::avscannerimpl::CommandLineScanRunner runner(paths, false, exclusions);

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
    avscanner::avscannerimpl::CommandLineScanRunner runner(paths, true, exclusions);

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
    avscanner::avscannerimpl::CommandLineScanRunner runner(paths, true, exclusions);

    auto socket = std::make_shared<RecordingMockSocket>();
    runner.setSocket(socket);
    runner.run();

    fs::remove_all("/tmp/sandbox");

    ASSERT_EQ(socket->m_paths.size(), 1);
    EXPECT_EQ(socket->m_paths.at(0), "/tmp/sandbox/a/b/file2.txt");
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
    avscanner::avscannerimpl::CommandLineScanRunner runner(paths, true, exclusions);

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
    avscanner::avscannerimpl::CommandLineScanRunner runner(paths, true, exclusions);

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
    avscanner::avscannerimpl::CommandLineScanRunner runner(paths, true, exclusions);

    auto socket = std::make_shared<RecordingMockSocket>();
    runner.setSocket(socket);
    runner.run();

    fs::remove_all("/tmp/sandbox");

    ASSERT_EQ(socket->m_paths.size(), 1);
    EXPECT_EQ(socket->m_paths.at(0), "/tmp/sandbox/a/f/file2.txt");
}

TEST(CommandLineScanRunner, excludeSpecialMounts) // NOLINT
{
    fs::path startingpoint = fs::absolute("sandbox");

    using namespace avscanner::avscannerimpl;
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
    avscanner::avscannerimpl::CommandLineScanRunner runner(paths, false, exclusions);

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

