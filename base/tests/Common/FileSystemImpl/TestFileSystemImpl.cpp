// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "Common/FileSystem/IFileNotFoundException.h"
#include "Common/FileSystem/IFilePermissions.h"
#include "Common/FileSystem/IFileSystemException.h"
#include "Common/FileSystem/IFileTooLargeException.h"
#include "Common/FileSystem/IPermissionDeniedException.h"
#include "Common/FileSystemImpl/FileSystemImpl.h"
#include "Common/SslImpl/Digest.h"
#include "tests/Common/Helpers/FileSystemReplaceAndRestore.h"
#include "tests/Common/Helpers/MockFileSystem.h"
#include "tests/Common/Helpers/TempDir.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <chrono>
#include <fcntl.h>
#include <thread>
#include <unistd.h>

using namespace Common::FileSystem;
namespace
{
    void removeFile(const std::string& path)
    {
        int ret = ::remove(path.c_str());
        if (ret == -1 && errno == ENOENT)
        {
            return;
        }
        ASSERT_EQ(ret, 0);
    }

    void makedir(const std::string& path, mode_t mode)
    {
        int ret = ::mkdir(path.c_str(), mode);
        if (ret == -1 && errno == EEXIST)
        {
            return;
        }
        ASSERT_EQ(ret, 0);
    }

    class FileSystemImplTest : public ::testing::Test
    {
    public:
        ~FileSystemImplTest() {}
        std::unique_ptr<IFileSystem> m_fileSystem;
        void SetUp() override
        {
           m_fileSystem = std::make_unique<FileSystemImpl>();
        }

        void copyFileAndExpectThrow(const Path& src, const Path& dest, const std::string& message)
        {
            try
            {
                m_fileSystem->copyFile(src, dest);
                FAIL();
            }
            catch (const IFileSystemException& ex)
            {
                EXPECT_EQ(ex.what(), message);
            }
        }};

    // cppcheck-suppress syntaxError
    TEST_F(FileSystemImplTest, basenameReturnsCorrectMatchingValue)
    {
        std::vector<std::pair<std::string, std::string>> values = { { "/tmp/tmpfile.txt", "tmpfile.txt" },
                                                                    { "/tmp/tmpfile/", "" },
                                                                    { "/tmp/tmp1/tmp2/tmp3/tmpfile.txt",
                                                                      "tmpfile.txt" },
                                                                    { "/tmp", "tmp" },
                                                                    { "tmp", "tmp" },
                                                                    { "appid-25_policy.xml", "appid-25_policy.xml" },
                                                                    { "", "" } };
        for (auto& pair : values)
        {
            EXPECT_EQ(Common::FileSystem::basename(pair.first), pair.second);
        }
    }

    TEST_F(FileSystemImplTest, joinReturnsCorrectValueWithNoSeporator)
    {
        std::string path1("/tmp");
        std::string path2("tempfile.txt");
        std::string expectedValue("/tmp/tempfile.txt");
        EXPECT_EQ(Common::FileSystem::join(path1, path2), expectedValue);
    }

    TEST_F(FileSystemImplTest, joinReturnsCorrectValueWithPath1TrailingSeporator)
    {
        std::string path1("/tmp/");
        std::string path2("tempfile.txt");
        std::string expectedValue("/tmp/tempfile.txt");
        EXPECT_EQ(Common::FileSystem::join(path1, path2), expectedValue);
    }

    TEST_F(FileSystemImplTest, joinReturnsCorrectValueWithPath2StartingWithSeporator)
    {
        // python -c 'import os; print os.path.join("/foo","/bar")' -> "/bar

        std::string path1("/tmp");
        std::string path2("/tempfile.txt");
        std::string expectedValue("/tempfile.txt");
        EXPECT_EQ(Common::FileSystem::join(path1, path2), expectedValue);
    }

    TEST_F(FileSystemImplTest, joinReturnsCorrectValueWithPath1TrailingSeporatorAndPath2StartingWithSeporator)
    {
        // python -c 'import os; print os.path.join("/tmp/","/tempfile.txt")' -> /tempfile.txt
        std::string path1("/tmp/");
        std::string path2("/tempfile.txt");
        std::string expectedValue("/tempfile.txt");
        EXPECT_EQ(Common::FileSystem::join(path1, path2), expectedValue);
    }

    TEST_F(FileSystemImplTest, joinWithDotSlashReturnsExpectedPath)
    {
        std::string path1("/tmp/");
        std::string path2("./tempfile.txt");
        std::string expectedValue("/tmp/tempfile.txt");
        EXPECT_EQ(Common::FileSystem::join(path1, path2), expectedValue);
    }

    TEST_F(FileSystemImplTest, joinWithDotSlashSlashReturnsExpectedPath)
    {
        // python -c 'import os; print os.path.join("/tmp/",".//tempfile.txt")' -> /tmp/.//tempfile.txt
        // python doesn't have any special handling for ./ in join
        // python -c 'import os; print os.path.normpath(os.path.join("/tmp/",".//tempfile.txt"))' -> /tmp/tempfile.txt

        std::string path1("/tmp/");
        std::string path2(".//tempfile.txt");
        std::string expectedValue("/tmp/tempfile.txt");
        EXPECT_EQ(Common::FileSystem::join(path1, path2), expectedValue);
    }

    TEST_F(FileSystemImplTest, joinWithEmptyStringsReturnsExpectedPath)
    {
        // python -c 'import os; print os.path.join("","")' -> ""
        std::string path1;
        std::string path2;
        std::string expectedValue;
        EXPECT_EQ(Common::FileSystem::join(path1, path2), expectedValue);
    }

    TEST_F(FileSystemImplTest, joinWithEmptyPath1AndAbsolutePath2ReturnsPath2)
    {
        Path path1;
        Path path2("/foo/bar");
        Path& expectedValue = path2;
        EXPECT_EQ(Common::FileSystem::join(path1, path2), expectedValue);
    }

    TEST_F(FileSystemImplTest, joinWithEmptyPath1AndRelativePath2ReturnsExpectedPath)
    {
        // python -c 'import os; print os.path.join("","foo/bar")' -> foo/bar
        Path path1;
        Path path2("foo/bar");
        Path expectedValue("foo/bar");
        EXPECT_EQ(Common::FileSystem::join(path1, path2), expectedValue);
    }

    TEST_F(FileSystemImplTest, joinWithEmptyPath2ReturnsExpectedPath)
    {
        // python -c 'import os; print os.path.join("foo/bar","")' -> foo/bar
        Path path1("foo/bar");
        Path path2;
        Path expectedValue("foo/bar");
        EXPECT_EQ(Common::FileSystem::join(path1, path2), expectedValue);
    }

    TEST_F(FileSystemImplTest, joinWithTwoRelativePathsReturnsARelativePath)
    {
        // python -c 'import os; print os.path.join("foo","bar")'
        Path path1("foo");
        Path path2("bar");
        Path expectedValue("foo/bar");
        EXPECT_EQ(Common::FileSystem::join(path1, path2), expectedValue);
    }

    TEST_F(FileSystemImplTest, existReturnsTrueWhenFileExists)
    {
        EXPECT_TRUE(m_fileSystem->exists("/etc/passwd"));
    }

    TEST_F(FileSystemImplTest, existReturnsFalseWhenFileOrDirectoryDoesNotExist)
    {
        EXPECT_FALSE(m_fileSystem->exists("/etc/ghdfoijsdfgjhszdgvzjsdjvcjsdhzc"));
    }

    TEST_F(FileSystemImplTest, existReturnsTrueWhenPathGivenIsDirectory)
    {
        EXPECT_TRUE(m_fileSystem->exists("/etc"));
    }

    TEST_F(FileSystemImplTest, isFileReturnsTrueWhenPathGivenPointsToAFile)
    {
        EXPECT_TRUE(m_fileSystem->isFile("/etc/passwd"));
    }

    TEST_F(FileSystemImplTest, isFileReturnsFalseWhenPathGivenPointsToADirectory)
    {
        EXPECT_FALSE(m_fileSystem->isFile("/etc"));
    }

    TEST_F(FileSystemImplTest, isFileReturnsTrueWhenPathGivenPointsToASymLink)
    {
        EXPECT_TRUE(m_fileSystem->isFile("/proc/self/exe"));
    }

    TEST_F(FileSystemImplTest, isFileReturnsFalseWhenPathGivenPointsToASocket)
    {
        EXPECT_FALSE(m_fileSystem->isFile("/run/systemd/private"));
    }

