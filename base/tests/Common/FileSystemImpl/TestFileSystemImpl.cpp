/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "Common/FileSystem/IFileSystem.h"
#include "Common/FileSystem/IFileSystemException.h"
#include "Common/FileSystemImpl/FileSystemImpl.h"
#include "../TestHelpers/TempDir.h"

#include <fstream>

using namespace Common::FileSystem;
namespace
{

    class FileSystemImplTest : public ::testing::Test
    {
    public:
        std::unique_ptr<IFileSystem> m_fileSystem;
        void SetUp() override
        {
            m_fileSystem.reset(new FileSystemImpl());
        }
    };

    TEST_F( FileSystemImplTest, basenameReturnsCorrectMatchingValue) //NOLINT
    {
        std::vector<std::pair<std::string, std::string>> values = {
                {"/tmp/tmpfile.txt", "tmpfile.txt" },
                {"/tmp/tmpfile/", "" },
                {"/tmp/tmp1/tmp2/tmp3/tmpfile.txt", "tmpfile.txt" },
                {"/tmp", "tmp"},
                {"tmp", "tmp"},
                {"appid-25_policy.xml", "appid-25_policy.xml"},
                {"",""}
        };
        for ( auto & pair: values)
        {
            EXPECT_EQ(Common::FileSystem::basename(pair.first), pair.second);
        }
    }


    TEST_F(FileSystemImplTest, joinReturnsCorrectValueWithNoSeporator) //NOLINT
    {
        std::string path1("/tmp");
        std::string path2("tempfile.txt");
        std::string expectedValue("/tmp/tempfile.txt");
        EXPECT_EQ(Common::FileSystem::join(path1, path2), expectedValue);
    }

    TEST_F(FileSystemImplTest, joinReturnsCorrectValueWithPath1TrailingSeporator) //NOLINT
    {
        std::string path1("/tmp/");
        std::string path2("tempfile.txt");
        std::string expectedValue("/tmp/tempfile.txt");
        EXPECT_EQ(Common::FileSystem::join(path1, path2), expectedValue);
    }

    TEST_F(FileSystemImplTest, joinReturnsCorrectValueWithPath2StartingWithSeporator) //NOLINT
    {
        // python -c 'import os; print os.path.join("/foo","/bar")' -> "/bar

        std::string path1("/tmp");
        std::string path2("/tempfile.txt");
        std::string expectedValue("/tempfile.txt");
        EXPECT_EQ(Common::FileSystem::join(path1, path2), expectedValue);
    }

    TEST_F(FileSystemImplTest, joinReturnsCorrectValueWithPath1TrailingSeporatorAndPath2StartingWithSeporator) //NOLINT
    {
        // python -c 'import os; print os.path.join("/tmp/","/tempfile.txt")' -> /tempfile.txt
        std::string path1("/tmp/");
        std::string path2("/tempfile.txt");
        std::string expectedValue("/tempfile.txt");
        EXPECT_EQ(Common::FileSystem::join(path1, path2), expectedValue);
    }

    TEST_F(FileSystemImplTest, joinWithDotSlashReturnsExpectedPath) //NOLINT
    {
        std::string path1("/tmp/");
        std::string path2("./tempfile.txt");
        std::string expectedValue("/tmp/tempfile.txt");
        EXPECT_EQ(Common::FileSystem::join(path1, path2), expectedValue);
    }

    TEST_F(FileSystemImplTest, joinWithDotSlashSlashReturnsExpectedPath) //NOLINT
    {
        // python -c 'import os; print os.path.join("/tmp/",".//tempfile.txt")' -> /tmp/.//tempfile.txt
        // python doesn't have any special handling for ./ in join
        // python -c 'import os; print os.path.normpath(os.path.join("/tmp/",".//tempfile.txt"))' -> /tmp/tempfile.txt

        std::string path1("/tmp/");
        std::string path2(".//tempfile.txt");
        std::string expectedValue("/tmp/tempfile.txt");
        EXPECT_EQ(Common::FileSystem::join(path1, path2), expectedValue);
    }

    TEST_F(FileSystemImplTest, joinWithEmptyStringsReturnsExpectedPath) //NOLINT
    {
        // python -c 'import os; print os.path.join("","")' -> ""
        std::string path1;
        std::string path2;
        std::string expectedValue;
        EXPECT_EQ(Common::FileSystem::join(path1, path2), expectedValue);
    }

    TEST_F(FileSystemImplTest, joinWithEmptyPath1AndAbsolutePath2ReturnsPath2) //NOLINT
    {
        Path path1;
        Path path2("/foo/bar");
        Path& expectedValue = path2;
        EXPECT_EQ(Common::FileSystem::join(path1, path2), expectedValue);
    }

    TEST_F(FileSystemImplTest, joinWithEmptyPath1AndRelativePath2ReturnsExpectedPath) //NOLINT
    {
        // python -c 'import os; print os.path.join("","foo/bar")' -> foo/bar
        Path path1;
        Path path2("foo/bar");
        Path expectedValue("foo/bar");
        EXPECT_EQ(Common::FileSystem::join(path1, path2), expectedValue);
    }

