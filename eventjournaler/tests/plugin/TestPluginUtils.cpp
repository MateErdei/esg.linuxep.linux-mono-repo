/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/FileSystem/IFileSystem.h>
#include <Common/Helpers/LogInitializedTests.h>
#include <Common/Helpers/MockFileSystem.h>
#include <Common/Helpers/FileSystemReplaceAndRestore.h>
#include <Common/Logging/ConsoleLoggingSetup.h>
#include <gtest/gtest.h>
#include <modules/pluginimpl/Logger.h>
#include <pluginimpl/DiskManager.h>
#include <Common/Helpers/TempDir.h>
#include <future>

class DiskManagerTest : public LogInitializedTests
{

};

TEST_F(DiskManagerTest, compress)
{
    Plugin::DiskManager disk;
    EXPECT_NO_THROW(disk.compressFile("/tmp/myfile.txt"));
}

TEST_F(DiskManagerTest, weCanDeleteOldJournalFiles) // NOLINT
{
    Tests::TempDir tempDir;
    tempDir.makeDirs("Root/subdir");
    tempDir.createFile("Root/file1.xz", "hello");
    tempDir.createFile("Root/file2.xz", "hello");
    tempDir.createFile("Root/subdir/file1.xz", "hello");
    Plugin::DiskManager disk;
    disk.deleteOldJournalFiles(tempDir.absPath("Root"),15);

    auto fs = Common::FileSystem::fileSystem();
    std::vector<Path> fileList = fs->listAllFilesInDirectoryTree(tempDir.absPath("Root"));

    EXPECT_EQ(fileList.size(), 0);
}

TEST_F(DiskManagerTest, weCanSortJournalFiles) // NOLINT
{
    // subject-uniquiId1-uniID2-timestanp-timestamp.xz

    auto mockFileSystem = new StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>{ mockFileSystem });
    std::vector<std::string> files = {"System-00000000005b067e-00000000005b067e-131803877044481699-131803877044481699.xz",
                                       "System-00000000005b067e-00000000005b067e-131803877044481800-131803877044481699.xz",
                                       "131803877044481200-131803877044481699.xz",
                                       "tmp.bin"};

    EXPECT_CALL(*mockFileSystem, isDirectory("randompath")).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, listAllFilesInDirectoryTree(_)).WillOnce(Return(files));
    EXPECT_CALL(*mockFileSystem, fileSize(_)).WillRepeatedly(Return(20));
    Plugin::DiskManager disk;
    std::list<Plugin::DiskManager::SubjectFileInfo> list = disk.getSortedListOFCompressedJournalFiles("randompath");
    // uncompressed file is ignored
    EXPECT_EQ(list.size(), 3);
    // in order to check ordering of elements in list iterate through the list to create vector with opposite ordering of list
    std::vector<std::string> sortedFiles;
    for (const auto& file : list)
    {
        sortedFiles.push_back(file.filepath);
    }
    // newest file first, malformed file at the end since the vector is in the oppositie order of the ordered list
    EXPECT_EQ(sortedFiles[0],"System-00000000005b067e-00000000005b067e-131803877044481800-131803877044481699.xz");
    EXPECT_EQ(sortedFiles[1],"System-00000000005b067e-00000000005b067e-131803877044481699-131803877044481699.xz");
    EXPECT_EQ(sortedFiles[2],"131803877044481200-131803877044481699.xz");

}