    TEST_F(FileSystemImplTest, isDirectoryReturnsFalseWhenPathGivenPointsToAFile)
    {
        EXPECT_FALSE(m_fileSystem->isDirectory("/etc/passwd"));
    }

    TEST_F(FileSystemImplTest, isDirectoryReturnsTrueWhenRelativePathGivenPointsToCurrentDirectory)
    {
        EXPECT_TRUE(m_fileSystem->isDirectory("."));
    }

    TEST_F(FileSystemImplTest, isDirectoryReturnsFalseWhenDirectoryDoesNotExist)
    {
        EXPECT_FALSE(m_fileSystem->isDirectory("/etc/ghdfoijsdfgjhszdgvzjsdjvcjsdhzc"));
    }

    TEST_F(FileSystemImplTest, isDirectoryReturnsTrueWhenPathGivenIsDirectory)
    {
        EXPECT_TRUE(m_fileSystem->isDirectory("/etc"));
    }

    TEST_F(FileSystemImplTest, isFileOrDirectoryReturnsTrueWhenPathGivenPointsToAFile)
    {
        EXPECT_TRUE(m_fileSystem->isFileOrDirectory("/etc/passwd"));
    }

    TEST_F(FileSystemImplTest, isFileOrDirectoryReturnsTrueWhenPathGivenPointsToASymLink)
    {
        EXPECT_TRUE(m_fileSystem->isFileOrDirectory("/proc/self/exe"));
    }

    TEST_F(FileSystemImplTest, isFileOrDirectoryReturnsFalseWhenPathGivenPointsToASocket)
    {
        EXPECT_FALSE(m_fileSystem->isFileOrDirectory("/run/systemd/private"));
    }

    TEST_F(FileSystemImplTest, isFileOrDirectoryReturnsTrueWhenRelativePathGivenPointsToCurrentDirectory)
    {
        EXPECT_TRUE(m_fileSystem->isFileOrDirectory("."));
    }

    TEST_F(FileSystemImplTest, isFileOrDirectoryReturnsFalseWhenFileDoesNotExist)
    {
        EXPECT_FALSE(m_fileSystem->isFileOrDirectory("/etc/ghdfoijsdfgjhszdgvzjsdjvcjsdhzc"));
    }

    TEST_F(FileSystemImplTest, isFileOrDirectoryReturnsTrueWhenPathGivenIsDirectory)
    {
        EXPECT_TRUE(m_fileSystem->isFileOrDirectory("/etc"));
    }

    TEST_F(FileSystemImplTest, isSymlinkReturnsTrueForSymlink)
    {
        EXPECT_TRUE(m_fileSystem->isSymlink("/proc/self/exe"));
    }

    TEST_F(FileSystemImplTest, currentWorkingDirectoryReturnsCorrectValue)
    {
        std::string startingDirectory = m_fileSystem->currentWorkingDirectory();

        std::string expectedPath("/tmp");
        int ret = chdir(expectedPath.c_str());
        ASSERT_EQ(ret, 0);
        EXPECT_EQ(m_fileSystem->currentWorkingDirectory(), expectedPath);

        ret = chdir(startingDirectory.c_str());
        ASSERT_EQ(ret, 0);

        EXPECT_EQ(m_fileSystem->currentWorkingDirectory(), startingDirectory);
    }

    TEST_F(FileSystemImplTest, readWriteStoresAndReadsExpectedContentFromFile)
    {
        std::string filePath =
            Common::FileSystem::join(m_fileSystem->currentWorkingDirectory(), "ReadWriteFileTest.txt");

        std::string testContent("HelloWorld");

        m_fileSystem->writeFile(filePath, testContent);

        EXPECT_EQ(m_fileSystem->readFile(filePath), testContent);
        removeFile(filePath);
    }

    TEST_F(FileSystemImplTest, writeOverwritesExistingFile)
    {
        std::string filePath = Common::FileSystem::join(m_fileSystem->currentWorkingDirectory(), "overwriteme.txt");

        std::string initialContent("initial");
        m_fileSystem->writeFile(filePath, initialContent);
        EXPECT_EQ(m_fileSystem->readFile(filePath), initialContent);

        std::string finalContent("final");
        m_fileSystem->writeFile(filePath, finalContent);

        EXPECT_EQ(m_fileSystem->readFile(filePath), finalContent);
        removeFile(filePath);
    }

    TEST_F(FileSystemImplTest, readThrowsForTooLargeFile)
    {
        std::string filePath =
            Common::FileSystem::join(m_fileSystem->currentWorkingDirectory(), "readThrowsForTooLargeFile.txt");

        std::string testContent("HelloWorld");

        m_fileSystem->writeFile(filePath, testContent);

        EXPECT_THROW(m_fileSystem->readFile(filePath, 2), IFileTooLargeException);

        removeFile(filePath);
    }

    TEST_F(FileSystemImplTest, writeFileUsingDirectoryPathShouldThrow)
    {
        std::string directoryPath =
            Common::FileSystem::join(m_fileSystem->currentWorkingDirectory(), "WriteToDirectoryTest");

        makedir(directoryPath, 0700);

        std::string testContent("HelloWorld");

        EXPECT_THROW(m_fileSystem->writeFile(directoryPath, testContent), IFileSystemException);

        ::rmdir(directoryPath.c_str());
    }

    TEST_F(FileSystemImplTest, readFileThatDoesNotExistsShouldThrow)
    {
        std::string filePath = Common::FileSystem::join(m_fileSystem->currentWorkingDirectory(), "ReadFileTest.txt");
        EXPECT_THROW(m_fileSystem->readFile(filePath), IFileNotFoundException);
    }

    TEST_F(FileSystemImplTest, readFileUsingDirectoryPathShouldThrow)
    {
        std::string directoryPath =
            Common::FileSystem::join(m_fileSystem->currentWorkingDirectory(), "WriteToDirectoryTest");

        makedir(directoryPath, 0700);
        EXPECT_THROW(m_fileSystem->readFile(directoryPath), IFileSystemException);

        ::rmdir(directoryPath.c_str());
    }

    TEST_F(FileSystemImplTest, readFileWhenFileDoesNotHaveReadPermissionsThrows)
    {
        if (::getuid() == 0)
        {
            // Disabled because this test fails in CI (probably is being run as root)
            return;
        }

        std::string filePath =
            Common::FileSystem::join(m_fileSystem->currentWorkingDirectory(), "ReadWriteFileTest.txt");

        std::string testContent("HelloWorld");

        m_fileSystem->writeFile(filePath, testContent);
        ::chmod(filePath.c_str(), 0200);

        EXPECT_THROW(m_fileSystem->readFile(filePath), IPermissionDeniedException);
        removeFile(filePath);
    }

    TEST_F(FileSystemImplTest, readLinesGetsExpectedContentFromFile)
    {
        std::string filePath = Common::FileSystem::join(m_fileSystem->currentWorkingDirectory(), "ReadLinesTest.txt");

        std::string testContent("HelloWorld\nHelloWorld2");

        m_fileSystem->writeFile(filePath, testContent);
        std::vector<std::string> content{ "HelloWorld", "HelloWorld2" };
        EXPECT_EQ(m_fileSystem->readLines(filePath), content);
        removeFile(filePath);
    }

    TEST_F(FileSystemImplTest, readLinesGetsExpectedContentFromEmptyFile)
    {
        std::string filePath = Common::FileSystem::join(m_fileSystem->currentWorkingDirectory(), "EmptyFileTest.txt");

        m_fileSystem->writeFile(filePath, "");
        std::vector<std::string> content;
        EXPECT_EQ(m_fileSystem->readLines(filePath), content);
        removeFile(filePath);
    }

    TEST_F(FileSystemImplTest, readLinesForFileThatDoesNotExistsShouldThrow)
    {
        std::string filePath = Common::FileSystem::join(m_fileSystem->currentWorkingDirectory(), "ReadFileTest.txt");
        EXPECT_THROW(m_fileSystem->readLines(filePath), IFileSystemException);
    }

    TEST_F(FileSystemImplTest, readLinesOnDirectoryPathShouldThrow)
    {
        std::string directoryPath =
            Common::FileSystem::join(m_fileSystem->currentWorkingDirectory(), "WriteToDirectoryTest");

        makedir(directoryPath, 0700);
        EXPECT_THROW(m_fileSystem->readLines(directoryPath), IFileSystemException);

        ::rmdir(directoryPath.c_str());
    }

