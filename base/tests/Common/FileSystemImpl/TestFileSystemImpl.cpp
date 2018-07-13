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
                {"",""}
        };
        for ( auto & pair: values)
        {
            EXPECT_EQ(m_fileSystem->basename(pair.first), pair.second);
        }
    }


    TEST_F(FileSystemImplTest, joinReturnsCorrectValueWithNoSeporator) //NOLINT
    {
        std::string path1("/tmp");
        std::string path2("tempfile.txt");
        std::string expectedValue("/tmp/tempfile.txt");
        EXPECT_EQ(m_fileSystem->join(path1, path2), expectedValue);
    }

    TEST_F(FileSystemImplTest, joinReturnsCorrectValueWithPath1TrailingSeporator) //NOLINT
    {
        std::string path1("/tmp/");
        std::string path2("tempfile.txt");
        std::string expectedValue("/tmp/tempfile.txt");
        EXPECT_EQ(m_fileSystem->join(path1, path2), expectedValue);
    }

    TEST_F(FileSystemImplTest, joinReturnsCorrectValueWithPath2StartingWithSeporator) //NOLINT
    {
        std::string path1("/tmp");
        std::string path2("/tempfile.txt");
        std::string expectedValue("/tmp/tempfile.txt");
        EXPECT_EQ(m_fileSystem->join(path1, path2), expectedValue);
    }

    TEST_F(FileSystemImplTest, joinReturnsCorrectValueWithPath1TrailingSeporatorAndPath2StartingWithSeporator) //NOLINT
    {
        std::string path1("/tmp/");
        std::string path2("/tempfile.txt");
        std::string expectedValue("/tmp/tempfile.txt");
        EXPECT_EQ(m_fileSystem->join(path1, path2), expectedValue);
    }

    TEST_F(FileSystemImplTest, joinWithDotSlashReturnsExpectedPath) //NOLINT
    {
        std::string path1("/tmp/");
        std::string path2("./tempfile.txt");
        std::string expectedValue("/tmp/tempfile.txt");
        EXPECT_EQ(m_fileSystem->join(path1, path2), expectedValue);
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
        std::string filePath = m_fileSystem->join(m_fileSystem->currentWorkingDirectory(), "ReadWriteFileTest.txt");

        std::string testContent("HelloWorld");

        m_fileSystem->writeFile(filePath, testContent);

        EXPECT_EQ(m_fileSystem->readFile(filePath), testContent);
        ::remove(filePath.c_str());
    }

    TEST_F(FileSystemImplTest, writeFileUsingDirectoryPathShouldThrow) // NOLINT
    {
        std::string directroryPath = m_fileSystem->join(m_fileSystem->currentWorkingDirectory(), "WriteToDirectoryTest");

        mkdir(directroryPath.c_str(), 0700);

        std::string testContent("HelloWorld");

        EXPECT_THROW(m_fileSystem->writeFile(directroryPath, testContent), IFileSystemException);

        ::rmdir(directroryPath.c_str());
    }

    TEST_F(FileSystemImplTest, readFileThatDoesNotExistsShouldThrow) // NOLINT
    {
        std::string filePath = m_fileSystem->join(m_fileSystem->currentWorkingDirectory(), "ReadFileTest.txt");
        EXPECT_THROW(m_fileSystem->readFile(filePath), IFileSystemException);

    }

    TEST_F(FileSystemImplTest, readFileUsingDirectoryPathShouldThrow) // NOLINT
    {
        std::string directroryPath = m_fileSystem->join(m_fileSystem->currentWorkingDirectory(), "WriteToDirectoryTest");

        mkdir(directroryPath.c_str(), 0700);
        EXPECT_THROW(m_fileSystem->readFile(directroryPath), IFileSystemException);

        ::rmdir(directroryPath.c_str());
    }

    TEST_F(FileSystemImplTest, atomicWriteStoresExpectedContentForFile) // NOLINT
    {
        std::string filePath = m_fileSystem->join(m_fileSystem->currentWorkingDirectory(), "AtomicWrite.txt");

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
            EXPECT_EQ(m_fileSystem->dirName(pair.first), pair.second);
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

    TEST_F( FileSystemImplTest, canCreateDirectories) // NOLINT
    {
        Tests::TempDir tempdir("","FileSystemImplTest_canCreateDirectories");
        Path A = tempdir.absPath("A");
        Path B = m_fileSystem->join(A, "B");
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

}

