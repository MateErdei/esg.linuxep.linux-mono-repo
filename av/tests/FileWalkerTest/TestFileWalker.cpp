/******************************************************************************************************

Copyright 2019-2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "FileWalkerMemoryAppenderUsingTests.h"

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
        MOCK_METHOD2(userDefinedExclusionCheck, bool(const fs::path& filepath, bool isSymlink));
    };

    class TestFileWalker : public FileWalkerMemoryAppenderUsingTests
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
    EXPECT_CALL(*callbacks, userDefinedExclusionCheck(_,_)).WillOnce(Return(false));
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
    EXPECT_CALL(*callbacks, userDefinedExclusionCheck(_,_)).WillOnce(Return(false));
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
    EXPECT_CALL(*callbacks, userDefinedExclusionCheck(_,_)).WillOnce(Return(false));
    EXPECT_CALL(*callbacks, processFile(fs::path("sandbox/a/b/file1.txt"), false)).WillOnce(Return());

    filewalker::FileWalker fw(*callbacks);
    fw.walk(startingPoint);
}

TEST_F(TestFileWalker, reuseObject) // NOLINT
{
    fs::create_directories("sandbox/a/b/d/e");
    std::ofstream("sandbox/a/b/file1.txt");
    fs::create_directories("sandbox/f/g/i/j");
    std::ofstream("sandbox/f/g/file2.txt");
    fs::path startingPoint = fs::path("sandbox/a");

    auto callbacks = std::make_shared<StrictMock<MockCallbacks>>();

    EXPECT_CALL(*callbacks, includeDirectory(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*callbacks, userDefinedExclusionCheck(_,_)).WillOnce(Return(false));
    EXPECT_CALL(*callbacks, processFile(fs::path("sandbox/a/b/file1.txt"), false)).WillOnce(Return());

    filewalker::FileWalker fw(*callbacks);
    fw.walk(startingPoint);

    Mock::VerifyAndClearExpectations(&callbacks);

    startingPoint = fs::path("sandbox/f");

    EXPECT_CALL(*callbacks, includeDirectory(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*callbacks, userDefinedExclusionCheck(_,_)).WillOnce(Return(false));
    EXPECT_CALL(*callbacks, processFile(fs::path("sandbox/f/g/file2.txt"), false)).WillOnce(Return());

    fw.walk(startingPoint);
}


TEST_F(TestFileWalker, scanDirectoryOnlyOnce) // NOLINT
{
    fs::create_directories("sandbox/a/b/d/e");
    std::ofstream("sandbox/a/b/file1.txt");
    fs::create_directories("sandbox/f/g/i/j");
    std::ofstream("sandbox/f/g/file2.txt");
    fs::path startingPoint = fs::path("sandbox/a");

    auto callbacks = std::make_shared<StrictMock<MockCallbacks>>();

    EXPECT_CALL(*callbacks, includeDirectory(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*callbacks, userDefinedExclusionCheck(_,_)).WillOnce(Return(false));
    EXPECT_CALL(*callbacks, processFile(fs::path("sandbox/a/b/file1.txt"), false)).WillOnce(Return());

    filewalker::FileWalker fw(*callbacks);
    fw.walk(startingPoint);

    Mock::VerifyAndClearExpectations(&callbacks);

    startingPoint = fs::path("sandbox");

    EXPECT_CALL(*callbacks, includeDirectory(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*callbacks, userDefinedExclusionCheck(_,_)).WillOnce(Return(false));
    EXPECT_CALL(*callbacks, processFile(fs::path("sandbox/f/g/file2.txt"), false)).WillOnce(Return());

    fw.walk(startingPoint);
}

TEST_F(TestFileWalker, absoluteIncludePath) // NOLINT
{
    fs::create_directories("sandbox/a/b/d/e");
    std::ofstream("sandbox/a/b/file1.txt");

    fs::path startingPoint = fs::absolute("sandbox");

    auto callbacks = std::make_shared<StrictMock<MockCallbacks>>();

    EXPECT_CALL(*callbacks, includeDirectory(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*callbacks, userDefinedExclusionCheck(_,_)).WillOnce(Return(false));
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
    EXPECT_CALL(*callbacks, userDefinedExclusionCheck(_,_)).WillOnce(Return(false));
    EXPECT_CALL(*callbacks, processFile(expected, false)).WillOnce(Return());

    filewalker::walk(startingPoint, *callbacks);
}

TEST_F(TestFileWalker, deleteParentDirWhileWalking) // NOLINT
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
    EXPECT_CALL(*callbacks, userDefinedExclusionCheck(_,_)).WillOnce(Return(false));
    EXPECT_CALL(*callbacks, processFile(_, _)).Times(AtLeast(1));

    auto deleteGrandparent = [](const fs::path& filepath, bool /*symlinkTarget*/) {
        fs::remove_all(filepath.parent_path().parent_path());
    };
    EXPECT_CALL(*callbacks, processFile(_, _)).WillOnce(deleteGrandparent).RetiresOnSaturation();

    EXPECT_NO_THROW(filewalker::walk(startingPoint, *callbacks));
}

