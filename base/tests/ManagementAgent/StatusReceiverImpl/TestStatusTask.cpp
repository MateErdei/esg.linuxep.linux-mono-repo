/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <ManagementAgent/StatusReceiverImpl/StatusTask.h>
#include <Common/FileSystemImpl/FileSystemImpl.h>

#include <tests/Common/TaskQueueImpl/FakeQueue.h>

#include <tests/Common/FileSystemImpl/MockFileSystem.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

/*
 *
                    StatusCache& statusCache,
                    std::string appId,
                    std::string statusXml,
                    std::string statusXmlWithoutTimestamps,
                    std::string tempDir,
                    std::string statusDir
 */

TEST(TestStatusTask, Construction) //NOLINT
{
    ManagementAgent::StatusReceiverImpl::StatusCache cache;
    ManagementAgent::StatusReceiverImpl::StatusTask task(
            cache,
            "APPID",
            "StatusWithTimestamp",
            "StatusWithoutTimestamp",
            "tempDir",
            "statusDir"
            );
}

TEST(TestStatusTask, WriteNewStatus) //NOLINT
{
    auto filesystemMock = new MockFileSystem();
    EXPECT_CALL(*filesystemMock, join("statusDir","APPID_status.xml")).WillOnce(Return("statusDir/APPID_status.xml"));
    EXPECT_CALL(*filesystemMock, writeFileAtomically("statusDir/APPID_status.xml","StatusWithTimestamp","tempDir")).WillOnce(Return());
    Common::FileSystem::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));
    ManagementAgent::StatusReceiverImpl::StatusCache cache;
    ManagementAgent::StatusReceiverImpl::StatusTask task(
            cache,
            "APPID",
            "StatusWithTimestamp",
            "StatusWithoutTimestamp",
            "tempDir",
            "statusDir"
    );
    task.run();
    Common::FileSystem::restoreFileSystem();
}

TEST(TestStatusTask, DontWriteOldStatus) //NOLINT
{

    auto filesystemMock = new MockFileSystem();

    EXPECT_CALL(*filesystemMock, join("statusDir","APPID_status.xml")).WillOnce(Return("statusDir/APPID_status.xml"));
    EXPECT_CALL(*filesystemMock, writeFileAtomically("statusDir/APPID_status.xml","StatusWithTimestamp","tempDir")).WillOnce(Return());


    Common::FileSystem::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));
    ManagementAgent::StatusReceiverImpl::StatusCache cache;
    ManagementAgent::StatusReceiverImpl::StatusTask task(
            cache,
            "APPID",
            "StatusWithTimestamp",
            "StatusWithoutTimestamp",
            "tempDir",
            "statusDir"
    );
    task.run();

    ManagementAgent::StatusReceiverImpl::StatusTask task2(
            cache,
            "APPID",
            "StatusWithTimestamp",
            "StatusWithoutTimestamp",
            "tempDir",
            "statusDir"
    );
    task2.run();


    Common::FileSystem::restoreFileSystem();
}