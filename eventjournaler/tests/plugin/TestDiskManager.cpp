// Copyright 2021-2024 Sophos Limited. All rights reserved.

#include "Common/FileSystem/IFileSystem.h"
#include "base/tests/Common/Helpers/FileSystemReplaceAndRestore.h"
#include "base/tests/Common/Helpers/LogInitializedTests.h"
#include "base/tests/Common/Helpers/MockFileSystem.h"
#include "base/tests/Common/Helpers/TempDir.h"
#include "pluginimpl/DiskManager.h"

#include <gtest/gtest.h>


class DiskManagerTest : public LogInitializedTests
{
    void TearDown() override
    {
        Tests::restoreFileSystem();
    }
};


TEST_F(DiskManagerTest, weCanDeleteOldJournalFiles)
{
    Tests::TempDir tempDir;
    tempDir.makeDirs("Root/producer/subdir");
    tempDir.makeDirs("Root/producer/subdir1");
    tempDir.makeDirs("Root/producer2/subdir1");

    tempDir.createFile("Root/producer/subdir1/file1.xz", "hello");
    tempDir.createFile("Root/producer/subdir1/file2.xz", "hello");
    tempDir.createFile("Root/producer2/subdir1/file1.xz", "hello");
    Plugin::DiskManager disk;
    disk.deleteOldJournalFiles(tempDir.absPath("Root"),10, 20);

    auto fs = Common::FileSystem::fileSystem();
    std::vector<Path> fileList = fs->listAllFilesInDirectoryTree(tempDir.absPath("Root"));

    EXPECT_EQ(fileList.size(), 1);
}

TEST_F(DiskManagerTest, weCanSortJournalFiles)
{
    // subject-uniquiId1-uniID2-timestanp-timestamp.xz

    auto mockFileSystem = std::make_unique<StrictMock<MockFileSystem>>();
    std::vector<std::string> files = {"System-00000000005b067e-00000000005b067e-131803877044481699-131803877044481699.xz",
                                       "System-00000000005b067e-00000000005b067e-131803877044481800-131803877044481699.xz",
                                       "131803877044481200-131803877044481699.xz",
                                       "tmp.bin"};

    EXPECT_CALL(*mockFileSystem, isDirectory("randompath")).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, listAllFilesInDirectoryTree(_)).WillOnce(Return(files));
    EXPECT_CALL(*mockFileSystem, fileSize(_)).WillRepeatedly(Return(20));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(mockFileSystem) };
    Plugin::DiskManager disk;
    std::vector<Plugin::DiskManager::SubjectFileInfo> list = disk.getSortedListOFCompressedJournalFiles("randompath");
    // uncompressed file is ignored
    EXPECT_EQ(list.size(), 3);

    EXPECT_EQ(list[2].filepath,"System-00000000005b067e-00000000005b067e-131803877044481800-131803877044481699.xz");
    EXPECT_EQ(list[1].filepath,"System-00000000005b067e-00000000005b067e-131803877044481699-131803877044481699.xz");
    EXPECT_EQ(list[0].filepath,"131803877044481200-131803877044481699.xz");

}