TEST_F(TestFileWalker, deleteCurrentDirWhileWalking) // NOLINT
{
    std::vector<fs::path> files = {
        "sandbox/a/file1.txt", "sandbox/a/file2.txt", "sandbox/b/file3.txt", "sandbox/b/file4.txt"
    };

    for (auto& p : files)
    {
        fs::create_directories(p.parent_path());
        std::ofstream(p).close();
    }

    fs::path startingPoint = fs::path("sandbox");
    auto callbacks = std::make_shared<StrictMock<MockCallbacks>>();

    EXPECT_CALL(*callbacks, includeDirectory(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*callbacks, userDefinedExclusionCheck(_,_)).WillOnce(Return(false));
    EXPECT_CALL(*callbacks, processFile(_, _)).Times(AtLeast(1));

    auto deleteParent = [](const fs::path& filepath, bool /*symlinkTarget*/) {
      fs::remove_all(filepath.parent_path());
    };
    EXPECT_CALL(*callbacks, processFile(_, _)).WillOnce(deleteParent).RetiresOnSaturation();

    EXPECT_NO_THROW(filewalker::walk(startingPoint, *callbacks));
}

TEST_F(TestFileWalker, moveCurrentDirWhileWalking) // NOLINT
{
    std::vector<fs::path> files = {
        "sandbox/a/file1.txt", "sandbox/a/file2.txt", "sandbox/b/file3.txt", "sandbox/b/file4.txt"
    };

    for (auto& p : files)
    {
        fs::create_directories(p.parent_path());
        std::ofstream(p).close();
    }

    fs::path startingPoint = fs::path("sandbox");
    auto callbacks = std::make_shared<StrictMock<MockCallbacks>>();

    EXPECT_CALL(*callbacks, includeDirectory(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*callbacks, userDefinedExclusionCheck(_,_)).WillOnce(Return(false));
    EXPECT_CALL(*callbacks, processFile(_, _)).Times(2);

    auto moveParent = [](const fs::path& filepath, bool /*symlinkTarget*/) {
      fs::rename(filepath.parent_path(), "sandbox/c");
      PRINT("Moved: " << filepath.parent_path() << " to: sandbox/c");
    };
    EXPECT_CALL(*callbacks, processFile(_, _)).WillOnce(moveParent).RetiresOnSaturation();

    EXPECT_NO_THROW(filewalker::walk(startingPoint, *callbacks));
}

class ScanRecorder
{
public:
    explicit ScanRecorder(std::vector<fs::path>& files)
        : m_allFiles(files)
    {}

    void recordScannedFiles(const sophos_filesystem::path& currentFilepath, bool /*symlinkTarget*/)
    {
        m_scannedFiles.push_back(currentFilepath);
    }

    void deleteScannedFiles(const sophos_filesystem::path& /*currentFilepath*/, bool /*symlinkTarget*/)
    {
        for (const auto& filepath : m_scannedFiles)
        {
            fs::remove(filepath);
        }
    }

    void deleteScannedDirs(const sophos_filesystem::path& /*currentFilepath*/, bool /*symlinkTarget*/)
    {
        for (const auto& filepath : m_scannedFiles)
        {
            fs::remove_all(filepath.parent_path());
        }
    }

    void deleteOneNotYetScannedFile(const sophos_filesystem::path& currentFilepath, bool /*symlinkTarget*/)
    {
        for (const auto& filepath : m_allFiles)
        {
            if (filepath != currentFilepath)
            {
                fs::remove(filepath);
                PRINT("Removed " << filepath);
                return;
            }
        }
    }

    void deleteOneNotYetScannedDir(const sophos_filesystem::path& currentFilepath, bool /*symlinkTarget*/)
    {
        for (const auto& filepath : m_allFiles)
        {
            if (filepath.parent_path() != currentFilepath.parent_path())
            {
                fs::remove_all(filepath.parent_path());
                PRINT("Removed " << filepath.parent_path());
                return;
            }
        }
    }

private:
    std::vector<sophos_filesystem::path> m_scannedFiles;
    std::vector<sophos_filesystem::path> m_allFiles;
};

TEST_F(TestFileWalker, deleteFilesAlreadyScannedWhileWalking) // NOLINT
{
    std::vector<fs::path> files = {
        "sandbox/file1.txt", "sandbox/file2.txt", "sandbox/file3.txt", "sandbox/file4.txt"
    };

    ScanRecorder scanRecorder(files);

    for (auto& p : files)
    {
        fs::create_directories(p.parent_path());
        std::ofstream(p).close();
    }

    fs::path startingPoint = fs::path("sandbox");
    auto callbacks = std::make_shared<StrictMock<MockCallbacks>>();

    EXPECT_CALL(*callbacks, includeDirectory(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*callbacks, userDefinedExclusionCheck(_,_)).WillOnce(Return(false));
    {
        InSequence seq;
        EXPECT_CALL(*callbacks, processFile(_, _)).WillOnce(Invoke(&scanRecorder, &ScanRecorder::recordScannedFiles));
        EXPECT_CALL(*callbacks, processFile(_, _)).WillOnce(Invoke(&scanRecorder, &ScanRecorder::recordScannedFiles));
        EXPECT_CALL(*callbacks, processFile(_, _)).WillOnce(Invoke(&scanRecorder, &ScanRecorder::deleteScannedFiles));
        EXPECT_CALL(*callbacks, processFile(_, _));
    }

    EXPECT_NO_THROW(filewalker::walk(startingPoint, *callbacks));
}

TEST_F(TestFileWalker, deleteDirsAlreadyScannedWhileWalking) // NOLINT
{
    std::vector<fs::path> files = {
        "sandbox/a/file1.txt", "sandbox/b/file2.txt", "sandbox/c/file3.txt", "sandbox/d/file4.txt"
    };

    ScanRecorder scanRecorder(files);

    for (auto& p : files)
    {
        fs::create_directories(p.parent_path());
        std::ofstream(p).close();
    }

    fs::path startingPoint = fs::path("sandbox");
    auto callbacks = std::make_shared<StrictMock<MockCallbacks>>();

    EXPECT_CALL(*callbacks, includeDirectory(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*callbacks, userDefinedExclusionCheck(_,_)).WillOnce(Return(false));
    {
        InSequence seq;
        EXPECT_CALL(*callbacks, processFile(_, _)).WillOnce(Invoke(&scanRecorder, &ScanRecorder::recordScannedFiles));
        EXPECT_CALL(*callbacks, processFile(_, _)).WillOnce(Invoke(&scanRecorder, &ScanRecorder::recordScannedFiles));
        EXPECT_CALL(*callbacks, processFile(_, _)).WillOnce(Invoke(&scanRecorder, &ScanRecorder::deleteScannedDirs));
        EXPECT_CALL(*callbacks, processFile(_, _));
    }

    EXPECT_NO_THROW(filewalker::walk(startingPoint, *callbacks));
}

TEST_F(TestFileWalker, deleteFileNotYetScannedWhileWalking) // NOLINT
{
    std::vector<fs::path> files = {
        "sandbox/file1.txt", "sandbox/file2.txt", "sandbox/file3.txt", "sandbox/file4.txt"
    };

    ScanRecorder scanRecorder(files);

    for (auto& p : files)
    {
        fs::create_directories(p.parent_path());
        std::ofstream(p).close();
    }

    fs::path startingPoint = fs::path("sandbox");
    auto callbacks = std::make_shared<StrictMock<MockCallbacks>>();

    EXPECT_CALL(*callbacks, includeDirectory(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*callbacks, userDefinedExclusionCheck(_,_)).WillOnce(Return(false));
    EXPECT_CALL(*callbacks, processFile(_, _)).Times(2);
    EXPECT_CALL(*callbacks, processFile(_, _)).WillOnce(Invoke(&scanRecorder, &ScanRecorder::deleteOneNotYetScannedFile)).RetiresOnSaturation();

    EXPECT_NO_THROW(filewalker::walk(startingPoint, *callbacks));
}

TEST_F(TestFileWalker, deleteDirNotYetScannedWhileWalking) // NOLINT
{
    std::vector<fs::path> files = {
        "sandbox/a/file1.txt", "sandbox/b/file2.txt", "sandbox/c/file3.txt", "sandbox/d/file4.txt"
    };

    ScanRecorder scanRecorder(files);

    for (auto& p : files)
    {
        fs::create_directories(p.parent_path());
        std::ofstream(p).close();
    }

    fs::path startingPoint = fs::path("sandbox");
    auto callbacks = std::make_shared<StrictMock<MockCallbacks>>();

    EXPECT_CALL(*callbacks, includeDirectory(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*callbacks, userDefinedExclusionCheck(_,_)).WillOnce(Return(false));
    EXPECT_CALL(*callbacks, processFile(_, _)).Times(2);
    EXPECT_CALL(*callbacks, processFile(_, _)).WillOnce(Invoke(&scanRecorder, &ScanRecorder::deleteOneNotYetScannedDir)).RetiresOnSaturation();

    EXPECT_NO_THROW(filewalker::walk(startingPoint, *callbacks));
}

TEST_F(TestFileWalker, filewalkerHandlesExceptionFromProcessFile) // NOLINT
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    std::vector<fs::path> files = {
        "sandbox/file1.txt", "sandbox/file2.txt", "sandbox/file3.txt", "sandbox/file4.txt"
    };

    for (auto& p : files)
    {
        fs::create_directories(p.parent_path());
        std::ofstream(p).close();
    }

    fs::path startingPoint = fs::absolute("sandbox");
    std::string expected = "Failed to process: " + startingPoint.string();
    auto callbacks = std::make_shared<StrictMock<MockCallbacks>>();

    EXPECT_CALL(*callbacks, includeDirectory(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*callbacks, userDefinedExclusionCheck(_,_)).WillOnce(Return(false));
    EXPECT_CALL(*callbacks, processFile(_, _)).Times(AtLeast(1));

    std::error_code ec (ENOENT, std::system_category());

    fs::filesystem_error fileDoesNotExist("File does not exist", ec);
    EXPECT_CALL(*callbacks, processFile(_, _)).WillOnce(Throw(fileDoesNotExist)).RetiresOnSaturation();

    EXPECT_NO_THROW(filewalker::walk(startingPoint, *callbacks));

    EXPECT_TRUE(appenderContains(expected));
}

TEST_F(TestFileWalker, excludeDirectory) // NOLINT
{
    fs::create_directories("sandbox/a/b/d/e");
    std::ofstream("sandbox/a/b/file1.txt");

    auto callbacks = std::make_shared<StrictMock<MockCallbacks>>();

    EXPECT_CALL(*callbacks, includeDirectory(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*callbacks, userDefinedExclusionCheck(_,_)).WillOnce(Return(false));
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
    EXPECT_CALL(*callbacks, userDefinedExclusionCheck(fs::path("sandbox"), _)).WillOnce(Return(true));
    EXPECT_CALL(*callbacks, processFile(fs::path("sandbox/a/b/file1.txt"), _)).Times(0);

    filewalker::walk("sandbox", *callbacks);
}

TEST_F(TestFileWalker, scanFileThatDoesNotExist) // NOLINT
{
    auto callbacks = std::make_shared<StrictMock<MockCallbacks>>();

    try
    {
        filewalker::walk("FileThatDoesNotExist", *callbacks);
        FAIL() << "walk() didn't throw";
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

TEST_F(TestFileWalker, scanDirectoryThatDoesNotExist) // NOLINT
{
    auto callbacks = std::make_shared<StrictMock<MockCallbacks>>();

    try
    {
        filewalker::walk("DirectoryThatDoesNotExist/", *callbacks);
        FAIL() << "walk() didn't throw";
    }
    catch (fs::filesystem_error& e)
    {
        EXPECT_EQ(
            e.what(),
            std::string("filesystem error: Failed to scan "
                        "\"DirectoryThatDoesNotExist/\": file/folder does not exist: No such file or directory"));
        EXPECT_EQ(e.code().value(), ENOENT);
    }
}

TEST_F(TestFileWalker, scanPathThatDoesNotExist) // NOLINT
{
    auto callbacks = std::make_shared<StrictMock<MockCallbacks>>();

    try
    {
        filewalker::walk("DirThatDoesNotExist/FileThatDoesNotExist", *callbacks);
        FAIL() << "walk() didn't throw";
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
    EXPECT_CALL(*callbacks, userDefinedExclusionCheck(_,_)).Times(0);
    EXPECT_CALL(*callbacks, processFile(fs::path("sandbox/a/b/file1.txt"), false)).WillOnce(Return());

    filewalker::walk(startingPath, *callbacks);
}

TEST_F(TestFileWalker, scanStartsFromSpecial) // NOLINT
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    ASSERT_EQ(::mkfifo("fifo", 0600), 0);
    const fs::path& startingPath("fifo");

    auto callbacks = std::make_shared<StrictMock<MockCallbacks>>();
    EXPECT_CALL(*callbacks, includeDirectory(_)).Times(0);
    EXPECT_CALL(*callbacks, userDefinedExclusionCheck(_,_)).Times(0);
    EXPECT_CALL(*callbacks, processFile(_, _)).Times(0);

    EXPECT_NO_THROW(filewalker::walk(startingPath, *callbacks));

    EXPECT_TRUE(appenderContains("Not scanning special file/device: \"fifo\""));
}

TEST_F(TestFileWalker, scanStartsFromSymlinkToSpecial) // NOLINT
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    ASSERT_EQ(::mkfifo("fifo", 0600), 0);
    const fs::path& startingPath("symlink_to_fifo");
    fs::create_symlink("fifo", startingPath);

    auto callbacks = std::make_shared<StrictMock<MockCallbacks>>();
    EXPECT_CALL(*callbacks, includeDirectory(_)).Times(0);
    EXPECT_CALL(*callbacks, userDefinedExclusionCheck(_,_)).Times(0);
    EXPECT_CALL(*callbacks, processFile(_, _)).Times(0);

    EXPECT_NO_THROW(filewalker::walk(startingPath, *callbacks));

    EXPECT_TRUE(appenderContains("Not scanning special file/device: \"symlink_to_fifo\""));
}

TEST_F(TestFileWalker, scanWalksSpecial) // NOLINT
{
    fs::create_directories("sandbox/a/b/d/e");
    std::ofstream("sandbox/a/b/file1.txt");
    ASSERT_EQ(::mkfifo("sandbox/a/b/fifo", 0600), 0);

    fs::path startingPoint = fs::path("sandbox");

    auto callbacks = std::make_shared<StrictMock<MockCallbacks>>();

    EXPECT_CALL(*callbacks, includeDirectory(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*callbacks, userDefinedExclusionCheck(_,_)).WillOnce(Return(false));
    EXPECT_CALL(*callbacks, processFile(_, _)).Times(0);
    EXPECT_CALL(*callbacks, processFile(fs::path("sandbox/a/b/file1.txt"), false)).WillOnce(Return());

    filewalker::walk(startingPoint, *callbacks);
}

TEST_F(TestFileWalker, scanWalksSymlinkToSpecial) // NOLINT
{
    fs::create_directories("sandbox/a/b/d/e");
    std::ofstream("sandbox/a/b/file1.txt");
    ASSERT_EQ(::mkfifo("fifo", 0600), 0);
    fs::create_symlink("../fifo", "sandbox/fifo");

    fs::path startingPoint = fs::path("sandbox");

    auto callbacks = std::make_shared<StrictMock<MockCallbacks>>();

    EXPECT_CALL(*callbacks, includeDirectory(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*callbacks, userDefinedExclusionCheck(_,_)).WillOnce(Return(false));
    EXPECT_CALL(*callbacks, processFile(_, _)).Times(0);
    EXPECT_CALL(*callbacks, processFile(fs::path("sandbox/a/b/file1.txt"), false)).WillOnce(Return());
    EXPECT_CALL(*callbacks, includeDirectory(fs::path("fifo"))).Times(0);
    EXPECT_CALL(*callbacks, includeDirectory(fs::path("sandbox/fifo"))).Times(0);

    filewalker::FileWalker walker(*callbacks);
    walker.followSymlinks();
    walker.walk(startingPoint);
}

TEST_F(TestFileWalker, startWithSymlinkToDirectory) // NOLINT
{
    fs::create_directories("sandbox/a/b/d/e");
    std::ofstream("sandbox/a/b/file1.txt");

    fs::path startingPoint = fs::path("symlink_to_sandbox");

    fs::create_symlink("sandbox", startingPoint);

    auto callbacks = std::make_shared<StrictMock<MockCallbacks>>();

    EXPECT_CALL(*callbacks, includeDirectory(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*callbacks, userDefinedExclusionCheck(_,_)).WillOnce(Return(false));
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
    EXPECT_CALL(*callbacks, userDefinedExclusionCheck(_,_)).WillOnce(Return(false));
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
    EXPECT_CALL(*callbacks, userDefinedExclusionCheck(_,_)).Times(0);
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
    EXPECT_CALL(*callbacks, userDefinedExclusionCheck(_,_)).Times(0);
    EXPECT_CALL(*callbacks, processFile(_, _)).Times(0);

    try
    {
        filewalker::walk(startingPoint, *callbacks);
        FAIL() << "walk() didn't throw";
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
    EXPECT_CALL(*callbacks, userDefinedExclusionCheck(_,_)).Times(0);
    EXPECT_CALL(*callbacks, processFile(_, _)).Times(0);

    try
    {
        filewalker::walk(startingPoint, *callbacks);
        FAIL() << "walk() didn't throw";
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

TEST_F(TestFileWalker, symlinksInWalkNoFollow) // NOLINT
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    fs::create_directories("sandbox/a/b/d/e");
    std::ofstream("sandbox/a/b/file1.txt");

    fs::path startingPoint = fs::path("sandbox");

    fs::create_directories("other_dir");
    std::ofstream("other_dir/file2.txt");
    fs::create_symlink("../other_dir", "sandbox/other_dir");
    fs::create_symlink("../other_dir/file2.txt", "sandbox/file2.txt");
    fs::create_symlink("absent", "sandbox/broken_symlink");

    fs::create_directories("other_dir2");
    std::ofstream("other_dir2/file3.txt");
    fs::create_symlink("../other_dir2/", "sandbox/other_dir2");
    fs::create_symlink("absent2/", "sandbox/broken_symlink2");

    auto callbacks = std::make_shared<StrictMock<MockCallbacks>>();

    EXPECT_CALL(*callbacks, includeDirectory(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*callbacks, userDefinedExclusionCheck(_,_)).WillOnce(Return(false));

    EXPECT_CALL(*callbacks, includeDirectory(fs::path("sandbox/other_dir"))).Times(0);
    EXPECT_CALL(*callbacks, processFile(fs::path("sandbox/a/b/file1.txt"), false)).WillOnce(Return());
    EXPECT_CALL(*callbacks, processFile(fs::path("sandbox/file2.txt"), true)).Times(0);
    EXPECT_CALL(*callbacks, processFile(fs::path("sandbox/other_dir/file2.txt"), _)).Times(0);
    EXPECT_CALL(*callbacks, processFile(fs::path("sandbox/broken_symlink"), _)).Times(0);

    EXPECT_CALL(*callbacks, includeDirectory(fs::path("sandbox/other_dir2"))).Times(0);
    EXPECT_CALL(*callbacks, processFile(fs::path("sandbox/other_dir2/file3.txt"), _)).Times(0);
    EXPECT_CALL(*callbacks, processFile(fs::path("sandbox/broken_symlink2"), _)).Times(0);

    filewalker::FileWalker walker(*callbacks);
    walker.walk(startingPoint);
}

TEST_F(TestFileWalker, followSymlinksInWalk) // NOLINT
{
    fs::create_directories("sandbox/a/b/d/e");
    std::ofstream("sandbox/a/b/file1.txt");

    fs::path startingPoint = fs::path("sandbox");

    fs::create_directories("other_dir");
    std::ofstream("other_dir/file2.txt");
    fs::create_symlink("../other_dir", "sandbox/other_dir");
    fs::create_symlink("../other_dir/file2.txt", "sandbox/file2.txt");
    fs::create_symlink("absent", "sandbox/broken_symlink");

    fs::create_directories("other_dir2");
    std::ofstream("other_dir2/file3.txt");
    fs::create_symlink("../other_dir2/", "sandbox/other_dir2");
    fs::create_symlink("absent2/", "sandbox/broken_symlink2");

    auto callbacks = std::make_shared<StrictMock<MockCallbacks>>();

    EXPECT_CALL(*callbacks, includeDirectory(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*callbacks, userDefinedExclusionCheck(_,_)).WillOnce(Return(false));

    EXPECT_CALL(*callbacks, includeDirectory(fs::path("sandbox/other_dir"))).WillOnce(Return(true));
    EXPECT_CALL(*callbacks, processFile(fs::path("sandbox/a/b/file1.txt"), false)).WillOnce(Return());
    EXPECT_CALL(*callbacks, processFile(fs::path("sandbox/file2.txt"), true)).WillOnce(Return());
    EXPECT_CALL(*callbacks, processFile(fs::path("sandbox/other_dir/file2.txt"), _)).WillOnce(Return());
    EXPECT_CALL(*callbacks, processFile(fs::path("sandbox/broken_symlink"), _)).Times(0);

    EXPECT_CALL(*callbacks, includeDirectory(fs::path("sandbox/other_dir2"))).WillOnce(Return(true));
    EXPECT_CALL(*callbacks, processFile(fs::path("sandbox/other_dir2/file3.txt"), _)).WillOnce(Return());
    EXPECT_CALL(*callbacks, processFile(fs::path("sandbox/broken_symlink2"), _)).Times(0);

    filewalker::FileWalker fw(*callbacks);
    fw.followSymlinks();
    fw.walk(startingPoint);
}

TEST_F(TestFileWalker, duplicateSymlinksInWalkNoFollow) // NOLINT
{
    fs::create_directories("sandbox");
    fs::path startingPoint = fs::path("sandbox");

    fs::create_directories("other_dir");
    std::ofstream("other_dir/file2.txt");
    fs::create_symlink("../other_dir", "sandbox/link1");
    fs::create_symlink("../other_dir", "sandbox/link2");

    auto callbacks = std::make_shared<StrictMock<MockCallbacks>>();

    EXPECT_CALL(*callbacks, includeDirectory(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*callbacks, userDefinedExclusionCheck(_,_)).WillOnce(Return(false));
    EXPECT_CALL(*callbacks, processFile(_, _)).Times(0);

    filewalker::walk(startingPoint, *callbacks);
}

TEST_F(TestFileWalker, duplicateDirSymlinksInWalk) // NOLINT
{
    fs::create_directories("sandbox");
    fs::path startingPoint = fs::path("sandbox");

    fs::create_directories("other_dir");
    std::ofstream("other_dir/file2.txt");
    fs::create_symlink("../other_dir", "sandbox/dirlink1");
    fs::create_symlink("../other_dir", "sandbox/dirlink2");

    auto callbacks = std::make_shared<StrictMock<MockCallbacks>>();

    EXPECT_CALL(*callbacks, includeDirectory(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*callbacks, userDefinedExclusionCheck(_,_)).WillOnce(Return(false));
    auto expected = AnyOf(fs::path("sandbox/dirlink1/file2.txt"), fs::path("sandbox/dirlink2/file2.txt"));
    EXPECT_CALL(*callbacks, processFile(expected, false)).Times(1);

    filewalker::FileWalker fw(*callbacks);
    fw.followSymlinks();
    fw.walk(startingPoint);
}

TEST_F(TestFileWalker, scanDirectoryAndSymlinkToDirectory) // NOLINT
{
    fs::create_directories("sandbox/a/b");
    fs::create_directories("sandbox/c/d");
    std::ofstream("sandbox/a/b/file1.txt");
    std::ofstream("sandbox/c/d/file2.txt");
    fs::create_symlink("../c/d", "sandbox/a/d");
    fs::create_symlink("../a/b", "sandbox/c/b");

    fs::path startingPoint = fs::path("sandbox");

    auto callbacks = std::make_shared<StrictMock<MockCallbacks>>();

    EXPECT_CALL(*callbacks, includeDirectory(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*callbacks, userDefinedExclusionCheck(_,_)).WillOnce(Return(false));
    auto expected1 = AnyOf(fs::path("sandbox/a/b/file1.txt"), fs::path("sandbox/c/b/file1.txt"));
    EXPECT_CALL(*callbacks, processFile(expected1, false)).Times(1);
    auto expected2 = AnyOf(fs::path("sandbox/c/d/file2.txt"), fs::path("sandbox/a/d/file2.txt"));
    EXPECT_CALL(*callbacks, processFile(expected2, false)).Times(1);

    filewalker::FileWalker fw(*callbacks);
    fw.followSymlinks();
    fw.walk(startingPoint);
}

TEST_F(TestFileWalker, symlinksToStartingDir) // NOLINT
{
    fs::create_directories("sandbox");
    std::ofstream("sandbox/file1.txt");
    fs::create_symlink(".", "sandbox/dirlink1");
    fs::create_directories("sandbox/dir2");
    fs::create_symlink("..", "sandbox/dir2/dirlink2");
    fs::path startingPoint = fs::path("sandbox");

    auto callbacks = std::make_shared<StrictMock<MockCallbacks>>();

    EXPECT_CALL(*callbacks, includeDirectory(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*callbacks, userDefinedExclusionCheck(_,_)).WillOnce(Return(false));
    EXPECT_CALL(*callbacks, processFile(fs::path("sandbox/file1.txt"), false)).Times(1);

    filewalker::FileWalker fw(*callbacks);
    fw.followSymlinks();
    fw.walk(startingPoint);
}

TEST_F(TestFileWalker, duplicateFileSymlinksInWalk) // NOLINT
{
    fs::create_directories("sandbox");
    fs::path startingPoint = fs::path("sandbox");

    fs::create_directories("other_dir");
    std::ofstream("other_dir/file1.txt");
    fs::create_symlink("../other_dir/file1.txt", "sandbox/filelink1");
    fs::create_symlink("../other_dir/file1.txt", "sandbox/filelink2");

    auto callbacks = std::make_shared<StrictMock<MockCallbacks>>();

    EXPECT_CALL(*callbacks, includeDirectory(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*callbacks, userDefinedExclusionCheck(_,_)).WillOnce(Return(false));
    auto expected = AnyOf(fs::path("sandbox/filelink1"), fs::path("sandbox/filelink2"));
    EXPECT_CALL(*callbacks, processFile(expected, true)).WillRepeatedly(Return());

    filewalker::FileWalker fw(*callbacks);
    fw.followSymlinks();
    fw.walk(startingPoint);
}

TEST_F(TestFileWalker, backtrackProtectionAppliesAcrossMultipleWalks) // NOLINT
{
    fs::create_directories("sandbox/dir1");
    fs::create_directories("sandbox/dir2");

    fs::create_directories("other_dir");
    std::ofstream("other_dir/file.txt");
    fs::create_symlink("../../other_dir", "sandbox/dir1/dirlink1");
    fs::create_symlink("../../other_dir", "sandbox/dir2/dirlink2");

    auto callbacks = std::make_shared<StrictMock<MockCallbacks>>();

    EXPECT_CALL(*callbacks, includeDirectory(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*callbacks, userDefinedExclusionCheck(_,_)).WillRepeatedly(Return(false));
    EXPECT_CALL(*callbacks, processFile(fs::path("sandbox/dir1/dirlink1/file.txt"), false)).Times(1);

    filewalker::FileWalker fw(*callbacks);
    fw.followSymlinks();
    fw.walk("sandbox/dir1");
    fw.walk("sandbox/dir2");
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
    EXPECT_CALL(*callbacks, userDefinedExclusionCheck(_, _)).WillOnce(Return(false));
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
    EXPECT_CALL(*callbacks, userDefinedExclusionCheck(_,_)).Times(0);
    EXPECT_CALL(*callbacks, processFile(_, _)).Times(0);

    try
    {
        filewalker::walk(pathToScan, *callbacks);
        FAIL() << "walk() didn't throw";
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

TEST_F(TestFileWalker, withStayOnDevice) // NOLINT
{
    fs::create_directories("sandbox/a/b/d/e");
    std::ofstream("sandbox/a/b/file1.txt");

    fs::path startingPoint = fs::path("sandbox");

    auto callbacks = std::make_shared<StrictMock<MockCallbacks>>();

    EXPECT_CALL(*callbacks, includeDirectory(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*callbacks, userDefinedExclusionCheck(_,_)).WillOnce(Return(false));
    EXPECT_CALL(*callbacks, processFile(fs::path("sandbox/a/b/file1.txt"), false)).WillOnce(Return());

    filewalker::FileWalker fw(*callbacks);
    fw.stayOnDevice();
    fw.walk(startingPoint);
}