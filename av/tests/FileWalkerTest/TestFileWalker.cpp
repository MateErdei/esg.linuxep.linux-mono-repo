// Copyright 2019-2023 Sophos Limited. All rights reserved.

#include "TestFileWalker.h"
#include "MockCallbacks.h"

#include "filewalker/FileWalker.h"
#include "common/AbortScanException.h"

#include <gtest/gtest.h>

#include <cerrno>
#include <fstream>
#include <string>

namespace fs = sophos_filesystem;

using namespace ::testing;

TEST_F(TestFileWalker, includeDirectory)
{
    fs::create_directories("sandbox/a/b/d/e");
    std::ofstream("sandbox/a/b/file1.txt").close();

    fs::path startingPoint = fs::path("sandbox");

    auto callbacks = std::make_shared<StrictMock<MockCallbacks>>();

    EXPECT_CALL(*callbacks, includeDirectory(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*callbacks, userDefinedExclusionCheck(_,_)).WillOnce(Return(false));
    EXPECT_CALL(*callbacks, processFile(fs::path("sandbox/a/b/file1.txt"), false)).WillOnce(Return());

    filewalker::FileWalker fw(*callbacks);
    fw.walk(startingPoint);
}

TEST_F(TestFileWalker, includeCurrentDirectory)
{
    fs::create_directories("sandbox/a/b/d/e");
    std::ofstream("sandbox/a/b/file1.txt").close();

    fs::path startingPoint = fs::path(".");

    auto callbacks = std::make_shared<StrictMock<MockCallbacks>>();

    EXPECT_CALL(*callbacks, includeDirectory(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*callbacks, userDefinedExclusionCheck(_,_)).WillOnce(Return(false));
    EXPECT_CALL(*callbacks, processFile(fs::path("./sandbox/a/b/file1.txt"), false)).WillOnce(Return());

    filewalker::FileWalker fw(*callbacks);
    fw.walk(startingPoint);
}

TEST_F(TestFileWalker, reuseObject)
{
    fs::create_directories("sandbox/a/b/d/e");
    std::ofstream("sandbox/a/b/file1.txt").close();
    fs::create_directories("sandbox/f/g/i/j");
    std::ofstream("sandbox/f/g/file2.txt").close();
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


TEST_F(TestFileWalker, scanDirectoryOnlyOnce)
{
    fs::create_directories("sandbox/a/b/d/e");
    std::ofstream("sandbox/a/b/file1.txt").close();
    fs::create_directories("sandbox/f/g/i/j");
    std::ofstream("sandbox/f/g/file2.txt").close();
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

TEST_F(TestFileWalker, absoluteIncludePath)
{
    fs::create_directories("sandbox/a/b/d/e");
    std::ofstream("sandbox/a/b/file1.txt").close();

    fs::path startingPoint = fs::absolute("sandbox");

    auto callbacks = std::make_shared<StrictMock<MockCallbacks>>();

    EXPECT_CALL(*callbacks, includeDirectory(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*callbacks, userDefinedExclusionCheck(_,_)).WillOnce(Return(false));
    EXPECT_CALL(*callbacks, processFile(fs::absolute("sandbox/a/b/file1.txt"), false)).WillOnce(Return());

    filewalker::FileWalker fw(*callbacks);
    fw.walk(startingPoint);
}

TEST_F(TestFileWalker, currentDirIsDeleted)
{
    fs::create_directories("sandbox/a/b/d/e");
    std::ofstream("sandbox/a/b/file1.txt").close();

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

    filewalker::FileWalker fw(*callbacks);
    fw.walk(startingPoint);
}

TEST_F(TestFileWalker, deleteParentDirWhileWalking)
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

    filewalker::FileWalker fw(*callbacks);
    EXPECT_NO_THROW(fw.walk(startingPoint));
}

TEST_F(TestFileWalker, deleteCurrentDirWhileWalking)
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

    filewalker::FileWalker fw(*callbacks);
    EXPECT_NO_THROW(fw.walk(startingPoint));
}

TEST_F(TestFileWalker, moveCurrentDirWhileWalking)
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

    filewalker::FileWalker fw(*callbacks);
    EXPECT_NO_THROW(fw.walk(startingPoint));
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

TEST_F(TestFileWalker, deleteFilesAlreadyScannedWhileWalking)
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

    filewalker::FileWalker fw(*callbacks);
    EXPECT_NO_THROW(fw.walk(startingPoint));
}

TEST_F(TestFileWalker, deleteDirsAlreadyScannedWhileWalking)
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

    filewalker::FileWalker fw(*callbacks);
    EXPECT_NO_THROW(fw.walk(startingPoint));
}

TEST_F(TestFileWalker, deleteFileNotYetScannedWhileWalking)
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

    filewalker::FileWalker fw(*callbacks);
    EXPECT_NO_THROW(fw.walk(startingPoint));
}

