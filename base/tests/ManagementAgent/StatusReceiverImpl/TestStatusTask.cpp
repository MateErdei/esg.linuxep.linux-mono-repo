/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <ManagementAgent/StatusReceiverImpl/StatusTask.h>
#include <ManagementAgent/StatusCacheImpl/StatusCache.h>
#include <Common/FileSystemImpl/FileSystemImpl.h>

#include <tests/Common/TaskQueueImpl/FakeQueue.h>

#include <tests/Common/FileSystemImpl/MockFileSystem.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

TEST(TestStatusTask, Construction) //NOLINT
{
    std::shared_ptr<ManagementAgent::StatusCache::IStatusCache> cache = std::make_shared<ManagementAgent::StatusCacheImpl::StatusCache>();
    EXPECT_NO_THROW
    (
    ManagementAgent::StatusReceiverImpl::StatusTask task(
            cache,
            "APPID",
            "StatusWithTimestamp",
            "StatusWithoutTimestamp",
            "tempDir",
            "statusDir"
            )
    );
}

TEST(TestStatusTask, checkTaskWritesOutNewStatusToFile) //NOLINT
{
    auto filesystemMock = new StrictMock<MockFileSystem>();
    EXPECT_CALL(*filesystemMock, writeFileAtomically("statusDir/APPID_status.xml","StatusWithTimestamp","tempDir")).WillOnce(Return());
    Common::FileSystem::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));
    std::shared_ptr<ManagementAgent::StatusCache::IStatusCache> cache = std::make_shared<ManagementAgent::StatusCacheImpl::StatusCache>();
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

TEST(TestStatusTask, checkTwoIdentialTasksDontWriteTwice) //NOLINT
{

    auto filesystemMock = new StrictMock<MockFileSystem>();

    EXPECT_CALL(*filesystemMock, writeFileAtomically("statusDir/APPID_status.xml","StatusWithTimestamp","tempDir")).WillOnce(Return());


    Common::FileSystem::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));
    std::shared_ptr<ManagementAgent::StatusCache::IStatusCache> cache = std::make_shared<ManagementAgent::StatusCacheImpl::StatusCache>();
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

TEST(TestStatusTask, checkTaskWorksWithEmptyAppIdAndStatusArguments) // NOLINT
{
    auto filesystemMock = new StrictMock<MockFileSystem>();
    EXPECT_CALL(*filesystemMock, writeFileAtomically("statusDir/_status.xml","","tempDir")).WillOnce(Return());
    Common::FileSystem::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));
    std::shared_ptr<ManagementAgent::StatusCache::IStatusCache> cache = std::make_shared<ManagementAgent::StatusCacheImpl::StatusCache>();
    ManagementAgent::StatusReceiverImpl::StatusTask task(
            cache,
            "",
            "",
            "",
            "tempDir",
            "statusDir"
    );
    task.run();
    Common::FileSystem::restoreFileSystem();
}