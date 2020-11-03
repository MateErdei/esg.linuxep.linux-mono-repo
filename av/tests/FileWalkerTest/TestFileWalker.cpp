/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <filewalker/FileWalker.h>
#include <gtest/gtest.h>
#include <tests/common/LogInitializedTests.h>

#include <cerrno>
#include <fstream>
#include <string>

#include <unistd.h>

/*
 * hack to allow gtest/gmock to interact with mock methods passing fs::path parameters
 *
 * See: https://github.com/google/googletest/issues/521
 *      https://github.com/google/googletest/issues/1614
 *      https://github.com/google/googletest/pull/1186
 *
 * hopefully fixed in googletest v1.10.0
 */
namespace std::experimental::filesystem // NOLINT
{
    void PrintTo(const path& p, std::ostream* os)
    {
        *os << p;
    }
} // namespace std::experimental::filesystem

namespace fs = sophos_filesystem;

using namespace ::testing;

namespace
{
    class MockCallbacks : public filewalker::IFileWalkCallbacks
    {
    public:
        MOCK_METHOD2(processFile, void(const fs::path& filepath, bool symlinkTarget));
        MOCK_METHOD1(includeDirectory, bool(const fs::path& filepath));
        MOCK_METHOD1(userDefinedExclusionCheck, bool(const fs::path& filepath));
    };

    class TestFileWalker : public LogInitializedTests
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
} // namespace