TEST_F(TestFileWalker, deleteDirNotYetScannedWhileWalking)
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

    filewalker::FileWalker fw(*callbacks);
    EXPECT_NO_THROW(fw.walk(startingPoint));
}


TEST_F(TestFileWalker, handlesExceptionFromProcessFile)
{
    fs::create_directories("sandbox");
    std::ofstream("sandbox/file1.txt").close();

    fs::path startingPoint = fs::absolute("sandbox/file1.txt");
    auto callbacks = std::make_shared<StrictMock<MockCallbacks>>();

    std::error_code ec (ENOENT, std::system_category());
    fs::filesystem_error fileDoesNotExist("File does not exist", ec);
    EXPECT_CALL(*callbacks, processFile(_, _)).WillOnce(Throw(fileDoesNotExist));
    EXPECT_CALL(*callbacks, registerError(_, _)).Times(1);

    filewalker::FileWalker fw(*callbacks);
    EXPECT_NO_THROW(fw.walk(startingPoint));
}

TEST_F(TestFileWalker, handlesExceptionFromProcessFileInWalk)
{
    std::vector<fs::path> files = {
        "sandbox/file1.txt", "sandbox/file2.txt", "sandbox/file3.txt", "sandbox/file4.txt"
    };

    for (auto& p : files)
    {
        fs::create_directories(p.parent_path());
        std::ofstream(p).close();
    }

    fs::path startingPoint = fs::absolute("sandbox");
    auto callbacks = std::make_shared<StrictMock<MockCallbacks>>();

    EXPECT_CALL(*callbacks, includeDirectory(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*callbacks, userDefinedExclusionCheck(_,_)).WillOnce(Return(false));
    EXPECT_CALL(*callbacks, processFile(_, _)).Times(AtLeast(1));

    std::error_code ec (ENOENT, std::system_category());

    fs::filesystem_error fileDoesNotExist("File does not exist", ec);
    EXPECT_CALL(*callbacks, processFile(_, _)).WillOnce(Throw(fileDoesNotExist)).RetiresOnSaturation();
    EXPECT_CALL(*callbacks, registerError(_, _)).Times(1);

    filewalker::FileWalker fw(*callbacks);
    EXPECT_NO_THROW(fw.walk(startingPoint));
}


TEST_F(TestFileWalker, abortScanExceptionFromProcessFile)
{
    fs::create_directories("sandbox");
    std::ofstream("sandbox/file1.txt").close();

    fs::path startingPoint = fs::absolute("sandbox/file1.txt");
    auto callbacks = std::make_shared<StrictMock<MockCallbacks>>();

    common::AbortScanException abortScan("Cannot connect to scanning service");
    EXPECT_CALL(*callbacks, processFile(_, _)).WillOnce(Throw(abortScan));

    try
    {
        filewalker::FileWalker fw(*callbacks);
        fw.walk(startingPoint);
        FAIL() << "walk() did not throw";
    }
    catch (common::AbortScanException& ex)
    {
        ASSERT_STREQ(ex.what(), abortScan.what());
    }
}

TEST_F(TestFileWalker, abortScanExceptionFromProcessFileInWalk)
{
    std::vector<fs::path> files = {
        "sandbox/file1.txt", "sandbox/file2.txt", "sandbox/file3.txt", "sandbox/file4.txt"
    };

    for (auto& p : files)
    {
        fs::create_directories(p.parent_path());
        std::ofstream(p).close();
    }

    fs::path startingPoint = fs::absolute("sandbox");
    auto callbacks = std::make_shared<StrictMock<MockCallbacks>>();

    EXPECT_CALL(*callbacks, includeDirectory(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*callbacks, userDefinedExclusionCheck(_,_)).WillOnce(Return(false));
    EXPECT_CALL(*callbacks, processFile(_, _)).Times(0);

    common::AbortScanException abortScan("Cannot connect to scanning service");
    EXPECT_CALL(*callbacks, processFile(_, _)).WillOnce(Throw(abortScan)).RetiresOnSaturation();

    try
    {
        filewalker::FileWalker fw(*callbacks);
        fw.walk(startingPoint);
        FAIL() << "walk() did not throw";
    }
    catch (common::AbortScanException& ex)
    {
        ASSERT_STREQ(ex.what(), abortScan.what());
    }
}

TEST_F(TestFileWalker, excludeDirectory)
{
    fs::create_directories("sandbox/a/b/d/e");
    std::ofstream("sandbox/a/b/file1.txt").close();

    auto callbacks = std::make_shared<StrictMock<MockCallbacks>>();

    EXPECT_CALL(*callbacks, includeDirectory(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*callbacks, userDefinedExclusionCheck(_,_)).WillOnce(Return(false));
    EXPECT_CALL(*callbacks, includeDirectory(fs::path("sandbox/a/b"))).WillOnce(Return(false));
    EXPECT_CALL(*callbacks, processFile(fs::path("sandbox/a/b/file1.txt"), _)).Times(0);

    filewalker::FileWalker fw(*callbacks);
    fw.walk("sandbox");
}

TEST_F(TestFileWalker, userExcludeDirectory)
{
    fs::create_directories("sandbox/a/b/d/e");
    std::ofstream("sandbox/a/b/file1.txt").close();

    auto callbacks = std::make_shared<StrictMock<MockCallbacks>>();

    EXPECT_CALL(*callbacks, includeDirectory(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*callbacks, userDefinedExclusionCheck(fs::path("sandbox"), _)).WillOnce(Return(true));
    EXPECT_CALL(*callbacks, processFile(fs::path("sandbox/a/b/file1.txt"), _)).Times(0);

    filewalker::FileWalker fw(*callbacks);
    fw.walk("sandbox");
}

TEST_F(TestFileWalker, scanFileThatDoesNotExist)
{
    auto callbacks = std::make_shared<StrictMock<MockCallbacks>>();

    try
    {
        filewalker::FileWalker fw(*callbacks);
        fw.walk("FileThatDoesNotExist");
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

TEST_F(TestFileWalker, scanDirectoryThatDoesNotExist)
{
    auto callbacks = std::make_shared<StrictMock<MockCallbacks>>();

    try
    {
        filewalker::FileWalker fw(*callbacks);
        fw.walk("DirectoryThatDoesNotExist/");
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

TEST_F(TestFileWalker, scanPathThatDoesNotExist)
{
    auto callbacks = std::make_shared<StrictMock<MockCallbacks>>();

    try
    {
        filewalker::FileWalker fw(*callbacks);
        fw.walk("DirThatDoesNotExist/FileThatDoesNotExist");
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

TEST_F(TestFileWalker, scanPathThatDoesNotExistWithSpecialCharacter)
{
    auto callbacks = std::make_shared<StrictMock<MockCallbacks>>();

    try
    {
        filewalker::FileWalker fw(*callbacks);
        fw.walk("DirThatDoesNotExist/FileThatDoesNotExist\a");
        FAIL() << "walk() didn't throw";
    }
    catch (fs::filesystem_error& e)
    {
        EXPECT_EQ(
            e.what(),
            std::string(
                "filesystem error: Failed to scan "
                "\"DirThatDoesNotExist/FileThatDoesNotExist\\a\": file/folder does not exist: No such file or directory"));
        EXPECT_EQ(e.code().value(), ENOENT);
    }
}

TEST_F(TestFileWalker, includeSingleFile)
{
    fs::create_directories("sandbox/a/b/d/e");
    std::ofstream("sandbox/a/b/file1.txt").close();

    const fs::path& startingPath("sandbox/a/b/file1.txt");

    auto callbacks = std::make_shared<StrictMock<MockCallbacks>>();

    EXPECT_CALL(*callbacks, includeDirectory(_)).Times(0);
    EXPECT_CALL(*callbacks, userDefinedExclusionCheck(_,_)).Times(0);
    EXPECT_CALL(*callbacks, processFile(fs::path("sandbox/a/b/file1.txt"), false)).WillOnce(Return());

    filewalker::FileWalker fw(*callbacks);
    fw.walk(startingPath);
}

TEST_F(TestFileWalker, scanStartsFromSpecial)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    ASSERT_EQ(::mkfifo("fifo", 0600), 0);
    const fs::path& startingPath("fifo");

    auto callbacks = std::make_shared<StrictMock<MockCallbacks>>();
    EXPECT_CALL(*callbacks, includeDirectory(_)).Times(0);
    EXPECT_CALL(*callbacks, userDefinedExclusionCheck(_,_)).Times(0);
    EXPECT_CALL(*callbacks, processFile(_, _)).Times(0);

    filewalker::FileWalker fw(*callbacks);
    EXPECT_NO_THROW(fw.walk(startingPath));

    EXPECT_TRUE(appenderContains("Not scanning special file/device: \"fifo\""));
}

TEST_F(TestFileWalker, scanStartsFromSymlinkToSpecial)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    ASSERT_EQ(::mkfifo("fifo", 0600), 0);
    const fs::path& startingPath("symlink_to_fifo");
    fs::create_symlink("fifo", startingPath);

    auto callbacks = std::make_shared<StrictMock<MockCallbacks>>();
    EXPECT_CALL(*callbacks, includeDirectory(_)).Times(0);
    EXPECT_CALL(*callbacks, userDefinedExclusionCheck(_,_)).Times(0);
    EXPECT_CALL(*callbacks, processFile(_, _)).Times(0);

    filewalker::FileWalker fw(*callbacks);
    EXPECT_NO_THROW(fw.walk(startingPath));

    EXPECT_TRUE(appenderContains("Not scanning special file/device: \"symlink_to_fifo\""));
}

TEST_F(TestFileWalker, scanWalksSpecial)
{
    fs::create_directories("sandbox/a/b/d/e");
    std::ofstream("sandbox/a/b/file1.txt").close();
    ASSERT_EQ(::mkfifo("sandbox/a/b/fifo", 0600), 0);

    fs::path startingPoint = fs::path("sandbox");

    auto callbacks = std::make_shared<StrictMock<MockCallbacks>>();

    EXPECT_CALL(*callbacks, includeDirectory(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*callbacks, userDefinedExclusionCheck(_,_)).WillOnce(Return(false));
    EXPECT_CALL(*callbacks, processFile(_, _)).Times(0);
    EXPECT_CALL(*callbacks, processFile(fs::path("sandbox/a/b/file1.txt"), false)).WillOnce(Return());

    filewalker::FileWalker fw(*callbacks);
    fw.walk(startingPoint);
}

TEST_F(TestFileWalker, scanWalksSymlinkToSpecial)
{
    fs::create_directories("sandbox/a/b/d/e");
    std::ofstream("sandbox/a/b/file1.txt").close();
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

TEST_F(TestFileWalker, startWithSymlinkToDirectory)
{
    fs::create_directories("sandbox/a/b/d/e");
    std::ofstream("sandbox/a/b/file1.txt").close();

    fs::path startingPoint = fs::path("symlink_to_sandbox");

    fs::create_symlink("sandbox", startingPoint);

    auto callbacks = std::make_shared<StrictMock<MockCallbacks>>();

    EXPECT_CALL(*callbacks, includeDirectory(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*callbacks, userDefinedExclusionCheck(_,_)).WillOnce(Return(false));
    EXPECT_CALL(*callbacks, processFile(fs::path("symlink_to_sandbox/a/b/file1.txt"), true)).WillOnce(Return());

    filewalker::FileWalker fw(*callbacks);
    fw.walk(startingPoint);
}

TEST_F(TestFileWalker, startWithDirectoryViaSymlink)
{
    fs::create_directories("sandbox/a/b/d/e");
    std::ofstream("sandbox/a/b/file1.txt").close();

    fs::create_symlink("sandbox", "symlink_to_sandbox");
    fs::path startingPoint = fs::path("symlink_to_sandbox/a");

    auto callbacks = std::make_shared<StrictMock<MockCallbacks>>();

    EXPECT_CALL(*callbacks, includeDirectory(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*callbacks, userDefinedExclusionCheck(_,_)).WillOnce(Return(false));
    EXPECT_CALL(*callbacks, processFile(fs::path("symlink_to_sandbox/a/b/file1.txt"), false)).WillOnce(Return());

    filewalker::FileWalker fw(*callbacks);
    fw.walk(startingPoint);
}

TEST_F(TestFileWalker, startWithSymlinkToFile)
{
    fs::create_directories("sandbox/a/b/d/e");
    std::ofstream("sandbox/a/b/file1.txt").close();

    fs::path startingPoint = fs::path("symlink_to_file");

    fs::create_symlink("sandbox/a/b/file1.txt", startingPoint);

    auto callbacks = std::make_shared<StrictMock<MockCallbacks>>();

    EXPECT_CALL(*callbacks, includeDirectory(_)).Times(0);
    EXPECT_CALL(*callbacks, userDefinedExclusionCheck(_,_)).Times(0);
    EXPECT_CALL(*callbacks, processFile(fs::path("symlink_to_file"), true)).WillOnce(Return());

    filewalker::FileWalker fw(*callbacks);
    fw.walk(startingPoint);
}

TEST_F(TestFileWalker, startWithBrokenSymlink)
{
    fs::create_directories("sandbox/a/b/d/e");
    std::ofstream("sandbox/a/b/file1.txt").close();

    fs::path startingPoint = fs::path("sandbox/broken_symlink");

    fs::create_symlink("missing", startingPoint);

    auto callbacks = std::make_shared<StrictMock<MockCallbacks>>();

    EXPECT_CALL(*callbacks, includeDirectory(_)).Times(0);
    EXPECT_CALL(*callbacks, userDefinedExclusionCheck(_,_)).Times(0);
    EXPECT_CALL(*callbacks, processFile(_, _)).Times(0);

    try
    {
        filewalker::FileWalker fw(*callbacks);
        fw.walk(startingPoint);
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

TEST_F(TestFileWalker, startWithBrokenSymlinkPath)
{
    fs::create_directories("sandbox/a/b/d/e");
    std::ofstream("sandbox/a/b/file1.txt").close();

    fs::path startingPoint = fs::path("sandbox/broken_symlink/start");

    fs::create_symlink("missing", startingPoint.parent_path());

    auto callbacks = std::make_shared<StrictMock<MockCallbacks>>();

    EXPECT_CALL(*callbacks, includeDirectory(_)).Times(0);
    EXPECT_CALL(*callbacks, userDefinedExclusionCheck(_,_)).Times(0);
    EXPECT_CALL(*callbacks, processFile(_, _)).Times(0);

    try
    {
        filewalker::FileWalker fw(*callbacks);
        fw.walk(startingPoint);
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

TEST_F(TestFileWalker, symlinksInWalkNoFollow)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    fs::create_directories("sandbox/a/b/d/e");
    std::ofstream("sandbox/a/b/file1.txt").close();

    fs::path startingPoint = fs::path("sandbox");

    fs::create_directories("other_dir");
    std::ofstream("other_dir/file2.txt").close();
    fs::create_symlink("../other_dir", "sandbox/other_dir");
    fs::create_symlink("../other_dir/file2.txt", "sandbox/file2.txt");
    fs::create_symlink("absent", "sandbox/broken_symlink");

    fs::create_directories("other_dir2");
    std::ofstream("other_dir2/file3.txt").close();
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

TEST_F(TestFileWalker, followSymlinksInWalk)
{
    fs::create_directories("sandbox/a/b/d/e");
    std::ofstream("sandbox/a/b/file1.txt").close();

    fs::path startingPoint = fs::path("sandbox");

    fs::create_directories("other_dir");
    std::ofstream("other_dir/file2.txt").close();
    fs::create_symlink("../other_dir", "sandbox/other_dir");
    fs::create_symlink("../other_dir/file2.txt", "sandbox/file2.txt");
    fs::create_symlink("absent", "sandbox/broken_symlink");

    fs::create_directories("other_dir2");
    std::ofstream("other_dir2/file3.txt").close();
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

TEST_F(TestFileWalker, duplicateSymlinksInWalkNoFollow)
{
    fs::create_directories("sandbox");
    fs::path startingPoint = fs::path("sandbox");

    fs::create_directories("other_dir");
    std::ofstream("other_dir/file2.txt").close();
    fs::create_symlink("../other_dir", "sandbox/link1");
    fs::create_symlink("../other_dir", "sandbox/link2");

    auto callbacks = std::make_shared<StrictMock<MockCallbacks>>();

    EXPECT_CALL(*callbacks, includeDirectory(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*callbacks, userDefinedExclusionCheck(_,_)).WillOnce(Return(false));
    EXPECT_CALL(*callbacks, processFile(_, _)).Times(0);

    filewalker::FileWalker fw(*callbacks);
    fw.walk(startingPoint);
}

TEST_F(TestFileWalker, duplicateDirSymlinksInWalk)
{
    fs::create_directories("sandbox");
    fs::path startingPoint = fs::path("sandbox");

    fs::create_directories("other_dir");
    std::ofstream("other_dir/file2.txt").close();
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

TEST_F(TestFileWalker, scanDirectoryAndSymlinkToDirectory)
{
    fs::create_directories("sandbox/a/b");
    fs::create_directories("sandbox/c/d");
    std::ofstream("sandbox/a/b/file1.txt").close();
    std::ofstream("sandbox/c/d/file2.txt").close();
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

TEST_F(TestFileWalker, symlinksToStartingDir)
{
    fs::create_directories("sandbox");
    std::ofstream("sandbox/file1.txt").close();
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

TEST_F(TestFileWalker, duplicateFileSymlinksInWalk)
{
    fs::create_directories("sandbox");
    fs::path startingPoint = fs::path("sandbox");

    fs::create_directories("other_dir");
    std::ofstream("other_dir/file1.txt").close();
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

TEST_F(TestFileWalker, backtrackProtectionAppliesAcrossMultipleWalks)
{
    fs::create_directories("sandbox/dir1");
    fs::create_directories("sandbox/dir2");

    fs::create_directories("other_dir");
    std::ofstream("other_dir/file.txt").close();
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


TEST_F(TestFileWalker, withStayOnDevice)
{
    fs::create_directories("sandbox/a/b/d/e");
    std::ofstream("sandbox/a/b/file1.txt").close();

    fs::path startingPoint = fs::path("sandbox");

    auto callbacks = std::make_shared<StrictMock<MockCallbacks>>();

    EXPECT_CALL(*callbacks, includeDirectory(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*callbacks, userDefinedExclusionCheck(_,_)).WillOnce(Return(false));
    EXPECT_CALL(*callbacks, processFile(fs::path("sandbox/a/b/file1.txt"), false)).WillOnce(Return());

    filewalker::FileWalker fw(*callbacks);
    fw.stayOnDevice();
    fw.walk(startingPoint);
}

TEST_F(TestFileWalker, scanContinuesAfterIncludeDirectoryThrows)
{
    fs::create_directories("sandbox/a/b");
    fs::create_directories("sandbox/c/d");

    fs::path startingPoint = fs::path("sandbox");

    auto callbacks = std::make_shared<StrictMock<MockCallbacks>>();

    EXPECT_CALL(*callbacks, userDefinedExclusionCheck(_, _)).WillOnce(Return(false));

    InSequence seq;

    EXPECT_CALL(*callbacks, includeDirectory(_)).WillOnce(Return(true));
    fs::filesystem_error ex ("fs error", std::error_code(0, std::system_category()));
    EXPECT_CALL(*callbacks, includeDirectory(_)).WillOnce(Throw(ex));
    EXPECT_CALL(*callbacks, registerError(_, _)).Times(1);
    EXPECT_CALL(*callbacks, includeDirectory(_)).Times(AtLeast(2)).WillRepeatedly(Return(true));


    filewalker::FileWalker fw(*callbacks);
    fw.walk(startingPoint);
}

TEST_F(TestFileWalker, scanContinuesAfterIncludeDirectoryThrowsTwice)
{
    fs::create_directories("sandbox/a/b");
    fs::create_directories("sandbox/c/d");

    fs::path startingPoint = fs::path("sandbox");

    auto callbacks = std::make_shared<StrictMock<MockCallbacks>>();

    EXPECT_CALL(*callbacks, userDefinedExclusionCheck(_, _)).WillOnce(Return(false));

    // called three times: sandbox, b, d
    EXPECT_CALL(*callbacks, includeDirectory(_)).Times(3).WillRepeatedly(Return(true));

    fs::filesystem_error ex ("fs error", std::error_code(0, std::system_category()));
    EXPECT_CALL(*callbacks, includeDirectory(fs::path("sandbox/a"))).WillOnce(Throw(ex));
    EXPECT_CALL(*callbacks, includeDirectory(fs::path("sandbox/c"))).WillOnce(Throw(ex));
    EXPECT_CALL(*callbacks, registerError(_, _)).Times(1);

    filewalker::FileWalker fw(*callbacks);
    fw.walk(startingPoint);
}

TEST_F(TestFileWalker, symlinkNoPermission)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    fs::create_directories("target");
    ASSERT_FALSE(fs::exists("link"));
    fs::create_directories("link");
    std::ofstream("target/file").close();
    ASSERT_FALSE(fs::exists("link/symlink"));
    fs::create_symlink("../target/file", "link/symlink");
    fs::permissions("target/file", fs::perms::none);
    fs::permissions("target", fs::perms::none);

    auto callbacks = std::make_shared<StrictMock<MockCallbacks>>();
    EXPECT_CALL(*callbacks, includeDirectory(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*callbacks, userDefinedExclusionCheck(_,_)).WillOnce(Return(false));

    filewalker::FileWalker fw(*callbacks);
    auto startingPoint = fs::path("link");
    fw.walk(startingPoint);

    EXPECT_FALSE(appenderContains("Failed to get the status of"));
}

TEST_F(TestFileWalker, missingStartPoint)
{
    fs::path startingPoint = fs::path("sandbox/missing");

    auto callbacks = std::make_shared<StrictMock<MockCallbacks>>();

    try
    {
        filewalker::FileWalker fw(*callbacks);
        fw.walk(startingPoint);

        FAIL() << "walk() didn't throw";
    }
    catch (fs::filesystem_error& e)
    {
        EXPECT_EQ(
            e.what(),
            std::string("filesystem error: Failed to scan "
                        "\"sandbox/missing\": file/folder does not exist: No such file or directory"));
        EXPECT_EQ(e.code().value(), ENOENT);
    }
}

TEST_F(TestFileWalker, missingStartPointNoError)
{
    fs::path startingPoint = fs::path("sandbox/missing");

    auto callbacks = std::make_shared<StrictMock<MockCallbacks>>();
    EXPECT_CALL(*callbacks, registerError(_, _)).Times(1);

    filewalker::FileWalker fw(*callbacks);
    fw.abortOnMissingStartingPoint(false);

    EXPECT_NO_THROW(fw.walk(startingPoint));
}

TEST_F(TestFileWalker, pathTooLong)
{
    std::string longPathStr(4097, 'a');
    fs::path startingPoint = fs::path(longPathStr);

    auto callbacks = std::make_shared<StrictMock<MockCallbacks>>();

    try
    {
        filewalker::FileWalker fw(*callbacks);
        fw.walk(startingPoint);
        FAIL() << "walk() didn't throw";
    }
    catch (fs::filesystem_error& e)
    {
        EXPECT_EQ(
            e.what(),
            std::string("filesystem error: Failed to start scan: "
                        "Starting Path too long: File name too long"));
        EXPECT_EQ(e.code().value(), ENAMETOOLONG);
    }
}