    TEST_F(FileSystemImplTest, joinWithEmptyPath2ReturnsExpectedPath) //NOLINT
    {
        // python -c 'import os; print os.path.join("foo/bar","")' -> foo/bar
        Path path1("foo/bar");
        Path path2;
        Path expectedValue("foo/bar");
        EXPECT_EQ(Common::FileSystem::join(path1, path2), expectedValue);
    }

    TEST_F(FileSystemImplTest, joinWithTwoRelativePathsReturnsARelativePath) //NOLINT
    {
        // python -c 'import os; print os.path.join("foo","bar")'
        Path path1("foo");
        Path path2("bar");
        Path expectedValue("foo/bar");
        EXPECT_EQ(Common::FileSystem::join(path1, path2), expectedValue);
    }

    TEST_F(FileSystemImplTest, existReturnsTrueWhenFileExists) //NOLINT
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

    TEST_F(FileSystemImplTest, isDirectoryReturnsFalseWhenDirectoryDoesNotExist) // NOLINT
    {
        EXPECT_FALSE(m_fileSystem->isDirectory("/etc/ghdfoijsdfgjhszdgvzjsdjvcjsdhzc"));
    }

    TEST_F(FileSystemImplTest, isDirectoryReturnsTrueWhenPathGivenIsDirectory) // NOLINT
    {
        EXPECT_TRUE(m_fileSystem->isDirectory("/etc"));
    }

    TEST_F(FileSystemImplTest, currentWorkingDirectoryReturnsCorrectValue) // NOLINT
    {
        std::string startingDirectory = m_fileSystem->currentWorkingDirectory();

        std::string expectedPath("/tmp");
        chdir(expectedPath.c_str());

        EXPECT_EQ(m_fileSystem->currentWorkingDirectory(), expectedPath);

        chdir(startingDirectory.c_str());

        EXPECT_EQ(m_fileSystem->currentWorkingDirectory(), startingDirectory);
    }

    TEST_F(FileSystemImplTest, readWriteStoresAndReadsExpectedContentFromFile) // NOLINT
    {
        std::string filePath = Common::FileSystem::join(m_fileSystem->currentWorkingDirectory(), "ReadWriteFileTest.txt");

        std::string testContent("HelloWorld");

        m_fileSystem->writeFile(filePath, testContent);

        EXPECT_EQ(m_fileSystem->readFile(filePath), testContent);
        ::remove(filePath.c_str());
    }

    TEST_F(FileSystemImplTest, writeFileUsingDirectoryPathShouldThrow) // NOLINT
    {
        std::string directroryPath = Common::FileSystem::join(m_fileSystem->currentWorkingDirectory(), "WriteToDirectoryTest");

        mkdir(directroryPath.c_str(), 0700);

        std::string testContent("HelloWorld");

        EXPECT_THROW(m_fileSystem->writeFile(directroryPath, testContent), IFileSystemException);

        ::rmdir(directroryPath.c_str());
    }

    TEST_F(FileSystemImplTest, readFileThatDoesNotExistsShouldThrow) // NOLINT
    {
        std::string filePath = Common::FileSystem::join(m_fileSystem->currentWorkingDirectory(), "ReadFileTest.txt");
        EXPECT_THROW(m_fileSystem->readFile(filePath), IFileSystemException);

    }

    TEST_F(FileSystemImplTest, readFileUsingDirectoryPathShouldThrow) // NOLINT
    {
        std::string directroryPath = Common::FileSystem::join(m_fileSystem->currentWorkingDirectory(), "WriteToDirectoryTest");

        mkdir(directroryPath.c_str(), 0700);
        EXPECT_THROW(m_fileSystem->readFile(directroryPath), IFileSystemException);

        ::rmdir(directroryPath.c_str());
    }

    TEST_F(FileSystemImplTest, atomicWriteStoresExpectedContentForFile) // NOLINT
    {
        std::string filePath = Common::FileSystem::join(m_fileSystem->currentWorkingDirectory(), "AtomicWrite.txt");

        std::string testContent("HelloWorld");

        m_fileSystem->writeFileAtomically(filePath, testContent, m_fileSystem->currentWorkingDirectory() );

        EXPECT_EQ(m_fileSystem->readFile(filePath), testContent);
        ::remove(filePath.c_str());
    }

    TEST_F( FileSystemImplTest, dirNameReturnsCorrectMatchingValue) // NOLINT
    {
        std::vector<std::pair<std::string, std::string>> values = {
                {"/tmp/tmpfile.txt", "/tmp" },
                {"/tmp/tmpfile/", "/tmp" },
                {"/tmp/tmp1/tmp2/tmp3/tmpfile.txt", "/tmp/tmp1/tmp2/tmp3" },
                {"/tmp", ""},
                {"tmp", ""},
                {"",""}
        };
        for ( auto & pair: values)
        {
            EXPECT_EQ(Common::FileSystem::dirName(pair.first), pair.second);
        }
    }

