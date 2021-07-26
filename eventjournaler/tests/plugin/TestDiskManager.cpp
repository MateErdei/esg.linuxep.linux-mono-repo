/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/FileSystem/IFileSystem.h>
#include <Common/Helpers/LogInitializedTests.h>
#include <Common/Helpers/MockFileSystem.h>
#include <Common/Helpers/FileSystemReplaceAndRestore.h>
#include <Common/Logging/ConsoleLoggingSetup.h>
#include <gtest/gtest.h>

#include <pluginimpl/DiskManager.h>
#include <Common/Helpers/TempDir.h>


class DiskManagerTest : public LogInitializedTests
{

};

TEST_F(DiskManagerTest, compressFileDoesNotThrowWhenFileDoesNotExist )
{
    Plugin::DiskManager disk;
    EXPECT_NO_THROW(disk.compressFile("/tmp/myfile.txt"));
}

TEST_F(DiskManagerTest, weCanDeleteOldJournalFiles) // NOLINT
{
    Tests::TempDir tempDir;
    tempDir.makeDirs("Root/subdir");
    tempDir.makeDirs("Root/subdir1");
    tempDir.createFile("Root/subdir1/file1.xz", "hello");
    tempDir.createFile("Root/subdir1/file2.xz", "hello");
    tempDir.createFile("Root/subdir/file1.xz", "hello");
    Plugin::DiskManager disk;
    disk.deleteOldJournalFiles(tempDir.absPath("Root"),10, 20);

    auto fs = Common::FileSystem::fileSystem();
    std::vector<Path> fileList = fs->listAllFilesInDirectoryTree(tempDir.absPath("Root"));

    EXPECT_EQ(fileList.size(), 1);
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
    std::vector<Plugin::DiskManager::SubjectFileInfo> list = disk.getSortedListOFCompressedJournalFiles("randompath");
    // uncompressed file is ignored
    EXPECT_EQ(list.size(), 3);

    EXPECT_EQ(list[2].filepath,"System-00000000005b067e-00000000005b067e-131803877044481800-131803877044481699.xz");
    EXPECT_EQ(list[1].filepath,"System-00000000005b067e-00000000005b067e-131803877044481699-131803877044481699.xz");
    EXPECT_EQ(list[0].filepath,"131803877044481200-131803877044481699.xz");

}
