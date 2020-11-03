/******************************************************************************************************

Copyright 2019-2020, Sophos Limited.  All rights reserved.

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
#if __cplusplus >= 201703L && __has_include(<filesystem>)
namespace std::filesystem // NOLINT
#else
namespace std::experimental::filesystem // NOLINT
#endif
{
    void PrintTo(const path& p, std::ostream* os)
    {
        *os << p;
    }
}

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

TEST_F(TestFileWalker, includeCurrentDirectory) // NOLINT
{
    fs::create_directories("sandbox/a/b/d/e");
    std::ofstream("sandbox/a/b/file1.txt");

    fs::path startingPoint = fs::path(".");

    auto callbacks = std::make_shared<StrictMock<MockCallbacks>>();

    EXPECT_CALL(*callbacks, includeDirectory(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*callbacks, userDefinedExclusionCheck(_)).WillOnce(Return(false));
    EXPECT_CALL(*callbacks, processFile(fs::path("./sandbox/a/b/file1.txt"), false)).WillOnce(Return());

    filewalker::walk(startingPoint, *callbacks);
}

TEST_F(TestFileWalker, useObject) // NOLINT
{
    fs::create_directories("sandbox/a/b/d/e");
    std::ofstream("sandbox/a/b/file1.txt");

    fs::path startingPoint = fs::path("sandbox");

    auto callbacks = std::make_shared<StrictMock<MockCallbacks>>();

    EXPECT_CALL(*callbacks, includeDirectory(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*callbacks, userDefinedExclusionCheck(_)).WillOnce(Return(false));
    EXPECT_CALL(*callbacks, processFile(fs::path("sandbox/a/b/file1.txt"), false)).WillOnce(Return());

    filewalker::FileWalker fw(*callbacks);
    fw.walk(startingPoint);
}

TEST_F(TestFileWalker, reuseObject) // NOLINT
{
    fs::create_directories("sandbox/a/b/d/e");
    std::ofstream("sandbox/a/b/file1.txt");

    fs::path startingPoint = fs::path("sandbox");

    auto callbacks = std::make_shared<StrictMock<MockCallbacks>>();

    EXPECT_CALL(*callbacks, includeDirectory(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*callbacks, userDefinedExclusionCheck(_)).WillOnce(Return(false));
    EXPECT_CALL(*callbacks, processFile(fs::path("sandbox/a/b/file1.txt"), false)).WillOnce(Return());

    filewalker::FileWalker fw(*callbacks);
    fw.walk(startingPoint);

    Mock::VerifyAndClearExpectations(&callbacks);

    EXPECT_CALL(*callbacks, includeDirectory(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*callbacks, userDefinedExclusionCheck(_)).WillOnce(Return(false));
    EXPECT_CALL(*callbacks, processFile(fs::path("sandbox/a/b/file1.txt"), false)).WillOnce(Return());

    fw.walk(startingPoint);
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

TEST_F(TestFileWalker, currentDirIsDeleted) // NOLINT
{
    fs::create_directories("sandbox/a/b/d/e");
    std::ofstream("sandbox/a/b/file1.txt");

    fs::path startingPoint = fs::absolute("sandbox");
    fs::path expected = fs::absolute("sandbox/a/b/file1.txt");

    fs::path workingDir = fs::absolute("tempDir");
    fs::create_directory(workingDir);
    fs::current_path(workingDir);
    fs::remove(workingDir);

    EXPECT_ANY_THROW(fs::current_path(workingDir));

    auto callbacks = std::make_shared<StrictMock<MockCallbacks>>();

    EXPECT_CALL(*callbacks, includeDirectory(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*callbacks, userDefinedExclusionCheck(_)).WillOnce(Return(false));
    EXPECT_CALL(*callbacks, processFile(expected, false)).WillOnce(Return());

    filewalker::walk(startingPoint, *callbacks);
}

TEST_F(TestFileWalker, DISABLED_treeChangesWhileWalking) // NOLINT
{
    std::vector<fs::path> files = {
        "sandbox/a/a/file1.txt", "sandbox/a/b/file2.txt", "sandbox/b/a/file3.txt", "sandbox/b/b/file4.txt"
    };

    for (auto& p : files)
    {
        fs::create_directories(p.parent_path());
        std::ofstream(p).close();
    }

    fs::path startingPoint = fs::path("sandbox");
    auto callbacks = std::make_shared<StrictMock<MockCallbacks>>();

    EXPECT_CALL(*callbacks, includeDirectory(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*callbacks, userDefinedExclusionCheck(_)).WillOnce(Return(false));
    EXPECT_CALL(*callbacks, processFile(_, _)).Times(AtLeast(1));

    auto deleteGrandparent = [](const fs::path& filepath, bool /*symlinkTarget*/) {
        fs::remove_all(filepath.parent_path().parent_path());
    };
    EXPECT_CALL(*callbacks, processFile(_, _)).WillOnce(deleteGrandparent);

    EXPECT_NO_THROW(filewalker::walk(startingPoint, *callbacks));
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