    TEST_F( FileSystemImplTest, isExecutableReturnTrueForExecutables) // NOLINT
    {
        EXPECT_TRUE( m_fileSystem->isExecutable("/bin/bash"));
    }

    TEST_F( FileSystemImplTest, isExecutableReturnFalseForNonExecutables) // NOLINT
    {
        EXPECT_FALSE( m_fileSystem->isExecutable("/etc/passwd"));
    }

    TEST_F( FileSystemImplTest, isExecutableReturnFalseForNonExistingFiles) // NOLINT
    {
        EXPECT_FALSE( m_fileSystem->isExecutable("/tmp/thisfiledoesnotexist"));
    }

    TEST_F(FileSystemImplTest, listFilesReturnsAListOfPathsOnlyContainingRegularFiles) // NOLINT
    {
        Tests::TempDir tempDir;
        tempDir.makeDirs("Root/subdir");
        tempDir.createFile("Root/file1", "hello" );
        tempDir.createFile("Root/file2", "hello" );

        std::vector<std::string> fileList = m_fileSystem->listFiles(tempDir.absPath("Root"));

        EXPECT_EQ(fileList.size(), 2);

        std::sort(fileList.begin(), fileList.end());

        EXPECT_EQ(fileList[0], tempDir.absPath("Root/file1"));
        EXPECT_EQ(fileList[1], tempDir.absPath("Root/file2"));
    }

    TEST_F( FileSystemImplTest, canCreateDirectories) // NOLINT
    {
        Tests::TempDir tempdir("","FileSystemImplTest_canCreateDirectories");
        Path A = tempdir.absPath("A");
        Path B = Common::FileSystem::join(A, "B");
        m_fileSystem->makedirs(B);
        EXPECT_TRUE(m_fileSystem->isDirectory(A));
        EXPECT_TRUE(m_fileSystem->isDirectory(B));
    }

    TEST_F( FileSystemImplTest, copyFile) // NOLINT
    {
        Tests::TempDir tempdir("","FileSystemImplTest_copyFile");
        Path A = tempdir.absPath("A");
        Path B = tempdir.absPath("B");
        tempdir.createFile("A","FOOBAR");
        m_fileSystem->copyFile(A,B);
        EXPECT_TRUE(m_fileSystem->exists(B));
        std::string content = m_fileSystem->readFile(B);
        EXPECT_EQ(content,"FOOBAR");
    }

    TEST_F( FileSystemImplTest, copyFileDoesNotExist) // NOLINT
    {
        Tests::TempDir tempdir("","FileSystemImplTest_copyFile");
        Path A = tempdir.absPath("A");
        Path B = tempdir.absPath("B");
        EXPECT_ANY_THROW(m_fileSystem->copyFile(A,B));
        EXPECT_FALSE(m_fileSystem->exists(B));
    }

    TEST_F( FileSystemImplTest, copyFileNonAccessibleDest) // NOLINT
    {
        Tests::TempDir tempdir("","FileSystemImplTest_copyFile");
        Path A = tempdir.absPath("A");
        Path B = Common::FileSystem::join("NotADirectory", "NotAFile");
        EXPECT_ANY_THROW(m_fileSystem->copyFile(A,B));
        EXPECT_FALSE(m_fileSystem->exists(B));
    }

    TEST_F(FileSystemImplTest, removeFileDeletesFile) // NOLINT
    {
        std::string filePath = Common::FileSystem::join(m_fileSystem->currentWorkingDirectory(), "remove.txt");

        std::string testContent("HelloWorld");

        m_fileSystem->writeFile(filePath, testContent);

        EXPECT_TRUE(m_fileSystem->isFile(filePath));

        m_fileSystem->removeFile(filePath);

        EXPECT_FALSE(m_fileSystem->exists(filePath));
    }

    TEST_F(FileSystemImplTest, removeFileThrowsIfFileDoesNotExist) // NOLINT
    {
        std::string filePath = Common::FileSystem::join(m_fileSystem->currentWorkingDirectory(), "remove.txt");
        ASSERT_FALSE(m_fileSystem->exists(filePath));
        EXPECT_THROW(m_fileSystem->removeFile(filePath), IFileSystemException);
    }

    TEST_F(FileSystemImplTest, makeAbsoluteReturnsArgumentWithArgumentIsAbsolute) // NOLINT
    {
        Path original = "/foo";
        Path result = Common::FileSystem::fileSystem()->make_absolute(original);
        EXPECT_EQ(original, result);
    }

    TEST_F(FileSystemImplTest, makeAbsoluteReturnsAbsolutePathWhenArgumentIsRelative) // NOLINT
    {
        Path original = "foo";
        Path result = Common::FileSystem::fileSystem()->make_absolute(original);
        EXPECT_EQ(result[0], '/');
    }

}

