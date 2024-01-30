// Copyright 2020-2023 Sophos Limited. All rights reserved.

#include <gmock/gmock-matchers.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "Diagnose/diagnose/GatherFiles.h"
#include "Diagnose/diagnose/SystemCommands.h"
#include "tests/Common/Helpers/MockFileSystem.h"
#include "tests/Common/Helpers/TempDir.h"
#include "tests/Common/Helpers/LogInitializedTests.h"

class TestGatherFiles : public LogOffInitializedTests
{

};


TEST_F(TestGatherFiles, copyFileIntoDirectoryNoDuplicateTargetFile)
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
TEST_F(TestGatherFiles, copyFileIntoDirectoryTwoDuplicateTargetFiles)
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
TEST_F(TestGatherFiles, copyFileIntoDirectoryOneDuplicateOneNot)
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

TEST_F(TestGatherFiles, gatherAuditLogsTest)
{
    std::string filenameA = "audit.log";
    std::string filenameB = "audit.log.0";
    std::string filenameC = "audit.log.1";

    Tests::TempDir tempdir("", "audit");
    Tests::TempDir tempdir2("", "destination");

    Path srcA = tempdir.absPath(filenameA);
    Path srcB = tempdir.absPath(filenameB);
    Path srcC = tempdir.absPath(filenameC);
    Path destA = tempdir2.absPath(filenameA);
    Path destB = tempdir2.absPath(filenameB);
    Path destC = tempdir2.absPath(filenameC);
    std::vector<std::string> fileList{ srcB, srcC, srcA};

    Sequence s;
    auto mockFileSystem = new StrictMock<MockFileSystem>();
    std::unique_ptr<MockFileSystem> mockIFileSystemPtr = std::unique_ptr<MockFileSystem>(mockFileSystem);

    EXPECT_CALL(*mockFileSystem, isDirectory(tempdir.dirPath())).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, listFiles(tempdir.dirPath())).WillOnce(Return(fileList));
    EXPECT_CALL(*mockFileSystem, isFile(srcA)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, isFile(srcB)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, isFile(srcC)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, exists(destA)).WillOnce(Return(false));
    EXPECT_CALL(*mockFileSystem, exists(destB)).WillOnce(Return(false));
    EXPECT_CALL(*mockFileSystem, exists(destC)).WillOnce(Return(false));

    // testing the sort order as we copy after we sort
    EXPECT_CALL(*mockFileSystem, copyFile(srcA, destA)).InSequence(s);
    EXPECT_CALL(*mockFileSystem, copyFile(srcB, destB)).InSequence(s);
    EXPECT_CALL(*mockFileSystem, copyFile(srcC, destC)).InSequence(s);

    diagnose::GatherFiles gather(std::move(mockIFileSystemPtr));
    gather.gatherAuditLogs(tempdir.dirPath(), tempdir2.dirPath());
}

TEST_F(TestGatherFiles, gatherAuditLogsTest12Files)
{
    Tests::TempDir tempdir("", "audit");
    Tests::TempDir tempdir2("", "destination");
    std::vector<Path> fileList;

    for(int i = 10; i >= 0 ; i--)
    {
        std::string filename = "audit.log.";
        filename += std::to_string(i);
        Path src = tempdir.absPath(filename);
        fileList.push_back(src);
    }

    fileList.push_back("audit.log");
    std::string auditDot11 = fileList.at(0);
    std::string auditDot12 = fileList.at(1);
    auto mockFileSystem = new StrictMock<MockFileSystem>();
    std::unique_ptr<MockFileSystem> mockIFileSystemPtr = std::unique_ptr<MockFileSystem>(mockFileSystem);

    EXPECT_CALL(*mockFileSystem, isDirectory(tempdir.dirPath())).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, listFiles(tempdir.dirPath())).WillOnce(Return(fileList));
    EXPECT_CALL(*mockFileSystem, isFile(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockFileSystem, exists(_)).WillRepeatedly(Return(false));
    EXPECT_CALL(*mockFileSystem, copyFile(_, _)).Times(10);

    // making sure these files are not being copied
    EXPECT_CALL(*mockFileSystem, copyFile(auditDot11, _)).Times(0);
    EXPECT_CALL(*mockFileSystem, copyFile(auditDot12, _)).Times(0);

    diagnose::GatherFiles gather(std::move(mockIFileSystemPtr));
    gather.gatherAuditLogs(tempdir.dirPath(), tempdir2.dirPath());
}

