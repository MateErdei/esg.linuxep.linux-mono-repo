/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include <Common/FileSystem/IFilePermissions.h>
#include <Common/FileSystem/IFileSystem.h>
#include <Common/FileSystem/IFileSystemException.h>
#include <Common/FileSystemImpl/FileSystemImpl.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <tests/Common/Helpers/FileSystemReplaceAndRestore.h>
#include <tests/Common/Helpers/MockFileSystem.h>
#include <tests/Common/Helpers/TempDir.h>

#include <fstream>

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
        ~FileSystemImplTest() { Tests::restoreFileSystem(); }
        std::unique_ptr<IFileSystem> m_fileSystem;
        void SetUp() override { m_fileSystem.reset(new FileSystemImpl()); }

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
        }
    };

    TEST_F(FileSystemImplTest, basenameReturnsCorrectMatchingValue) // NOLINT
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

    TEST_F(FileSystemImplTest, joinReturnsCorrectValueWithNoSeporator) // NOLINT
    {
        std::string path1("/tmp");
        std::string path2("tempfile.txt");
        std::string expectedValue("/tmp/tempfile.txt");
        EXPECT_EQ(Common::FileSystem::join(path1, path2), expectedValue);
    }

    TEST_F(FileSystemImplTest, joinReturnsCorrectValueWithPath1TrailingSeporator) // NOLINT
    {
        std::string path1("/tmp/");
        std::string path2("tempfile.txt");
        std::string expectedValue("/tmp/tempfile.txt");
        EXPECT_EQ(Common::FileSystem::join(path1, path2), expectedValue);
    }

    TEST_F(FileSystemImplTest, joinReturnsCorrectValueWithPath2StartingWithSeporator) // NOLINT
    {
        // python -c 'import os; print os.path.join("/foo","/bar")' -> "/bar

        std::string path1("/tmp");
        std::string path2("/tempfile.txt");
        std::string expectedValue("/tempfile.txt");
        EXPECT_EQ(Common::FileSystem::join(path1, path2), expectedValue);
    }

    TEST_F(FileSystemImplTest, joinReturnsCorrectValueWithPath1TrailingSeporatorAndPath2StartingWithSeporator) // NOLINT
    {
        // python -c 'import os; print os.path.join("/tmp/","/tempfile.txt")' -> /tempfile.txt
        std::string path1("/tmp/");
        std::string path2("/tempfile.txt");
        std::string expectedValue("/tempfile.txt");
        EXPECT_EQ(Common::FileSystem::join(path1, path2), expectedValue);
    }

    TEST_F(FileSystemImplTest, joinWithDotSlashReturnsExpectedPath) // NOLINT
    {
        std::string path1("/tmp/");
        std::string path2("./tempfile.txt");
        std::string expectedValue("/tmp/tempfile.txt");
        EXPECT_EQ(Common::FileSystem::join(path1, path2), expectedValue);
    }

    TEST_F(FileSystemImplTest, joinWithDotSlashSlashReturnsExpectedPath) // NOLINT
    {
        // python -c 'import os; print os.path.join("/tmp/",".//tempfile.txt")' -> /tmp/.//tempfile.txt
        // python doesn't have any special handling for ./ in join
        // python -c 'import os; print os.path.normpath(os.path.join("/tmp/",".//tempfile.txt"))' -> /tmp/tempfile.txt

        std::string path1("/tmp/");
        std::string path2(".//tempfile.txt");
        std::string expectedValue("/tmp/tempfile.txt");
        EXPECT_EQ(Common::FileSystem::join(path1, path2), expectedValue);
    }

    TEST_F(FileSystemImplTest, joinWithEmptyStringsReturnsExpectedPath) // NOLINT
    {
        // python -c 'import os; print os.path.join("","")' -> ""
        std::string path1;
        std::string path2;
        std::string expectedValue;
        EXPECT_EQ(Common::FileSystem::join(path1, path2), expectedValue);
    }

    TEST_F(FileSystemImplTest, joinWithEmptyPath1AndAbsolutePath2ReturnsPath2) // NOLINT
    {
        Path path1;
        Path path2("/foo/bar");
        Path& expectedValue = path2;
        EXPECT_EQ(Common::FileSystem::join(path1, path2), expectedValue);
    }

    TEST_F(FileSystemImplTest, joinWithEmptyPath1AndRelativePath2ReturnsExpectedPath) // NOLINT
    {
        // python -c 'import os; print os.path.join("","foo/bar")' -> foo/bar
        Path path1;
        Path path2("foo/bar");
        Path expectedValue("foo/bar");
        EXPECT_EQ(Common::FileSystem::join(path1, path2), expectedValue);
    }

    TEST_F(FileSystemImplTest, joinWithEmptyPath2ReturnsExpectedPath) // NOLINT
    {
        // python -c 'import os; print os.path.join("foo/bar","")' -> foo/bar
        Path path1("foo/bar");
        Path path2;
        Path expectedValue("foo/bar");
        EXPECT_EQ(Common::FileSystem::join(path1, path2), expectedValue);
    }

    TEST_F(FileSystemImplTest, joinWithTwoRelativePathsReturnsARelativePath) // NOLINT
    {
        // python -c 'import os; print os.path.join("foo","bar")'
        Path path1("foo");
        Path path2("bar");
        Path expectedValue("foo/bar");
        EXPECT_EQ(Common::FileSystem::join(path1, path2), expectedValue);
    }

    TEST_F(FileSystemImplTest, existReturnsTrueWhenFileExists) // NOLINT
    {
        EXPECT_TRUE(m_fileSystem->exists("/etc/passwd"));
    }

    TEST_F(FileSystemImplTest, existReturnsFalseWhenFileOrDirectoryDoesNotExist) // NOLINT
    {
        EXPECT_FALSE(m_fileSystem->exists("/etc/ghdfoijsdfgjhszdgvzjsdjvcjsdhzc"));
    }

    TEST_F(FileSystemImplTest, existReturnsTrueWhenPathGivenIsDirectory) // NOLINT
    {
        EXPECT_TRUE(m_fileSystem->exists("/etc"));
    }

    TEST_F(FileSystemImplTest, isFileReturnsTrueWhenPathGivenPointsToAFile) // NOLINT
    {
        EXPECT_TRUE(m_fileSystem->isFile("/etc/passwd"));
    }

    TEST_F(FileSystemImplTest, isFileReturnsFalseWhenPathGivenPointsToADirectory) // NOLINT
    {
        EXPECT_FALSE(m_fileSystem->isFile("/etc"));
    }

    TEST_F(FileSystemImplTest, isDirectoryReturnsFalseWhenPathGivenPointsToAFile) // NOLINT
    {
        EXPECT_FALSE(m_fileSystem->isDirectory("/etc/passwd"));
    }

    TEST_F(FileSystemImplTest, isDirectoryReturnsTrueWhenRelativePathGivenPointsToCurrentDirectory) // NOLINT
    {
        EXPECT_TRUE(m_fileSystem->isDirectory("."));
    }

    TEST_F(FileSystemImplTest, isDirectoryReturnsFalseWhenDirectoryDoesNotExist) // NOLINT
    {
        EXPECT_FALSE(m_fileSystem->isDirectory("/etc/ghdfoijsdfgjhszdgvzjsdjvcjsdhzc"));
    }

    TEST_F(FileSystemImplTest, isDirectoryReturnsTrueWhenPathGivenIsDirectory) // NOLINT
    {
        EXPECT_TRUE(m_fileSystem->isDirectory("/etc"));
    }

    TEST_F(FileSystemImplTest, isSymlinkReturnsTrueForSymlink) // NOLINT
    {
        EXPECT_TRUE(m_fileSystem->isSymlink("/proc/self/exe"));
    }

    TEST_F(FileSystemImplTest, currentWorkingDirectoryReturnsCorrectValue) // NOLINT
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

    TEST_F(FileSystemImplTest, readWriteStoresAndReadsExpectedContentFromFile) // NOLINT
    {
        std::string filePath =
            Common::FileSystem::join(m_fileSystem->currentWorkingDirectory(), "ReadWriteFileTest.txt");

        std::string testContent("HelloWorld");

        m_fileSystem->writeFile(filePath, testContent);

        EXPECT_EQ(m_fileSystem->readFile(filePath), testContent);
        removeFile(filePath);
    }

    TEST_F(FileSystemImplTest, writeOverwritesExistingFile) // NOLINT
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

    TEST_F(FileSystemImplTest, readThrowsForTooLargeFile) // NOLINT
    {
        std::string filePath =
            Common::FileSystem::join(m_fileSystem->currentWorkingDirectory(), "readThrowsForTooLargeFile.txt");

        std::string testContent("HelloWorld");

        m_fileSystem->writeFile(filePath, testContent);

        EXPECT_THROW(m_fileSystem->readFile(filePath, 2), IFileSystemException); // NOLINT

        removeFile(filePath);
    }

    TEST_F(FileSystemImplTest, writeFileUsingDirectoryPathShouldThrow) // NOLINT
    {
        std::string directoryPath =
            Common::FileSystem::join(m_fileSystem->currentWorkingDirectory(), "WriteToDirectoryTest");

        makedir(directoryPath, 0700);

        std::string testContent("HelloWorld");

        EXPECT_THROW(m_fileSystem->writeFile(directoryPath, testContent), IFileSystemException); // NOLINT

        ::rmdir(directoryPath.c_str());
    }

    TEST_F(FileSystemImplTest, readFileThatDoesNotExistsShouldThrow) // NOLINT
    {
        std::string filePath = Common::FileSystem::join(m_fileSystem->currentWorkingDirectory(), "ReadFileTest.txt");
        EXPECT_THROW(m_fileSystem->readFile(filePath), IFileSystemException); // NOLINT
    }

    TEST_F(FileSystemImplTest, readFileUsingDirectoryPathShouldThrow) // NOLINT
    {
        std::string directoryPath =
            Common::FileSystem::join(m_fileSystem->currentWorkingDirectory(), "WriteToDirectoryTest");

        makedir(directoryPath, 0700);
        EXPECT_THROW(m_fileSystem->readFile(directoryPath), IFileSystemException); // NOLINT

        ::rmdir(directoryPath.c_str());
    }

    TEST_F(FileSystemImplTest, readLinesGetsExpectedContentFromFile) // NOLINT
    {
        std::string filePath = Common::FileSystem::join(m_fileSystem->currentWorkingDirectory(), "ReadLinesTest.txt");

        std::string testContent("HelloWorld\nHelloWorld2");

        m_fileSystem->writeFile(filePath, testContent);
        std::vector<std::string> content{ "HelloWorld", "HelloWorld2" };
        EXPECT_EQ(m_fileSystem->readLines(filePath), content);
        removeFile(filePath);
    }

    TEST_F(FileSystemImplTest, readLinesGetsExpectedContentFromEmptyFile) // NOLINT
    {
        std::string filePath = Common::FileSystem::join(m_fileSystem->currentWorkingDirectory(), "EmptyFileTest.txt");

        m_fileSystem->writeFile(filePath, "");
        std::vector<std::string> content;
        EXPECT_EQ(m_fileSystem->readLines(filePath), content);
        removeFile(filePath);
    }

    TEST_F(FileSystemImplTest, readLinesForFileThatDoesNotExistsShouldThrow) // NOLINT
    {
        std::string filePath = Common::FileSystem::join(m_fileSystem->currentWorkingDirectory(), "ReadFileTest.txt");
        EXPECT_THROW(m_fileSystem->readLines(filePath), IFileSystemException); // NOLINT
    }

    TEST_F(FileSystemImplTest, readLinesOnDirectoryPathShouldThrow) // NOLINT
    {
        std::string directoryPath =
            Common::FileSystem::join(m_fileSystem->currentWorkingDirectory(), "WriteToDirectoryTest");

        makedir(directoryPath, 0700);
        EXPECT_THROW(m_fileSystem->readLines(directoryPath), IFileSystemException); // NOLINT

        ::rmdir(directoryPath.c_str());
    }

    TEST_F(FileSystemImplTest, atomicWriteStoresExpectedContentForFile) // NOLINT
    {
        std::string filePath = Common::FileSystem::join(m_fileSystem->currentWorkingDirectory(), "AtomicWrite.txt");

        std::string testContent("HelloWorld");

        m_fileSystem->writeFileAtomically(filePath, testContent, m_fileSystem->currentWorkingDirectory());

        EXPECT_EQ(m_fileSystem->readFile(filePath), testContent);
        removeFile(filePath);
    }

    TEST_F(FileSystemImplTest, dirNameReturnsCorrectMatchingValue) // NOLINT
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

    TEST_F(FileSystemImplTest, isExecutableReturnTrueForExecutables) // NOLINT
    {
        EXPECT_TRUE(m_fileSystem->isExecutable("/bin/bash"));
    }

    TEST_F(FileSystemImplTest, isExecutableReturnFalseForNonExecutables) // NOLINT
    {
        EXPECT_FALSE(m_fileSystem->isExecutable("/etc/passwd"));
    }

    TEST_F(FileSystemImplTest, isExecutableReturnFalseForNonExistingFiles) // NOLINT
    {
        EXPECT_FALSE(m_fileSystem->isExecutable("/tmp/thisfiledoesnotexist"));
    }

    TEST_F(FileSystemImplTest, listFilesReturnsAListOfPathsOnlyContainingRegularFiles) // NOLINT
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

    TEST_F( // NOLINT
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

    TEST_F(FileSystemImplTest, listDirectoriesReturnsAListOfDirectories) // NOLINT
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

    TEST_F(FileSystemImplTest, canCreateDirectories) // NOLINT
    {
        Tests::TempDir tempdir("", "FileSystemImplTest_canCreateDirectories");
        Path A = tempdir.absPath("A");
        Path B = Common::FileSystem::join(A, "B");
        m_fileSystem->makedirs(B);
        EXPECT_TRUE(m_fileSystem->isDirectory(A));
        EXPECT_TRUE(m_fileSystem->isDirectory(B));
    }

    TEST_F(FileSystemImplTest, copyFile) // NOLINT
    {
        Tests::TempDir tempdir("", "FileSystemImplTest_copyFile");
        Path A = tempdir.absPath("A");
        Path B = tempdir.absPath("B");
        tempdir.createFile("A", "FOOBAR");
        EXPECT_NO_THROW(m_fileSystem->copyFile(A, B)); // NOLINT
        EXPECT_TRUE(m_fileSystem->exists(B));
        std::string content = m_fileSystem->readFile(B);
        EXPECT_EQ(content, "FOOBAR");
    }

    TEST_F(FileSystemImplTest, copyFileDoesNotExist) // NOLINT
    {
        Tests::TempDir tempdir("", "FileSystemImplTest_copyFile");
        Path A = tempdir.absPath("A");
        Path B = tempdir.absPath("B");
        copyFileAndExpectThrow(A, B, "Failed to copy file: '" + A + "' to '" + B + "', source file does not exist.");
        ; // NOLINT
        EXPECT_FALSE(m_fileSystem->exists(B));
    }

    TEST_F(FileSystemImplTest, copyFileWrongPermissions) // NOLINT
    {
        Tests::TempDir tempdir("", "FileSystemImplTest_copyFile");
        Path A = tempdir.absPath("A");
        Path B = tempdir.absPath("B");
        auto mockFileSystem = new StrictMock<MockFileSystem>();
        // With wrong permissions file will exist but not open
        EXPECT_CALL(*mockFileSystem, exists(A)).WillOnce(Return(true));
        std::unique_ptr<MockFileSystem> mockIFileSystemPtr = std::unique_ptr<MockFileSystem>(mockFileSystem);
        Tests::replaceFileSystem(std::move(mockIFileSystemPtr));
        copyFileAndExpectThrow(A, B, "Failed to copy file: '" + A + "' to '" + B + "', reading file failed.");
        EXPECT_FALSE(m_fileSystem->exists(B));
    }

    TEST_F(FileSystemImplTest, copyFileNonAccessibleDest) // NOLINT
    {
        Tests::TempDir tempdir("", "FileSystemImplTest_copyFile");
        Path A = tempdir.absPath("A");
        Path B = Common::FileSystem::join("NotADirectory", "NotAFile");
        tempdir.createFile("A", "FOOBAR");
        copyFileAndExpectThrow(A, B, "Failed to copy file: '" + A + "' to '" + B + "', dest file was not created.");
        EXPECT_FALSE(m_fileSystem->exists(B));
    }

    TEST_F(FileSystemImplTest, copyFileThrowsOnFailToCopyContent) // NOLINT
    {
        Tests::TempDir tempdir("", "FileSystemImplTest_copyFile");
        Path src = tempdir.absPath("A");
        Path dest = tempdir.absPath("B");
        tempdir.createFile("A", "FOOBAR");

        auto mockFileSystem = new StrictMock<MockFileSystem>();
        EXPECT_CALL(*mockFileSystem, exists(src)).WillOnce(Return(true));
        EXPECT_CALL(*mockFileSystem, exists(dest)).WillOnce(Return(true));
        EXPECT_CALL(*mockFileSystem, fileSize(src)).WillOnce(Return(1));
        EXPECT_CALL(*mockFileSystem, fileSize(dest)).WillOnce(Return(0));
        EXPECT_CALL(*mockFileSystem, removeFile(dest)).WillOnce(Return());
        std::unique_ptr<MockFileSystem> mockIFileSystemPtr = std::unique_ptr<MockFileSystem>(mockFileSystem);
        Tests::replaceFileSystem(std::move(mockIFileSystemPtr));
        copyFileAndExpectThrow(
            src,
            dest,
            "Failed to copy file: '" + src + "' to '" + dest +
                "', contents failed to copy. Check space available on device.");
    }

    TEST_F(FileSystemImplTest, removeFileDeletesFile) // NOLINT
    {
        std::string filePath =
            Common::FileSystem::join(m_fileSystem->currentWorkingDirectory(), "removeFileDeletesFile.txt");

        std::string testContent("HelloWorld");

        m_fileSystem->writeFile(filePath, testContent);

        EXPECT_TRUE(m_fileSystem->isFile(filePath));

        m_fileSystem->removeFile(filePath);

        EXPECT_FALSE(m_fileSystem->exists(filePath));
    }

    TEST_F(FileSystemImplTest, removeFileThrowsIfFileDoesNotExist) // NOLINT
    {
        std::string filePath =
            Common::FileSystem::join(m_fileSystem->currentWorkingDirectory(), "removeFileThrowsIfFileDoesNotExist.txt");
        ASSERT_FALSE(m_fileSystem->exists(filePath));
        EXPECT_THROW(m_fileSystem->removeFile(filePath), IFileSystemException); // NOLINT
    }

    TEST_F(FileSystemImplTest, removeDirectoryDeletesFile) // NOLINT
    {
        std::string filePath =
            Common::FileSystem::join(m_fileSystem->currentWorkingDirectory(), "removeDirectoryDeletesFile.txt");

        std::string testContent("HelloWorld");

        m_fileSystem->writeFile(filePath, testContent);

        EXPECT_TRUE(m_fileSystem->isFile(filePath));

        m_fileSystem->removeFileOrDirectory(filePath);

        EXPECT_FALSE(m_fileSystem->exists(filePath));
    }

    TEST_F(FileSystemImplTest, removeDirectoryDeletesEmptyDirectory) // NOLINT
    {
        std::string filePath =
            Common::FileSystem::join(m_fileSystem->currentWorkingDirectory(), "removeDirectoryDeletesEmptyDirectory");

        m_fileSystem->makedirs(filePath);

        EXPECT_TRUE(m_fileSystem->isDirectory(filePath));

        m_fileSystem->removeFileOrDirectory(filePath);

        EXPECT_FALSE(m_fileSystem->exists(filePath));
    }

    TEST_F(FileSystemImplTest, removeDirectoryDeletesNonEmptyDirectory) // NOLINT
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

    TEST_F(FileSystemImplTest, makeAbsoluteReturnsArgumentWithArgumentIsAbsolute) // NOLINT
    {
        Path original = "/foo";
        Path result = Common::FileSystem::fileSystem()->makeAbsolute(original);
        EXPECT_EQ(original, result);
    }

    TEST_F(FileSystemImplTest, makeAbsoluteReturnsAbsolutePathWhenArgumentIsRelative) // NOLINT
    {
        Path original = "foo";
        Path result = Common::FileSystem::fileSystem()->makeAbsolute(original);
        EXPECT_EQ(result[0], '/');
    }

    TEST_F(FileSystemImplTest, readLinkReturnsLinkedToPathWhenCalledOnSymbolicLink) // NOLINT
    {
        Path findPath = m_fileSystem->readlink("/proc/self/exe");
        ASSERT_NE(findPath.size(), 0);
    }

    TEST_F(FileSystemImplTest, readLinkReturnsEmptyWhenCalledOnNonExistentFile) // NOLINT
    {
        Path findPath = m_fileSystem->readlink("/Not/a/file/path");
        ASSERT_EQ(findPath.size(), 0);
    }

    TEST_F(FileSystemImplTest, readLinkReturnsEmptyWhenCalledOnNonLinkFile) // NOLINT
    {
        Path findPath = m_fileSystem->readlink("/bin/bash");
        ASSERT_EQ(findPath.size(), 0);
    }

} // namespace