    TEST_F( FileSystemImplTest, openFileStreamWorks)
    {
        Tests::TempDir tempDir;
        tempDir.makeDirs("Root/");
        tempDir.createFile("Root/file","hello\nworld");
        auto file = m_fileSystem->openFileForRead(tempDir.absPath("Root/file"));
        std::string line;
        std::getline(*file, line);
        EXPECT_EQ("hello",line);
        std::getline(*file, line);
        EXPECT_EQ("world",line);
    }

    TEST_F( FileSystemImplTest, openFileStreamFileDoesNotexist)
    {
        EXPECT_THROW(m_fileSystem->openFileForRead("Root/file"), IFileSystemException);
    }

    TEST_F( FileSystemImplTest, openFileStreamFileWhenFileIsADir)
    {
        Tests::TempDir tempDir;
        tempDir.makeDirs("Root/");
        EXPECT_THROW(m_fileSystem->openFileForRead(tempDir.absPath("Root")), IFileSystemException);
    }

    TEST_F(FileSystemImplTest, atomicWriteStoresExpectedContentForFile)
    {
        std::string filePath = Common::FileSystem::join(m_fileSystem->currentWorkingDirectory(), "AtomicWrite.txt");

        std::string testContent("HelloWorld");

        m_fileSystem->writeFileAtomically(filePath, testContent, m_fileSystem->currentWorkingDirectory());

        EXPECT_EQ(m_fileSystem->readFile(filePath), testContent);
        removeFile(filePath);
    }

    TEST_F(FileSystemImplTest, dirNameReturnsCorrectMatchingValue)
    {
        std::vector<std::pair<std::string, std::string>> values = { { "/tmp/tmpfile.txt", "/tmp" },
                                                                    { "/tmp/tmpfile/", "/tmp" },
                                                                    { "/tmp/tmp1/tmp2/tmp3/tmpfile.txt",
                                                                      "/tmp/tmp1/tmp2/tmp3" },
                                                                    { "/tmp", "" },
                                                                    { "tmp", "" },
                                                                    { "", "" } };
        for (auto& pair : values)
        {
            EXPECT_EQ(Common::FileSystem::dirName(pair.first), pair.second);
        }
    }

    TEST_F(FileSystemImplTest, subdirNameFromPathReturnsCorrectMatchingValue)
    {
        std::vector<std::pair<std::string, std::string>> values = { { "/tmp/tmpfile.txt", "tmpfile.txt" },
                                                                    { "/tmp/tmpfile/", "tmpfile" },
                                                                    { "/tmp/tmp1/tmp2/tmp3/tmpfile.txt",
                                                                      "tmpfile.txt" },
                                                                    { "/tmp", "tmp" },
                                                                    { "tmp", "tmp" },
                                                                    { "", "" } };
        for (auto& pair : values)
        {
            EXPECT_EQ(Common::FileSystem::subdirNameFromPath(pair.first), pair.second);
        }
    }
    TEST_F(FileSystemImplTest, isExecutableReturnTrueForExecutables)
    {
        EXPECT_TRUE(m_fileSystem->isExecutable("/bin/bash"));
    }

    TEST_F(FileSystemImplTest, isExecutableReturnFalseForNonExecutables)
    {
        EXPECT_FALSE(m_fileSystem->isExecutable("/etc/passwd"));
    }

    TEST_F(FileSystemImplTest, isExecutableReturnFalseForNonExistingFiles)
    {
        EXPECT_FALSE(m_fileSystem->isExecutable("/tmp/thisfiledoesnotexist"));
    }

    TEST_F(FileSystemImplTest, listFilesReturnsAListOfPathsOnlyContainingRegularFiles)
    {
        Tests::TempDir tempDir;
        tempDir.makeDirs("Root/subdir");
        tempDir.createFile("Root/file1", "hello");
        tempDir.createFile("Root/file2", "hello");

        std::vector<std::string> fileList = m_fileSystem->listFiles(tempDir.absPath("Root"));

        EXPECT_EQ(fileList.size(), 2);

        std::sort(fileList.begin(), fileList.end());

        EXPECT_EQ(fileList[0], tempDir.absPath("Root/file1"));
        EXPECT_EQ(fileList[1], tempDir.absPath("Root/file2"));
    }

    TEST_F(
        FileSystemImplTest,
        listFilesAndDirectoriesReturnsAListOfPathsOnlyContainingRegularFilesAndDirectories)
    {
        Tests::TempDir tempDir;
        tempDir.makeDirs("Root/subdir");
        tempDir.createFile("Root/file1", "hello");
        tempDir.createFile("Root/file2", "hello");
        int ret = ::symlink("/etc/hosts", tempDir.absPath("Root/symlink").c_str());
        ASSERT_EQ(ret, 0);

        std::vector<std::string> fileList = m_fileSystem->listFilesAndDirectories(tempDir.absPath("Root"));

        EXPECT_EQ(fileList.size(), 3);

        std::sort(fileList.begin(), fileList.end());

        EXPECT_EQ(fileList[0], tempDir.absPath("Root/file1"));
        EXPECT_EQ(fileList[1], tempDir.absPath("Root/file2"));
        EXPECT_EQ(fileList[2], tempDir.absPath("Root/subdir"));
    }

    TEST_F(FileSystemImplTest, listDirectoriesReturnsAListOfDirectories)
    {
        Tests::TempDir tempDir;
        tempDir.makeDirs("Root/subdir1");
        tempDir.makeDirs("Root/subdir2");
        tempDir.makeDirs("Root/blah/willNotBeSeen");
        tempDir.createFile("Root/file123", "hello123");
        tempDir.createFile("Root/file321", "hello321");
        int ret = ::symlink("/etc/hosts", tempDir.absPath("Root/symlink").c_str());
        ASSERT_EQ(ret, 0);

        std::vector<std::string> fileList = m_fileSystem->listDirectories(tempDir.absPath("Root"));

        EXPECT_EQ(fileList.size(), 3);

        std::sort(fileList.begin(), fileList.end());

        EXPECT_EQ(fileList[0], tempDir.absPath("Root/blah"));
        EXPECT_EQ(fileList[1], tempDir.absPath("Root/subdir1"));
        EXPECT_EQ(fileList[2], tempDir.absPath("Root/subdir2"));
    }

    TEST_F(FileSystemImplTest, canCreateDirectories)
    {
        Tests::TempDir tempdir("", "FileSystemImplTest_canCreateDirectories");
        Path A = tempdir.absPath("A");
        Path B = Common::FileSystem::join(A, "B");
        m_fileSystem->makedirs(B);
        EXPECT_TRUE(m_fileSystem->isDirectory(A));
        EXPECT_TRUE(m_fileSystem->isDirectory(B));
    }

    TEST_F(FileSystemImplTest, copyFile)
    {
        Tests::TempDir tempdir("", "FileSystemImplTest_copyFile");
        Path A = tempdir.absPath("A");
        Path B = tempdir.absPath("B");
        tempdir.createFile("A", "FOOBAR");
        EXPECT_NO_THROW(m_fileSystem->copyFile(A, B));
        EXPECT_TRUE(m_fileSystem->exists(B));
        std::string content = m_fileSystem->readFile(B);
        EXPECT_EQ(content, "FOOBAR");
    }

    TEST_F(FileSystemImplTest, copyLargeFile)
    {
        Tests::TempDir tempdir("", "FileSystemImplTest_copyLargeFile");
        Path A = tempdir.absPath("A");
        Path B = tempdir.absPath("B");
        const int SIZE = 100000; // file size
        std::ostringstream large_string_stream;
        for (int i = 0; i < SIZE / 10; ++i)
        {
            large_string_stream << "0123456789";
        }
        std::string expected_contents = large_string_stream.str();
        tempdir.createFile("A", expected_contents);
        EXPECT_NO_THROW(m_fileSystem->copyFile(A, B));
        EXPECT_TRUE(m_fileSystem->exists(B));
        std::string content = m_fileSystem->readFile(B);
        EXPECT_EQ(content, expected_contents);
    }

    TEST_F(FileSystemImplTest, copyFileDoesNotExist)
    {
        Tests::TempDir tempdir("", "FileSystemImplTest_copyFile");
        Path A = tempdir.absPath("A");
        Path B = tempdir.absPath("B");
        copyFileAndExpectThrow(A, B, "Failed to copy file: '" + A + "' to '" + B + "', source file does not exist.");
        ;
        EXPECT_FALSE(m_fileSystem->exists(B));
    }