TEST_F(TestGatherFiles, gatherAuditLogs1InvalidAuditFile)
{
    std::string filenameA = "audit.log";
    std::string filenameB = "audit.log.0";
    std::string filenameC = "audit.log.1....";

    Tests::TempDir tempdir("", "audit");
    Tests::TempDir tempdir2("", "destination");

    Path srcA = tempdir.absPath(filenameA);
    Path srcB = tempdir.absPath(filenameB);
    Path srcC = tempdir.absPath(filenameC);
    Path destA = tempdir2.absPath(filenameA);
    Path destB = tempdir2.absPath(filenameB);
    Path destC = tempdir2.absPath(filenameC);
    std::vector<std::string> fileList{ srcB, srcC, srcA};

    Sequence s;
    auto mockFileSystem = new StrictMock<MockFileSystem>();
    std::unique_ptr<MockFileSystem> mockIFileSystemPtr = std::unique_ptr<MockFileSystem>(mockFileSystem);

    EXPECT_CALL(*mockFileSystem, isDirectory(tempdir.dirPath())).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, listFiles(tempdir.dirPath())).WillOnce(Return(fileList));
    EXPECT_CALL(*mockFileSystem, isFile(srcA)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, isFile(srcB)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, isFile(srcC)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, exists(destA)).WillOnce(Return(false));
    EXPECT_CALL(*mockFileSystem, exists(destB)).WillOnce(Return(false));
    EXPECT_CALL(*mockFileSystem, exists(destC)).WillOnce(Return(false));

    // testing the sort order as we copy after we sort
    EXPECT_CALL(*mockFileSystem, copyFile(srcA, destA)).InSequence(s);
    EXPECT_CALL(*mockFileSystem, copyFile(srcB, destB)).InSequence(s);
    EXPECT_CALL(*mockFileSystem, copyFile(srcC, destC)).InSequence(s);

    diagnose::GatherFiles gather(std::move(mockIFileSystemPtr));
    gather.gatherAuditLogs(tempdir.dirPath(), tempdir2.dirPath());

}

TEST_F(TestGatherFiles, emptyAuditDirectory)
{
    Tests::TempDir tempdir("", "audit");
    Tests::TempDir tempdir2("", "destination");
    std::vector<std::string> fileList{};

    auto mockFileSystem = new StrictMock<MockFileSystem>();
    std::unique_ptr<MockFileSystem> mockIFileSystemPtr = std::unique_ptr<MockFileSystem>(mockFileSystem);


    EXPECT_CALL(*mockFileSystem, isDirectory(tempdir.dirPath())).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, listFiles(tempdir.dirPath())).WillOnce(Return(fileList));
    // making sure files are not being copied
    EXPECT_CALL(*mockFileSystem, copyFile(_, _)).Times(0);

    diagnose::GatherFiles gather(std::move(mockIFileSystemPtr));
    gather.gatherAuditLogs(tempdir.dirPath(), tempdir2.dirPath());
}

TEST_F(TestGatherFiles, directoryIncludesNonAuditFiles)
{
    std::string filenameA = "audit.log";
    std::string filenameB = "audit.log.0";
    std::string filenameC = "audit.log.1";
    std::string filenameD = "aaaaaaa.log";

    Tests::TempDir tempdir("", "audit");
    Tests::TempDir tempdir2("", "destination");

    Path srcA = tempdir.absPath(filenameA);
    Path srcB = tempdir.absPath(filenameB);
    Path srcC = tempdir.absPath(filenameC);
    Path srcD = tempdir.absPath(filenameD);
    Path destA = tempdir2.absPath(filenameA);
    Path destB = tempdir2.absPath(filenameB);
    Path destC = tempdir2.absPath(filenameC);
    Path destD = tempdir2.absPath(filenameD);
    std::vector<std::string> fileList{ srcB, srcC, srcA, "aaaaaaa.log"};

    Sequence s;
    auto mockFileSystem = new StrictMock<MockFileSystem>();
    std::unique_ptr<MockFileSystem> mockIFileSystemPtr = std::unique_ptr<MockFileSystem>(mockFileSystem);

    EXPECT_CALL(*mockFileSystem, isDirectory(tempdir.dirPath())).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, listFiles(tempdir.dirPath())).WillOnce(Return(fileList));
    EXPECT_CALL(*mockFileSystem, isFile(srcA)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, isFile(srcB)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, isFile(srcC)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, exists(destA)).WillOnce(Return(false));
    EXPECT_CALL(*mockFileSystem, exists(destB)).WillOnce(Return(false));
    EXPECT_CALL(*mockFileSystem, exists(destC)).WillOnce(Return(false));

    // testing the sort order as we copy after we sort
    EXPECT_CALL(*mockFileSystem, copyFile(srcA, destA)).InSequence(s);
    EXPECT_CALL(*mockFileSystem, copyFile(srcB, destB)).InSequence(s);
    EXPECT_CALL(*mockFileSystem, copyFile(srcC, destC)).InSequence(s);
    // making sure file is not being copied
    EXPECT_CALL(*mockFileSystem, copyFile(srcD, destD)).Times(0);

    diagnose::GatherFiles gather(std::move(mockIFileSystemPtr));
    gather.gatherAuditLogs(tempdir.dirPath(), tempdir2.dirPath());
}