/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gmock/gmock-matchers.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <modules/Diagnose/diagnose/GatherFiles.h>
#include <modules/Diagnose/diagnose/SystemCommands.h>
#include <tests/Common/Helpers/MockFileSystem.h>
#include <tests/Common/Helpers/TempDir.h>
#include <tests/Common/Helpers/LogInitializedTests.h>

class TestGatherFiles : public LogOffInitializedTests
{

};


TEST_F(TestGatherFiles, copyFileIntoDirectoryNoDuplicateTargetFile) // NOLINT
{
    Tests::TempDir tempdir("", "dir1");
    Tests::TempDir tempdir2("", "dir2");
    tempdir.createFile("A", "FOOBAR");
    Path src = tempdir.absPath("A");
    Path dest = tempdir2.absPath("A");

    auto mockFileSystem = new StrictMock<MockFileSystem>();
    std::unique_ptr<MockFileSystem> mockIFileSystemPtr = std::unique_ptr<MockFileSystem>(mockFileSystem);

    EXPECT_CALL(*mockFileSystem, isFile(src)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, exists(dest)).WillOnce(Return(false));
    EXPECT_CALL(*mockFileSystem, copyFile(src, dest));

    diagnose::GatherFiles gather(std::move(mockIFileSystemPtr));

    gather.copyFileIntoDirectory(src, tempdir2.dirPath());
}

// This test will try to copy file dir1/A to dir2/ except that we mock calls to make gather files see that both dir2/A
// and dir2/A.1 exist and it will work out to save to dir2/A.2
TEST_F(TestGatherFiles, copyFileIntoDirectoryTwoDuplicateTargetFiles) // NOLINT
{
    Tests::TempDir tempdir("", "dir1");
    Tests::TempDir tempdir2("", "dir2");
    tempdir.createFile("A", "FOOBAR");
    Path src = tempdir.absPath("A");
    Path dest = tempdir2.absPath("A");

    auto mockFileSystem = new StrictMock<MockFileSystem>();
    std::unique_ptr<MockFileSystem> mockIFileSystemPtr = std::unique_ptr<MockFileSystem>(mockFileSystem);

    EXPECT_CALL(*mockFileSystem, isFile(src)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, exists(dest)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, exists(dest + ".1")).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, exists(dest + ".2")).WillOnce(Return(false));
    EXPECT_CALL(*mockFileSystem, copyFile(src, dest + ".2"));

    diagnose::GatherFiles gather(std::move(mockIFileSystemPtr));
    gather.copyFileIntoDirectory(src, tempdir2.dirPath());
}

// This test will try to copy file dir1/A to dir2/ except that we mock calls to make gather files see that dir2/A
// exists and it will work out to save to dir2/A.1 we also copy dir1/B to dir2/B afterwards.
TEST_F(TestGatherFiles, copyFileIntoDirectoryOneDuplicateOneNot) // NOLINT
{
    std::string filenameA = "A";
    std::string filenameB = "B";
    Tests::TempDir tempdir("", "dir1");
    Tests::TempDir tempdir2("", "dir2");
    tempdir.createFile(filenameA, "FOOBAR");
    tempdir.createFile(filenameB, "FOOBAR");
    Path srcA = tempdir.absPath(filenameA);
    Path srcB = tempdir.absPath(filenameB);
    Path destA = tempdir2.absPath(filenameA);
    Path destB = tempdir2.absPath(filenameB);

    auto mockFileSystem = new StrictMock<MockFileSystem>();
    std::unique_ptr<MockFileSystem> mockIFileSystemPtr = std::unique_ptr<MockFileSystem>(mockFileSystem);

    EXPECT_CALL(*mockFileSystem, isFile(srcA)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, isFile(srcB)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, exists(destA)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, exists(destA + ".1")).WillOnce(Return(false));
    EXPECT_CALL(*mockFileSystem, exists(destB)).WillOnce(Return(false));
    EXPECT_CALL(*mockFileSystem, copyFile(srcA, destA + ".1"));
    EXPECT_CALL(*mockFileSystem, copyFile(srcB, destB));

    diagnose::GatherFiles gather(std::move(mockIFileSystemPtr));
    gather.copyFileIntoDirectory(srcA, tempdir2.dirPath());
    gather.copyFileIntoDirectory(srcB, tempdir2.dirPath());
}