    TEST_F(FileSystemImplTest, copyFileWrongPermissions)
    {
        Tests::TempDir tempdir("", "FileSystemImplTest_copyFile");
        Path A = tempdir.absPath("A");
        Path B = tempdir.absPath("B");
        auto mockFileSystem = std::make_unique<StrictMock<MockFileSystem>>();
        // With wrong permissions file will exist but not open
        EXPECT_CALL(*mockFileSystem, exists(A)).WillOnce(Return(true));
        Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockFileSystem));
        copyFileAndExpectThrow(A, B, "Failed to copy file: '" + A + "' to '" + B + "', reading file failed.");
        EXPECT_FALSE(m_fileSystem->exists(B));
    }

    TEST_F(FileSystemImplTest, copyFileNonAccessibleDest)
    {
        Tests::TempDir tempdir("", "FileSystemImplTest_copyFile");
        Path A = tempdir.absPath("A");
        Path B = Common::FileSystem::join("NotADirectory", "NotAFile");
        tempdir.createFile("A", "FOOBAR");
        copyFileAndExpectThrow(A, B, "Failed to copy file: '" + A + "' to '" + B + "', writing file failed.");
        EXPECT_FALSE(m_fileSystem->exists(B));
    }

    TEST_F(FileSystemImplTest, copyFileThrowsOnFailToCopyContent)
    {
        Tests::TempDir tempdir("", "FileSystemImplTest_copyFile");
        Path src = tempdir.absPath("A");
        Path dest = tempdir.absPath("B");
        tempdir.createFile("A", "FOOBAR");

        auto mockFileSystem = std::make_unique<StrictMock<MockFileSystem>>();
        EXPECT_CALL(*mockFileSystem, exists(src)).WillOnce(Return(true));
        EXPECT_CALL(*mockFileSystem, fileSize(src)).WillOnce(Return(1));
        EXPECT_CALL(*mockFileSystem, fileSize(dest)).WillOnce(Return(0));
        EXPECT_CALL(*mockFileSystem, removeFile(dest)).WillOnce(Return());
        Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockFileSystem));
        copyFileAndExpectThrow(
            src,
            dest,
            "Failed to copy file: '" + src + "' to '" + dest +
                "', contents failed to copy. Check space available on device.");
    }

    TEST_F(FileSystemImplTest, copyFileAndSetPermissions)
    {
        Tests::TempDir tempdir("", "FileSystemImplTest_copyFile");
        Path A = tempdir.absPath("A");
        Path B = tempdir.absPath("B");
        tempdir.createFile("A", "FOOBAR");
        mode_t newFilePermissions = S_IFREG | S_IRUSR | S_IWUSR | S_IRGRP;

        auto filePermissions = Common::FileSystem::filePermissions();
        std::string ownerName = filePermissions->getUserName(A);
        std::string groupName = filePermissions->getGroupName(A);

        EXPECT_FALSE(m_fileSystem->exists(B));
        EXPECT_NE(newFilePermissions, filePermissions->getFilePermissions(A));
        EXPECT_NO_THROW(
            m_fileSystem->copyFileAndSetPermissions(A, B, newFilePermissions, ownerName, groupName));
        EXPECT_TRUE(m_fileSystem->exists(B));

        std::string content = m_fileSystem->readFile(B);
        EXPECT_EQ(content, "FOOBAR");

        mode_t filePermissionsRead = filePermissions->getFilePermissions(B);
        EXPECT_EQ(newFilePermissions, filePermissionsRead);
    }

    TEST_F(FileSystemImplTest, copyFileOverwritesExistingFile)
    {
        Tests::TempDir tempdir("", "FileSystemImplTest_copyFile");
        Path A = tempdir.absPath("A");
        Path B = tempdir.absPath("B");
        tempdir.createFile("A", "FOOBAR-A");
        tempdir.createFile("B", "FOOBAR-B");
        EXPECT_NO_THROW(m_fileSystem->copyFile(A, B));
        EXPECT_TRUE(m_fileSystem->exists(B));
        std::string content = m_fileSystem->readFile(B);
        EXPECT_EQ(content, "FOOBAR-A");
    }

    TEST_F(FileSystemImplTest, removeFileDeletesFile)
    {
        std::string filePath =
            Common::FileSystem::join(m_fileSystem->currentWorkingDirectory(), "removeFileDeletesFile.txt");

        std::string testContent("HelloWorld");

        m_fileSystem->writeFile(filePath, testContent);

        EXPECT_TRUE(m_fileSystem->isFile(filePath));

        m_fileSystem->removeFile(filePath);

        EXPECT_FALSE(m_fileSystem->exists(filePath));
    }

    TEST_F(FileSystemImplTest, removeFileThrowsIfFileDoesNotExist)
    {
        std::string filePath =
            Common::FileSystem::join(m_fileSystem->currentWorkingDirectory(), "removeFileThrowsIfFileDoesNotExist.txt");
        ASSERT_FALSE(m_fileSystem->exists(filePath));
        EXPECT_THROW(m_fileSystem->removeFile(filePath), IFileSystemException);
    }

    TEST_F(FileSystemImplTest, removeDirectoryDeletesFile)
    {
        std::string filePath =
            Common::FileSystem::join(m_fileSystem->currentWorkingDirectory(), "removeDirectoryDeletesFile.txt");

        std::string testContent("HelloWorld");

        m_fileSystem->writeFile(filePath, testContent);

        EXPECT_TRUE(m_fileSystem->isFile(filePath));

        m_fileSystem->removeFileOrDirectory(filePath);

        EXPECT_FALSE(m_fileSystem->exists(filePath));
    }

    TEST_F(FileSystemImplTest, removeDirectoryDeletesEmptyDirectory)
    {
        std::string filePath =
            Common::FileSystem::join(m_fileSystem->currentWorkingDirectory(), "removeDirectoryDeletesEmptyDirectory");

        m_fileSystem->makedirs(filePath);

        EXPECT_TRUE(m_fileSystem->isDirectory(filePath));

        m_fileSystem->removeFileOrDirectory(filePath);

        EXPECT_FALSE(m_fileSystem->exists(filePath));
    }

    TEST_F(FileSystemImplTest, removeDirectoryDeletesNonEmptyDirectory)
    {
        std::string dirPath = Common::FileSystem::join(
            m_fileSystem->currentWorkingDirectory(), "removeDirectoryDeletesNonEmptyDirectory");
        std::string filePath = Common::FileSystem::join(dirPath, "testFile");

        m_fileSystem->makedirs(dirPath);
        std::string testContent("HelloWorld");
        m_fileSystem->writeFile(filePath, testContent);

        EXPECT_TRUE(m_fileSystem->isDirectory(dirPath));
        EXPECT_TRUE(m_fileSystem->isFile(filePath));

        m_fileSystem->removeFileOrDirectory(dirPath);

        EXPECT_FALSE(m_fileSystem->exists(dirPath));
    }

    TEST_F(FileSystemImplTest, removeFileOrDirectoryThrowsOnNonExistentDirectory)
    {
        std::string dirPath = Common::FileSystem::join(
            m_fileSystem->currentWorkingDirectory(), "removeFileOrDirectoryDoesNotThrowOnNonExistentDirectory");
        EXPECT_FALSE(m_fileSystem->exists(dirPath));
        EXPECT_THROW(m_fileSystem->removeFileOrDirectory(dirPath), std::exception);
    }

    TEST_F(FileSystemImplTest, makeAbsoluteReturnsArgumentWithArgumentIsAbsolute)
    {
        Path original = "/foo";
        Path result = Common::FileSystem::fileSystem()->makeAbsolute(original);
        EXPECT_EQ(original, result);
    }

    TEST_F(FileSystemImplTest, makeAbsoluteReturnsAbsolutePathWhenArgumentIsRelative)
    {
        Path original = "foo";
        Path result = Common::FileSystem::fileSystem()->makeAbsolute(original);
        EXPECT_EQ(result[0], '/');
    }

    TEST_F(FileSystemImplTest, readLinkReturnsLinkedToPathWhenCalledOnSymbolicLink)
    {
        std::optional<Path> findPath = m_fileSystem->readlink("/proc/self/exe");
        ASSERT_TRUE(findPath.has_value());
    }

    TEST_F(FileSystemImplTest, readLinkReturnsEmptyWhenCalledOnNonExistentFile)
    {
        std::optional<Path> findPath = m_fileSystem->readlink("/Not/a/file/path");
        ASSERT_FALSE(findPath.has_value());
    }

    TEST_F(FileSystemImplTest, readLinkReturnsEmptyWhenCalledOnNonLinkFile)
    {
        std::optional<Path> findPath = m_fileSystem->readlink("/bin/bash");
        ASSERT_FALSE(findPath.has_value());
    }

    TEST_F(FileSystemImplTest, readLinkConvertsRelativeSymlinkTargetToAbsolutePath)
    {
        Tests::TempDir tempDir;
        tempDir.makeDirs("Root/subdir1");
        tempDir.makeDirs("Root/subdir2");
        tempDir.createFile("Root/subdir2/file1", "hello");
        int ret = ::symlink("../subdir2/file1", tempDir.absPath("Root/subdir1/symlink").c_str());
        ASSERT_EQ(ret, 0);

        std::optional<Path> findPath = m_fileSystem->readlink(tempDir.absPath("Root/subdir1/symlink"));
        ASSERT_TRUE(findPath.has_value());
        EXPECT_EQ(findPath.value(), tempDir.absPath("Root/subdir1/../subdir2/file1").c_str());
    }

    TEST_F(FileSystemImplTest, lastModifiedTimeReturnsTimeOnDirectories)
    {
        std::time_t curTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        std::this_thread::sleep_for(std::chrono::seconds(1));
        Tests::TempDir tempDir;
        tempDir.makeDirs("Root/subdir");
        std::time_t time_created = m_fileSystem->lastModifiedTime(tempDir.absPath("Root/subdir"));

        ASSERT_GE(time_created, curTime);
    }

    TEST_F(FileSystemImplTest, lastModifiedTimeReturnsTimeOnFiles)
    {
        std::time_t curTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        std::this_thread::sleep_for(std::chrono::seconds(1));
        Tests::TempDir tempDir;
        tempDir.createFile("emptyFile", "");
        std::time_t time_created = m_fileSystem->lastModifiedTime(tempDir.absPath("emptyFile"));
        ASSERT_GE(time_created, curTime);
    }

    TEST_F(FileSystemImplTest, lastModifiedTimeReturnsTimeOnSymlinks)
    {
        std::time_t curTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        std::this_thread::sleep_for(std::chrono::seconds(1));
        Tests::TempDir tempDir;
        tempDir.createFile("emptyFile", "");
        int success = symlink(tempDir.absPath("emptyFile").c_str(), tempDir.absPath("symlink").c_str());
        ASSERT_EQ(success, 0);
        std::time_t time_created = m_fileSystem->lastModifiedTime(tempDir.absPath("symlink"));
        ASSERT_GE(time_created, curTime);
    }

    TEST_F(FileSystemImplTest, removeFilesInDirectoryRemovesFilesInDirectory)
    {
        std::vector<Path> filesInDirectory{ "file1", "file2", "file3" };

        Tests::TempDir tempDir;
        Path directoryPath = tempDir.dirPath();
        for (auto& filePath : filesInDirectory)
        {
            tempDir.createFile(filePath, "");
        }

        m_fileSystem->removeFilesInDirectory(directoryPath);

        ASSERT_FALSE(
            Common::FileSystem::fileSystem()->isFile(Common::FileSystem::join(directoryPath, filesInDirectory[0])));
        ASSERT_FALSE(
            Common::FileSystem::fileSystem()->isFile(Common::FileSystem::join(directoryPath, filesInDirectory[1])));
        ASSERT_FALSE(
            Common::FileSystem::fileSystem()->isFile(Common::FileSystem::join(directoryPath, filesInDirectory[2])));
    }

    TEST_F(FileSystemImplTest, removeFilesInDirDoesNotThrowWhenDirectoryDoesNotExist)
    {
        Tests::TempDir tempDir;
        Path directoryPath = Common::FileSystem::join(tempDir.dirPath(), "missing_dir");

        EXPECT_NO_THROW(m_fileSystem->removeFilesInDirectory(directoryPath));
    }

    TEST_F(FileSystemImplTest, removeFilesInDirDoesNotThrowWhenFilesDoNotExistDoesNotThrow)
    {
        Tests::TempDir tempDir;
        Path directoryPath = tempDir.dirPath();

        EXPECT_EQ(m_fileSystem->listFiles(directoryPath).size(), 0);
        EXPECT_NO_THROW(m_fileSystem->removeFilesInDirectory(directoryPath));
    }

    TEST_F(FileSystemImplTest, waitForExistingFileReturnsTrue)
    {
        Tests::TempDir tempDir;
        Path directoryPath = tempDir.dirPath();
        std::string filename = "existing-file";
        tempDir.createFile(filename, "");
        Path filePath = Common::FileSystem::join(directoryPath, filename);
        EXPECT_TRUE(m_fileSystem->waitForFile(filePath, 10));
    }

    TEST_F(FileSystemImplTest, waitForNonExistentFileReturnsFalse)
    {
        Tests::TempDir tempDir;
        Path directoryPath = tempDir.dirPath();
        std::string filename = "existing-file";
        Path filePath = Common::FileSystem::join(directoryPath, filename);
        EXPECT_FALSE(m_fileSystem->waitForFile(filePath, 10));
    }

    TEST_F(FileSystemImplTest, appendToFileCreatesAndAppendsToFile)
    {
        Tests::TempDir tempDir;
        Path directoryPath = tempDir.dirPath();
        std::string filename = "appendTestFileName";
        Path filePath = Common::FileSystem::join(directoryPath, filename);
        m_fileSystem->appendFile(filePath, "a");
        m_fileSystem->appendFile(filePath, "b");
        m_fileSystem->appendFile(filePath, "c");
        EXPECT_EQ(m_fileSystem->readFile(filePath), R"(abc)");
    }

    TEST_F(FileSystemImplTest, appendToFileAppendsToExistingFile)
    {
        Tests::TempDir tempDir;
        Path directoryPath = tempDir.dirPath();
        std::string filename = "appendTestExistingFileName";
        Path filePath = Common::FileSystem::join(directoryPath, filename);
        tempDir.createFile(filename, "123");
        m_fileSystem->appendFile(filePath, "a");
        m_fileSystem->appendFile(filePath, "b");
        m_fileSystem->appendFile(filePath, "c");
        EXPECT_EQ(m_fileSystem->readFile(filePath), R"(123abc)");
    }

    TEST_F(FileSystemImplTest, moveFile)
    {
        std::string filePath = Common::FileSystem::join(m_fileSystem->currentWorkingDirectory(), "moveFile.txt");
        std::string filePathMoved = filePath + "moved";
        std::string testContent("HelloWorld");
        m_fileSystem->writeFile(filePath, testContent);
        EXPECT_TRUE(m_fileSystem->isFile(filePath));
        m_fileSystem->moveFile(filePath, filePathMoved);
        EXPECT_FALSE(m_fileSystem->exists(filePath));
        EXPECT_TRUE(m_fileSystem->exists(filePathMoved));
    }

    TEST_F(FileSystemImplTest, moveFileWorksOnDirectories)
    {
        Tests::TempDir tempDir;
        Path directoryPath = tempDir.dirPath();
        Path dirPath = Common::FileSystem::join(directoryPath, "moveFileWorksOnDirectories");
        std::string dirPathMoved = dirPath + "moved";
        EXPECT_FALSE(m_fileSystem->exists(dirPath));
        EXPECT_FALSE(m_fileSystem->exists(dirPathMoved));
        m_fileSystem->makedirs(dirPath);
        std::string testFilePath = Common::FileSystem::join(dirPath, "moveFileWorksOnDirectoriesTestFile");
        std::string testFilePathAfterMove =
            Common::FileSystem::join(dirPathMoved, "moveFileWorksOnDirectoriesTestFile");
        m_fileSystem->writeFile(testFilePath, "HelloWorld");
        EXPECT_TRUE(m_fileSystem->isFile(testFilePath));

        m_fileSystem->moveFile(dirPath, dirPathMoved);
        EXPECT_FALSE(m_fileSystem->exists(testFilePath));
        EXPECT_TRUE(m_fileSystem->exists(testFilePathAfterMove));
    }

    TEST_F(FileSystemImplTest, listAllFilesInDirectoryTreeReturnsExpectedListOfFiles)
    {
        Tests::TempDir tempDir;
        tempDir.makeDirs("Root/subdir");
        tempDir.createFile("Root/file1", "hello");
        tempDir.createFile("Root/file2", "hello");
        tempDir.createFile("Root/subdir/file1", "hello");

        std::vector<Path> fileList = m_fileSystem->listAllFilesInDirectoryTree(tempDir.absPath("Root"));

        EXPECT_EQ(fileList.size(), 3);

        std::sort(fileList.begin(), fileList.end());

        EXPECT_EQ(fileList[0], tempDir.absPath("Root/file1"));
        EXPECT_EQ(fileList[1], tempDir.absPath("Root/file2"));
        EXPECT_EQ(fileList[2], tempDir.absPath("Root/subdir/file1"));
    }

    TEST_F(FileSystemImplTest, listAllFilesInDirectoryTreeHandlesEmptyDirectory)
    {
        Tests::TempDir tempDir;
        tempDir.makeDirs("Root");

        std::vector<Path> fileList = m_fileSystem->listAllFilesInDirectoryTree(tempDir.absPath("Root"));

        EXPECT_EQ(fileList.size(), 0);
    }

    TEST_F(FileSystemImplTest, listAllFilesInDirectoryTreeDoesntThrowOnDirThatDoesNotExist)
    {
        std::vector<Path> fileList;
        EXPECT_NO_THROW(fileList = m_fileSystem->listAllFilesInDirectoryTree("DoesntExistDir"));
        EXPECT_EQ(fileList.size(), 0);
    }

    TEST_F(FileSystemImplTest, listAllFilesAndDirsInDirectoryTreeReturnsExpectedListOfFiles)
    {
        Tests::TempDir tempDir;
        tempDir.makeDirs("Root/subdir");
        tempDir.createFile("Root/file1", "hello");
        tempDir.createFile("Root/file2", "hello");
        tempDir.createFile("Root/subdir/file1", "hello");

        std::vector<Path> fileList = m_fileSystem->listAllFilesAndDirsInDirectoryTree(tempDir.absPath("Root"));

        EXPECT_EQ(fileList.size(), 4);

        std::sort(fileList.begin(), fileList.end());

        EXPECT_EQ(fileList[0], tempDir.absPath("Root/file1"));
        EXPECT_EQ(fileList[1], tempDir.absPath("Root/file2"));
        EXPECT_EQ(fileList[2], tempDir.absPath("Root/subdir"));
        EXPECT_EQ(fileList[3], tempDir.absPath("Root/subdir/file1"));
    }

    TEST_F(FileSystemImplTest, listAllFilesAndDirsInDirectoryTreeHandlesEmptyDir)
    {
        Tests::TempDir tempDir;
        tempDir.makeDirs("empty_dir");
        std::vector<Path> fileList = m_fileSystem->listAllFilesAndDirsInDirectoryTree(tempDir.absPath("empty_dir"));
        ASSERT_EQ(fileList.size(), 0);
    }

    TEST_F(FileSystemImplTest, listAllFilesAndDirsInDirectoryTreeHandlesFile)
    {
        Tests::TempDir tempDir;
        tempDir.makeDirs("dir");
        tempDir.createFile("dir/file1", "hello");
        std::vector<Path> fileList = m_fileSystem->listAllFilesAndDirsInDirectoryTree(tempDir.absPath("dir/file1"));
        ASSERT_EQ(fileList.size(), 0);
    }

    TEST_F(FileSystemImplTest, listAllFilesAndDirsInDirectoryTreeDoesNotThrowOnPathThatDoesNotExist)
    {
        std::vector<Path> fileList;
        EXPECT_NO_THROW(fileList = m_fileSystem->listAllFilesAndDirsInDirectoryTree("Does not exist"));
        EXPECT_EQ(fileList.size(), 0);
    }

    TEST_F(FileSystemImplTest, calculateDigestFromValidPath)
    {
        const Tests::TempDir tempDir{};
        const std::string filename{ "file" };
        const std::string content{ "foo" };
        tempDir.createFile(filename, content);

        std::string digest{};
        EXPECT_NO_THROW(
            digest = m_fileSystem->calculateDigest(Common::SslImpl::Digest::md5, tempDir.absPath(filename)));
        EXPECT_EQ(digest, Common::SslImpl::calculateDigest(Common::SslImpl::Digest::md5, content));
    }

    TEST_F(FileSystemImplTest, calculateDigestThrowsOnNonExistentPath)
    {
        const Tests::TempDir tempDir{};
        EXPECT_ANY_THROW(
            std::ignore = m_fileSystem->calculateDigest(Common::SslImpl::Digest::md5, tempDir.absPath("non-existent")));
    }

    TEST_F(FileSystemImplTest, calculateDigestThrowsOnValidDirectoryPath)
    {
        const Tests::TempDir tempDir{};
        EXPECT_ANY_THROW(std::ignore = m_fileSystem->calculateDigest(Common::SslImpl::Digest::md5, tempDir.dirPath()));
    }

    TEST_F(FileSystemImplTest, calculateDigestFromValidFd)
    {
        const Tests::TempDir tempDir{};
        const std::string filename{ "file" };
        const std::string content{ "foo" };
        tempDir.createFile(filename, content);

        const int fd{ ::open(tempDir.absPath(filename).c_str(), O_RDONLY) };

        std::string digest{};
        EXPECT_NO_THROW(digest = m_fileSystem->calculateDigest(Common::SslImpl::Digest::md5, fd));
        EXPECT_EQ(digest, Common::SslImpl::calculateDigest(Common::SslImpl::Digest::md5, content));

        EXPECT_EQ(::close(fd), 0);
    }

    TEST_F(FileSystemImplTest, calculateDigestFromValidFdButAfterDeletingFile)
    {
        const Tests::TempDir tempDir{};
        const std::string filename{ "file" };
        const std::string content{ "foo" };
        tempDir.createFile(filename, content);

        const int fd{ ::open(tempDir.absPath(filename).c_str(), O_RDONLY) };
        fileSystem()->removeFile(tempDir.absPath(filename));

        std::string digest{};
        EXPECT_NO_THROW(digest = m_fileSystem->calculateDigest(Common::SslImpl::Digest::md5, fd));
        EXPECT_EQ(digest, Common::SslImpl::calculateDigest(Common::SslImpl::Digest::md5, content));

        EXPECT_EQ(::close(fd), 0);
    }

    TEST_F(FileSystemImplTest, calculateDigestThrowsOnInvalidFd)
    {
        EXPECT_ANY_THROW(std::ignore = m_fileSystem->calculateDigest(Common::SslImpl::Digest::md5, -1));
    }

    TEST_F(FileSystemImplTest, calculateDigestThrowsOnValidFdButWithoutReadFlag)
    {
        const Tests::TempDir tempDir{};
        const std::string filename{ "file" };
        const std::string content{ "foo" };
        tempDir.createFile(filename, content);

        const int fd{ ::open(tempDir.absPath(filename).c_str(), O_WRONLY) };
        EXPECT_ANY_THROW(std::ignore = m_fileSystem->calculateDigest(Common::SslImpl::Digest::md5, fd));
        EXPECT_EQ(::close(fd), 0);
    }

    TEST_F(FileSystemImplTest, calculateDigestThrowsOnValidDirectoryFd)
    {
        const Tests::TempDir tempDir{};

        const int fd{ ::open(tempDir.dirPath().c_str(), O_RDONLY) };
        EXPECT_ANY_THROW(std::ignore = m_fileSystem->calculateDigest(Common::SslImpl::Digest::md5, fd));
        EXPECT_EQ(::close(fd), 0);
    }

    TEST_F(FileSystemImplTest, calculateDigestWithFdOffsetCalculatesDigestOfWholeFile)
    {
        const Tests::TempDir tempDir;
        const std::string filename = "file";
        const std::string content = "foo_bar_baz";
        tempDir.createFile(filename, content);

        const int fd{ ::open(tempDir.absPath(filename).c_str(), O_RDONLY) };
        ::lseek(fd, 4, SEEK_SET); // change the file offset

        std::string digest;
        EXPECT_NO_THROW(digest = m_fileSystem->calculateDigest(Common::SslImpl::Digest::md5, fd));
        EXPECT_EQ(digest, Common::SslImpl::calculateDigest(Common::SslImpl::Digest::md5, content));

        EXPECT_EQ(::close(fd), 0);
    }

    class mockFileSystemForMoveFileTryCopy : public FileSystemImpl
    {
    public:
        MOCK_METHOD(int, moveFileImpl, (const Path& sourcePath, const Path& destPath), (const, override));
        MOCK_METHOD(void, copyFile, (const Path& sourcePath, const Path& destPath), (const, override));
        MOCK_METHOD(void, removeFile, (const Path& filePath), (const, override));
    };


    TEST_F(FileSystemImplTest, moveFileTryCopyMovesFile)
    {
        std::string filePath = Common::FileSystem::join(m_fileSystem->currentWorkingDirectory(), "moveFile.txt");
        std::string filePathMoved = filePath + "moved";
        std::string testContent("HelloWorld");
        m_fileSystem->writeFile(filePath, testContent);
        EXPECT_TRUE(m_fileSystem->isFile(filePath));
        m_fileSystem->moveFileTryCopy(filePath, filePathMoved);
        EXPECT_FALSE(m_fileSystem->exists(filePath));
        EXPECT_TRUE(m_fileSystem->exists(filePathMoved));
    }

    TEST_F(FileSystemImplTest, moveFileTryCopyWorksOnDirectories)
    {
        Tests::TempDir tempDir;
        Path directoryPath = tempDir.dirPath();
        Path dirPath = Common::FileSystem::join(directoryPath, "moveFileTryCopyWorksOnDirectories");
        std::string dirPathMoved = dirPath + "moved";
        EXPECT_FALSE(m_fileSystem->exists(dirPath));
        EXPECT_FALSE(m_fileSystem->exists(dirPathMoved));
        m_fileSystem->makedirs(dirPath);
        std::string testFilePath = Common::FileSystem::join(dirPath, "moveFileTryCopyWorksOnDirectories");
        std::string testFilePathAfterMove =
            Common::FileSystem::join(dirPathMoved, "moveFileTryCopyWorksOnDirectories");
        m_fileSystem->writeFile(testFilePath, "HelloWorld");
        EXPECT_TRUE(m_fileSystem->isFile(testFilePath));

        m_fileSystem->moveFileTryCopy(dirPath, dirPathMoved);
        EXPECT_FALSE(m_fileSystem->exists(testFilePath));
        EXPECT_TRUE(m_fileSystem->exists(testFilePathAfterMove));
    }

    TEST_F(FileSystemImplTest, moveFileTryCopyCopysFileWhenMoveFails_EXDEV)
    {
        std::string sourceDir = "Im/a/source/dir";
        std::string destDir = "Im/a/dest/dir";

        auto mockFileSystem = mockFileSystemForMoveFileTryCopy();

        EXPECT_CALL(mockFileSystem, moveFileImpl(sourceDir, destDir)).WillOnce(Return(EXDEV));
        EXPECT_CALL(mockFileSystem, copyFile(sourceDir, destDir)).Times(1);
        EXPECT_CALL(mockFileSystem, removeFile(sourceDir)).Times(1);
        EXPECT_NO_THROW(mockFileSystem.moveFileTryCopy(sourceDir, destDir));
    }

    TEST_F(FileSystemImplTest, moveFileTryCopyDoesntCopysFileWhenMoveFails_OTHERERROR)
    {
        std::string sourceDir = "Im/a/source/dir";
        std::string destDir = "Im/a/dest/dir";

        auto mockFileSystem = mockFileSystemForMoveFileTryCopy();

        EXPECT_CALL(mockFileSystem, moveFileImpl(sourceDir, destDir)).WillOnce(Return(1));
        EXPECT_THROW(mockFileSystem.moveFileTryCopy(sourceDir, destDir), IFileSystemException);
    }

    TEST_F(FileSystemImplTest, moveFileThrowsSourceDoesntExist)
    {
        std::string filePath = "/path/doesnt/exist/moveFile.txt";
        std::string filePathMoved = Common::FileSystem::join(m_fileSystem->currentWorkingDirectory(), "moveFile.txt");
        std::string expectedErr = "Could not move " + filePath + "(doesn't exist) to " + filePathMoved + "(exists): No such file or directory(2)";

        ASSERT_FALSE(m_fileSystem->isFile(filePath));
        try
        {
          m_fileSystem->moveFile(filePath, filePathMoved);
          FAIL() << "didnt throw when source file path didnt exist";
        }
        catch (const IFileSystemException& ex)
        {
          EXPECT_EQ(ex.what(), expectedErr);
        }
    }

    TEST_F(FileSystemImplTest, moveFileThrowsDestDirDoesntExist)
    {
        std::string filePath = Common::FileSystem::join(m_fileSystem->currentWorkingDirectory(), "moveFile.txt");
        m_fileSystem->writeFile(filePath, "text");
        std::string filePathMoved = "/path/doesnt/exist/moveFile.txt";

        std::string expectedErr = "Could not move " + filePath + "(exists) to " + filePathMoved + "(doesn't exist): No such file or directory(2)";

        ASSERT_TRUE(m_fileSystem->isFile(filePath));
        try
        {
          m_fileSystem->moveFile(filePath, filePathMoved);
          FAIL() << "didnt throw when destination file path didnt exist";
        }
        catch (const IFileSystemException& ex)
        {
          EXPECT_EQ(ex.what(), expectedErr);
        }
    }

    TEST_F(FileSystemImplTest, moveFileTryCopyThrowsSourceDoesntExist)
    {
        std::string filePath = "/path/doesnt/exist/moveFile.txt";
        std::string filePathMoved = Common::FileSystem::join(m_fileSystem->currentWorkingDirectory(), "moveFile.txt");
        std::string expectedErr = "Could not move " + filePath + "(doesn't exist) to " + filePathMoved + "(exists): No such file or directory(2)";

        ASSERT_FALSE(m_fileSystem->isFile(filePath));
        try
        {
          m_fileSystem->moveFileTryCopy(filePath, filePathMoved);
          FAIL() << "didnt throw when source file path didnt exist";
        }
        catch (const IFileSystemException& ex)
        {
          EXPECT_EQ(ex.what(), expectedErr);
        }
    }

    TEST_F(FileSystemImplTest, moveFileTryCopyThrowsDestDirDoesntExist)
    {
        std::string filePath = Common::FileSystem::join(m_fileSystem->currentWorkingDirectory(), "moveFile.txt");
        m_fileSystem->writeFile(filePath, "text");
        std::string filePathMoved = "/path/doesnt/exist/moveFile.txt";

        std::string expectedErr = "Could not move " + filePath + "(exists) to " + filePathMoved + "(doesn't exist): No such file or directory(2)";

        ASSERT_TRUE(m_fileSystem->isFile(filePath));
        try
        {
          m_fileSystem->moveFileTryCopy(filePath, filePathMoved);
          FAIL() << "didnt throw when destination file path didnt exist";
        }
        catch (const IFileSystemException& ex)
        {
          EXPECT_EQ(ex.what(), expectedErr);
        }
    }

    class mockFileSystemForRecursiveDelete : public FileSystemImpl
    {
    public:
        MOCK_CONST_METHOD2(listFilesAndDirectories, std::vector<Path>(const Path& directoryPath, bool includeSymlinks));
        MOCK_CONST_METHOD1(removeFileOrDirectory, void(const Path& path));
    };

    TEST_F(
        FileSystemImplTest,
        recursivelyDeleteContentsOfDirectoryRemovesAllFilesAndDirectoriesInPassedDirectory)
    {
        mockFileSystemForRecursiveDelete mockFileSystem = mockFileSystemForRecursiveDelete();
        Path pathToClearOut = "path to delete from";
        EXPECT_CALL(mockFileSystem, listFilesAndDirectories(pathToClearOut, false))
            .WillOnce(Return(std::vector<Path>{ "1", "2", "3" }));
        EXPECT_CALL(mockFileSystem, removeFileOrDirectory("1")).Times(1);
        EXPECT_CALL(mockFileSystem, removeFileOrDirectory("2")).Times(1);
        EXPECT_CALL(mockFileSystem, removeFileOrDirectory("3")).Times(1);
        mockFileSystem.recursivelyDeleteContentsOfDirectory(pathToClearOut);
    }

    TEST_F(FileSystemImplTest, recursivelyDeleteContentsOfDirectoryThrowsWhenEncounteringFileSystemException)
    {
        mockFileSystemForRecursiveDelete mockFileSystem = mockFileSystemForRecursiveDelete();
        Path pathToClearOut = "path to delete from";
        EXPECT_CALL(mockFileSystem, listFilesAndDirectories(pathToClearOut, false))
            .WillOnce(Throw(Common::FileSystem::IFileSystemException("Failed")));
        EXPECT_THROW(
            mockFileSystem.recursivelyDeleteContentsOfDirectory(pathToClearOut),
            Common::FileSystem::IFileSystemException);
    }

    TEST_F(
        FileSystemImplTest,
        compareFileDescriptor)
    {
        Tests::TempDir tempDir;
        tempDir.createFile("1", "");
        int fd1 = open(tempDir.absPath("1").c_str(), O_PATH);
        int fd2 = open(tempDir.absPath("1").c_str(), O_PATH);
        EXPECT_TRUE(m_fileSystem->compareFileDescriptors(fd1, fd2));
        close(fd1);
        close(fd2);
    }

    TEST_F(
        FileSystemImplTest,
        compareFileDescriptorFailWhenFileAreDiffrent)
    {
        Tests::TempDir tempDir;
        tempDir.createFile("1", "");
        tempDir.createFile("2", "");
        int fd1 = open(tempDir.absPath("1").c_str(), O_PATH);
        int fd2 = open(tempDir.absPath("2").c_str(), O_PATH);
        EXPECT_FALSE(m_fileSystem->compareFileDescriptors(fd1, fd2));
        close(fd1);
        close(fd2);
    }

    TEST_F(
        FileSystemImplTest,
        compareFileDescriptorFailsWhenFileDoesNotExist)
    {
        Tests::TempDir tempDir;
        int fd1 = open(tempDir.absPath("1").c_str(), O_PATH);
        int fd2 = open(tempDir.absPath("1").c_str(), O_PATH);
        EXPECT_FALSE(m_fileSystem->compareFileDescriptors(fd1, fd2));
        close(fd1);
        close(fd2);
    }

    TEST_F(
        FileSystemImplTest,
        getFileDescriptorWorksForDirectory)
    {
        Tests::TempDir tempDir;
        tempDir.makeDirs("1");
        int fd1 = open(tempDir.absPath("1").c_str(), O_PATH);
        int fd2 = m_fileSystem->getFileInfoDescriptor(tempDir.absPath("1"));
        EXPECT_TRUE(m_fileSystem->compareFileDescriptors(fd1, fd2));
        close(fd1);
        close(fd2);
    }

    TEST_F(
        FileSystemImplTest,
        getFileDescriptorWorksForFile)
    {
        Tests::TempDir tempDir;
        tempDir.createFile("1", "");
        int fd1 = open(tempDir.absPath("1").c_str(), O_PATH);
        int fd2 = m_fileSystem->getFileInfoDescriptor(tempDir.absPath("1"));
        EXPECT_TRUE(m_fileSystem->compareFileDescriptors(fd1, fd2));
        close(fd1);
        close(fd2);
    }

    TEST_F(
        FileSystemImplTest,
        getFileDescriptorReturnsMinus1WhenPathDoesNotExist)
    {
        int fd1 = m_fileSystem->getFileInfoDescriptor("/etc/randomsokdojdiuenb");
        EXPECT_EQ(-1, fd1);
        close(fd1);
    }

    TEST_F(
        FileSystemImplTest,
        getFileInfoDescriptorFromDirectoryFD)
    {
        Tests::TempDir tempDir;
        tempDir.makeDirs("subdir");
        tempDir.createFile("subdir/file1", "");
        int dirfd = open(tempDir.absPath("subdir").c_str(), O_PATH);
        int fd1 = open(tempDir.absPath("subdir/file1").c_str(), O_PATH);
        int fd2 = m_fileSystem->getFileInfoDescriptorFromDirectoryFD(dirfd, tempDir.absPath("subdir/file1"));
        EXPECT_TRUE(m_fileSystem->compareFileDescriptors(fd1, fd2));
        close(dirfd);
        close(fd1);
        close(fd2);
    }
    TEST_F(
        FileSystemImplTest,
        getFDFromDirectoryFDFailsWhendirDoesNotExist)
    {
        int dirfd = open("/etc/random383y3rhfr8e", O_PATH);
        int fd1 = m_fileSystem->getFileInfoDescriptorFromDirectoryFD(dirfd, "/etc/random383y3rhfr8e/file1");
        EXPECT_EQ(-1, fd1);
        close(dirfd);
        close(fd1);
    }

    TEST_F(
        FileSystemImplTest,
        getFDFromDirectoryFDFailsWhenFileDoesNotExist)
    {
        Tests::TempDir tempDir;
        tempDir.makeDirs("subdir");
        int dirfd = open(tempDir.absPath("subdir").c_str(), O_PATH);
        int fd1 = m_fileSystem->getFileInfoDescriptorFromDirectoryFD(dirfd, tempDir.absPath("subdir/file1"));
        EXPECT_EQ(-1, fd1);
        close(dirfd);
        close(fd1);
    }

    TEST_F( FileSystemImplTest, unlinkFileUsingDirectoryFDSuccess)
    {
        Tests::TempDir tempDir;
        tempDir.makeDirs("Root/");
        tempDir.createFile("Root/file","");
        int dirfd = open(tempDir.absPath("Root").c_str(), O_PATH);
        EXPECT_NO_THROW(m_fileSystem->unlinkFileUsingDirectoryFD(dirfd, "file"));
        close(dirfd);
        EXPECT_FALSE(m_fileSystem->isFile(tempDir.absPath("Root/file")));
    }

    TEST_F(FileSystemImplTest, unlinkFileUsingDirectoryFDFailsOnDir)
    {
        Tests::TempDir tempDir;
        tempDir.makeDirs("Root/subdir");

        int dirfd = open(tempDir.absPath("Root").c_str(), O_PATH);
        EXPECT_THROW(m_fileSystem->unlinkFileUsingDirectoryFD(dirfd, "subdir"),IFileSystemException);
        close(dirfd);
        EXPECT_TRUE(m_fileSystem->isDirectory(tempDir.absPath("Root/subdir")));
    }

    TEST_F(FileSystemImplTest, unlinkFileUsingDirectoryFDFailsOnDoesntExistFile)
    {
        Tests::TempDir tempDir;
        tempDir.makeDirs("Root/");

        int dirfd = open(tempDir.absPath("Root").c_str(), O_PATH);
        EXPECT_THROW(m_fileSystem->unlinkFileUsingDirectoryFD(dirfd, "subdir33"),IFileSystemException);
        close(dirfd);
    }

    TEST_F(FileSystemImplTest, unlinkFileUsingDirectoryFDFailsOnInvalidFD)
    {
        Tests::TempDir tempDir;
        tempDir.makeDirs("Root/subdir");
        EXPECT_THROW(m_fileSystem->unlinkFileUsingDirectoryFD(-1, "subdir"),IFileSystemException);

    }

    TEST_F( FileSystemImplTest, unlinkDirUsingDirectoryFDSuccess)
    {
        Tests::TempDir tempDir;
        tempDir.makeDirs("Root/subdir");
        int dirfd = open(tempDir.absPath("Root").c_str(), O_PATH);
        EXPECT_NO_THROW(m_fileSystem->unlinkDirUsingDirectoryFD(dirfd, "subdir"));
        close(dirfd);
        EXPECT_FALSE(m_fileSystem->isFile(tempDir.absPath("Root/subdir")));
    }

    TEST_F(FileSystemImplTest, unlinkDirUsingDirectoryFDFailsOnDoesntExistDir)
    {
        Tests::TempDir tempDir;
        tempDir.makeDirs("Root/");

        int dirfd = open(tempDir.absPath("Root").c_str(), O_PATH);
        EXPECT_THROW(m_fileSystem->unlinkDirUsingDirectoryFD(dirfd, "subdir33"),IFileSystemException);
        close(dirfd);
    }

    TEST_F(FileSystemImplTest, unlinkDirUsingDirectoryFDFailsOnInvalidFD)
    {
        Tests::TempDir tempDir;
        tempDir.makeDirs("Root/subdir");
        EXPECT_THROW(m_fileSystem->unlinkDirUsingDirectoryFD(-1, "subdir"),IFileSystemException);
    }

    TEST_F(FileSystemImplTest, getDiskSpaceInfo)
    {
        auto diskSpaceInfo = m_fileSystem->getDiskSpaceInfo("/");
        EXPECT_GE(diskSpaceInfo.capacity, diskSpaceInfo.free);
        EXPECT_GE(diskSpaceInfo.free, diskSpaceInfo.available);
        EXPECT_GT(diskSpaceInfo.available, 0);
    }

    TEST_F(FileSystemImplTest, getDiskSpaceInfoErrCode)
    {
        std::error_code ec;
        auto diskSpaceInfo = m_fileSystem->getDiskSpaceInfo("/opt", ec);
        EXPECT_EQ(ec.value(), 0);
        EXPECT_GE(diskSpaceInfo.capacity, diskSpaceInfo.free);
        EXPECT_GE(diskSpaceInfo.free, diskSpaceInfo.available);
        EXPECT_GT(diskSpaceInfo.available, 0);
    }

    TEST_F(FileSystemImplTest, getSystemCommandExecutablePathSucess)
    {
        auto filepath = m_fileSystem->getSystemCommandExecutablePath("touch");
        EXPECT_GT(filepath.size(), 0);
        EXPECT_TRUE(m_fileSystem->isExecutable(filepath));
    }

    TEST_F(FileSystemImplTest, getSystemCommandExecutablePathFailed)
    {
        EXPECT_THROW(std::ignore = m_fileSystem->getSystemCommandExecutablePath("notARealExe"),IFileSystemException);
    }
} // namespace
