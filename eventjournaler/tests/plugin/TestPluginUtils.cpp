/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/FileSystem/IFileSystem.h>
#include <Common/Helpers/LogInitializedTests.h>
#include <Common/Helpers/MockFileSystem.h>
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
    disk.deleteOldJournalFiles(tempDir.absPath("Root"),5);
    std::vector<Path> fileList;
    auto fs = Common::FileSystem::fileSystem();
    fs->listAllFilesInDirectoryTree(fileList, tempDir.absPath("Root"));

    EXPECT_EQ(fileList.size(), 0);

}