TEST_F(TestFileWalker, includeDirectory) // NOLINT
{
    fs::create_directories("sandbox/a/b/d/e");
    std::ofstream("sandbox/a/b/file1.txt");

    fs::path startingPoint = fs::path("sandbox");

    auto callbacks = std::make_shared<StrictMock<MockCallbacks>>();

    EXPECT_CALL(*callbacks, includeDirectory(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*callbacks, userDefinedExclusionCheck(_)).WillOnce(Return(false));
    EXPECT_CALL(*callbacks, processFile(fs::path("sandbox/a/b/file1.txt"), false)).WillOnce(Return());

    filewalker::walk(startingPoint, *callbacks);
}

TEST_F(TestFileWalker, absoluteIncludePath) // NOLINT
{
    fs::create_directories("sandbox/a/b/d/e");
    std::ofstream("sandbox/a/b/file1.txt");

    fs::path startingPoint = fs::absolute("sandbox");

    auto callbacks = std::make_shared<StrictMock<MockCallbacks>>();

    EXPECT_CALL(*callbacks, includeDirectory(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*callbacks, userDefinedExclusionCheck(_)).WillOnce(Return(false));
    EXPECT_CALL(*callbacks, processFile(fs::absolute("sandbox/a/b/file1.txt"), false)).WillOnce(Return());

    filewalker::walk(startingPoint, *callbacks);
}

TEST_F(TestFileWalker, excludeDirectory) // NOLINT
{
    fs::create_directories("sandbox/a/b/d/e");
    std::ofstream("sandbox/a/b/file1.txt");

    auto callbacks = std::make_shared<StrictMock<MockCallbacks>>();

    EXPECT_CALL(*callbacks, includeDirectory(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*callbacks, userDefinedExclusionCheck(_)).WillOnce(Return(false));
    EXPECT_CALL(*callbacks, includeDirectory(fs::path("sandbox/a/b"))).WillOnce(Return(false));
    EXPECT_CALL(*callbacks, processFile(fs::path("sandbox/a/b/file1.txt"), _)).Times(0);

    filewalker::walk("sandbox", *callbacks);
}

TEST_F(TestFileWalker, scanFileThatDoesNotExist) // NOLINT
{
    auto callbacks = std::make_shared<StrictMock<MockCallbacks>>();

    try
    {
        filewalker::walk("FileThatDoesNotExist", *callbacks);
    }
    catch (fs::filesystem_error& e)
    {
        EXPECT_EQ(
            e.what(),
            std::string("filesystem error: Failed to scan "
                        "\"FileThatDoesNotExist\": file/folder does not exist: No such file or directory"));
        EXPECT_EQ(e.code().value(), ENOENT);
    }
}

TEST_F(TestFileWalker, hugeFilePathStartFromPathRoot) // NOLINT
{
    const fs::path& startingPath = fs::current_path();
    fs::create_directories("TestHugePathFileWalker");
    fs::current_path("TestHugePathFileWalker");

    int depth;
    try
    {
        for (depth = 0; depth < 100; ++depth)
        {
            fs::create_directories("TestYuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuugePathFileWalker");
            fs::current_path("TestYuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuugePathFileWalker");
        }
    }
    catch (fs::filesystem_error& e)
    {
        // depending on starting path, we may not be able to create 100 subdirs
        ASSERT_GT(depth, 90);
    }

    fs::current_path(startingPath);

    auto callbacks = std::make_shared<StrictMock<MockCallbacks>>();

    EXPECT_CALL(*callbacks, includeDirectory(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*callbacks, userDefinedExclusionCheck(_)).WillOnce(Return(false));
    EXPECT_CALL(*callbacks, processFile(_, _)).Times(0);

    EXPECT_NO_THROW(filewalker::walk("TestHugePathFileWalker", *callbacks));

    auto traverse_and_delete_huge_directory = [](const sophos_filesystem::path& startingPath, int targetDirectory) {
        fs::current_path("TestHugePathFileWalker");
        for (int depth = 0; depth < targetDirectory; ++depth)
        {
            fs::current_path("TestYuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuugePathFileWalker");
        }
        fs::remove_all("TestYuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuugePathFileWalker");
        fs::current_path(startingPath);
    };

    traverse_and_delete_huge_directory(startingPath, 70);
    traverse_and_delete_huge_directory(startingPath, 35);
    fs::remove_all("TestHugePathFileWalker");
}

TEST_F(TestFileWalker, hugeStartingFilePath) // NOLINT
{
    const fs::path& startingPath = fs::current_path();
    fs::create_directories("TestHugePathFileWalker");
    fs::current_path("TestHugePathFileWalker");

    int depth;
    try
    {
        for (depth = 0; depth < 100; ++depth)
        {
            fs::create_directories("TestYuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuugePathFileWalker");
            fs::current_path("TestYuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuugePathFileWalker");
        }
    }
    catch (fs::filesystem_error& e)
    {
        // depending on starting path, we may not be able to create 100 subdirs
        ASSERT_GT(depth, 90);
    }

    const fs::path& pathToScan = fs::current_path();
    fs::current_path(startingPath);

    auto callbacks = std::make_shared<StrictMock<MockCallbacks>>();

    EXPECT_CALL(*callbacks, includeDirectory(_)).Times(0);
    EXPECT_CALL(*callbacks, userDefinedExclusionCheck(_)).Times(0);
    EXPECT_CALL(*callbacks, processFile(_, _)).Times(0);

    try
    {
        filewalker::walk(pathToScan, *callbacks);
    }
    catch (fs::filesystem_error& e)
    {
        EXPECT_EQ(
            e.what(),
            std::string("filesystem error: Failed to start scan: Starting Path too long: File name too long"));
        EXPECT_EQ(e.code().value(), ENAMETOOLONG);
    }

    auto traverse_and_delete_huge_directory = [](const sophos_filesystem::path& startingPath, int targetDirectory)
    {
        fs::current_path("TestHugePathFileWalker");
        for (int depth = 0; depth < targetDirectory; ++depth)
        {
            fs::current_path("TestYuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuugePathFileWalker");
        }
        fs::remove_all("TestYuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuugePathFileWalker");
        fs::current_path(startingPath);
    };

    traverse_and_delete_huge_directory(startingPath, 70);
    traverse_and_delete_huge_directory(startingPath, 35);
    fs::remove_all("TestHugePathFileWalker");
}