TEST_F(TestFileWalker, userExcludeDirectory) // NOLINT
{
    fs::create_directories("sandbox/a/b/d/e");
    std::ofstream("sandbox/a/b/file1.txt");

    auto callbacks = std::make_shared<StrictMock<MockCallbacks>>();

    EXPECT_CALL(*callbacks, includeDirectory(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*callbacks, userDefinedExclusionCheck(fs::path("sandbox"))).WillOnce(Return(true));
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

TEST_F(TestFileWalker, scanPathThatDoesNotExist) // NOLINT
{
    auto callbacks = std::make_shared<StrictMock<MockCallbacks>>();

    try
    {
        filewalker::walk("DirThatDoesNotExist/FileThatDoesNotExist", *callbacks);
    }
    catch (fs::filesystem_error& e)
    {
        EXPECT_EQ(
            e.what(),
            std::string(
                "filesystem error: Failed to scan "
                "\"DirThatDoesNotExist/FileThatDoesNotExist\": file/folder does not exist: No such file or directory"));
        EXPECT_EQ(e.code().value(), ENOENT);
    }
}

TEST_F(TestFileWalker, includeSingleFile) // NOLINT
{
    fs::create_directories("sandbox/a/b/d/e");
    std::ofstream("sandbox/a/b/file1.txt");

    const fs::path& startingPath("sandbox/a/b/file1.txt");

    auto callbacks = std::make_shared<StrictMock<MockCallbacks>>();

    EXPECT_CALL(*callbacks, includeDirectory(_)).Times(0);
    EXPECT_CALL(*callbacks, userDefinedExclusionCheck(_)).Times(0);
    EXPECT_CALL(*callbacks, processFile(fs::path("sandbox/a/b/file1.txt"), false)).WillOnce(Return());

    filewalker::walk(startingPath, *callbacks);
}

TEST_F(TestFileWalker, scanStartsFromSpecial) // NOLINT
{
    ASSERT_EQ(::mkfifo("fifo", 0600), 0);
    const fs::path& startingPath("fifo");

    auto callbacks = std::make_shared<StrictMock<MockCallbacks>>();
    EXPECT_CALL(*callbacks, includeDirectory(_)).Times(0);
    EXPECT_CALL(*callbacks, userDefinedExclusionCheck(_)).Times(0);
    EXPECT_CALL(*callbacks, processFile(_, _)).Times(0);

    try
    {
        filewalker::walk(startingPath, *callbacks);
    }
    catch (fs::filesystem_error& e)
    {
        EXPECT_EQ(
            e.what(),
            std::string("filesystem error: "
                        "recursive directory iterator cannot open directory: "
                        "Not a directory [fifo]"));
        EXPECT_EQ(e.code().value(), ENOTDIR);
    }
}

TEST_F(TestFileWalker, scanWalksSpecial) // NOLINT
{
    fs::create_directories("sandbox/a/b/d/e");
    std::ofstream("sandbox/a/b/file1.txt");
    ASSERT_EQ(::mkfifo("sandbox/a/b/fifo", 0600), 0);

    fs::path startingPoint = fs::path("sandbox");

    auto callbacks = std::make_shared<StrictMock<MockCallbacks>>();

    EXPECT_CALL(*callbacks, includeDirectory(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*callbacks, userDefinedExclusionCheck(_)).WillOnce(Return(false));
    EXPECT_CALL(*callbacks, processFile(_, _)).Times(0);
    EXPECT_CALL(*callbacks, processFile(fs::path("sandbox/a/b/file1.txt"), false)).WillOnce(Return());

    filewalker::walk(startingPoint, *callbacks);
}

TEST_F(TestFileWalker, startWithSymlinkToDirectory) // NOLINT
{
    fs::create_directories("sandbox/a/b/d/e");
    std::ofstream("sandbox/a/b/file1.txt");

    fs::path startingPoint = fs::path("symlink_to_sandbox");

    fs::create_symlink("sandbox", startingPoint);

    auto callbacks = std::make_shared<StrictMock<MockCallbacks>>();

    EXPECT_CALL(*callbacks, includeDirectory(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*callbacks, userDefinedExclusionCheck(_)).WillOnce(Return(false));
    EXPECT_CALL(*callbacks, processFile(fs::path("symlink_to_sandbox/a/b/file1.txt"), true)).WillOnce(Return());

    filewalker::walk(startingPoint, *callbacks);
}

TEST_F(TestFileWalker, startWithDirectoryViaSymlink) // NOLINT
{
    fs::create_directories("sandbox/a/b/d/e");
    std::ofstream("sandbox/a/b/file1.txt");

    fs::create_symlink("sandbox", "symlink_to_sandbox");
    fs::path startingPoint = fs::path("symlink_to_sandbox/a");

    auto callbacks = std::make_shared<StrictMock<MockCallbacks>>();

    EXPECT_CALL(*callbacks, includeDirectory(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*callbacks, userDefinedExclusionCheck(_)).WillOnce(Return(false));
    EXPECT_CALL(*callbacks, processFile(fs::path("symlink_to_sandbox/a/b/file1.txt"), false)).WillOnce(Return());

    filewalker::walk(startingPoint, *callbacks);
}

TEST_F(TestFileWalker, startWithSymlinkToFile) // NOLINT
{
    fs::create_directories("sandbox/a/b/d/e");
    std::ofstream("sandbox/a/b/file1.txt");

    fs::path startingPoint = fs::path("symlink_to_file");

    fs::create_symlink("sandbox/a/b/file1.txt", startingPoint);

    auto callbacks = std::make_shared<StrictMock<MockCallbacks>>();

    EXPECT_CALL(*callbacks, includeDirectory(_)).Times(0);
    EXPECT_CALL(*callbacks, userDefinedExclusionCheck(_)).Times(0);
    EXPECT_CALL(*callbacks, processFile(fs::path("symlink_to_file"), true)).WillOnce(Return());

    filewalker::walk(startingPoint, *callbacks);
}

TEST_F(TestFileWalker, startWithBrokenSymlink) // NOLINT
{
    fs::create_directories("sandbox/a/b/d/e");
    std::ofstream("sandbox/a/b/file1.txt");

    fs::path startingPoint = fs::path("sandbox/broken_symlink");

    fs::create_symlink("missing", startingPoint);

    auto callbacks = std::make_shared<StrictMock<MockCallbacks>>();

    EXPECT_CALL(*callbacks, includeDirectory(_)).Times(0);
    EXPECT_CALL(*callbacks, userDefinedExclusionCheck(_)).Times(0);
    EXPECT_CALL(*callbacks, processFile(_, _)).Times(0);

    try
    {
        filewalker::walk(startingPoint, *callbacks);
    }
    catch (fs::filesystem_error& e)
    {
        EXPECT_EQ(
            e.what(),
            std::string("filesystem error: Failed to scan "
                        "\"sandbox/broken_symlink\": file/folder does not exist: No such file or directory"));
        EXPECT_EQ(e.code().value(), ENOENT);
    }
}

TEST_F(TestFileWalker, startWithBrokenSymlinkPath) // NOLINT
{
    fs::create_directories("sandbox/a/b/d/e");
    std::ofstream("sandbox/a/b/file1.txt");

    fs::path startingPoint = fs::path("sandbox/broken_symlink/start");

    fs::create_symlink("missing", startingPoint.parent_path());

    auto callbacks = std::make_shared<StrictMock<MockCallbacks>>();

    EXPECT_CALL(*callbacks, includeDirectory(_)).Times(0);
    EXPECT_CALL(*callbacks, userDefinedExclusionCheck(_)).Times(0);
    EXPECT_CALL(*callbacks, processFile(_, _)).Times(0);

    try
    {
        filewalker::walk(startingPoint, *callbacks);
    }
    catch (fs::filesystem_error& e)
    {
        EXPECT_EQ(
            e.what(),
            std::string("filesystem error: Failed to scan "
                        "\"sandbox/broken_symlink/start\": file/folder does not exist: No such file or directory"));
        EXPECT_EQ(e.code().value(), ENOENT);
    }
}

TEST_F(TestFileWalker, symlinksInWalk) // NOLINT
{
    fs::create_directories("sandbox/a/b/d/e");
    std::ofstream("sandbox/a/b/file1.txt");

    fs::path startingPoint = fs::path("sandbox");

    fs::create_directories("other_dir");
    std::ofstream("other_dir/file2.txt");
    fs::create_symlink("../other_dir", "sandbox/other_dir");
    fs::create_symlink("../other_dir/file2.txt", "sandbox/file2.txt");
    fs::create_symlink("absent", "sandbox/broken_symlink");

    auto callbacks = std::make_shared<StrictMock<MockCallbacks>>();

    EXPECT_CALL(*callbacks, includeDirectory(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*callbacks, userDefinedExclusionCheck(_)).WillOnce(Return(false));
    EXPECT_CALL(*callbacks, includeDirectory(fs::path("sandbox/other_dir"))).WillOnce(Return(true));
    EXPECT_CALL(*callbacks, processFile(fs::path("sandbox/a/b/file1.txt"), false)).WillOnce(Return());
    EXPECT_CALL(*callbacks, processFile(fs::path("sandbox/file2.txt"), true)).WillOnce(Return());
    EXPECT_CALL(*callbacks, processFile(fs::path("sandbox/other_dir/file2.txt"), _)).Times(0);

    filewalker::walk(startingPoint, *callbacks);
}

TEST_F(TestFileWalker, followSymlinks) // NOLINT
{
    fs::create_directories("sandbox/a/b/d/e");
    std::ofstream("sandbox/a/b/file1.txt");

    fs::path startingPoint = fs::path("sandbox");

    fs::create_directories("other_dir");
    std::ofstream("other_dir/file2.txt");
    fs::create_symlink("../other_dir", "sandbox/other_dir");
    fs::create_symlink("../other_dir/file2.txt", "sandbox/file2.txt");
    fs::create_symlink("absent", "sandbox/broken_symlink");

    auto callbacks = std::make_shared<StrictMock<MockCallbacks>>();

    EXPECT_CALL(*callbacks, includeDirectory(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*callbacks, userDefinedExclusionCheck(_)).WillOnce(Return(false));
    EXPECT_CALL(*callbacks, includeDirectory(fs::path("sandbox/other_dir"))).WillOnce(Return(true));
    EXPECT_CALL(*callbacks, processFile(fs::path("sandbox/a/b/file1.txt"), false)).WillOnce(Return());
    EXPECT_CALL(*callbacks, processFile(fs::path("sandbox/file2.txt"), true)).WillOnce(Return());
    EXPECT_CALL(*callbacks, processFile(fs::path("sandbox/other_dir/file2.txt"), _)).WillOnce(Return());

    filewalker::FileWalker fw(*callbacks);
    fw.followSymlinks();
    fw.walk(startingPoint);
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

