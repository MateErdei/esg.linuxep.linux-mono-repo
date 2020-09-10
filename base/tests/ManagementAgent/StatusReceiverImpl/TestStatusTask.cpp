/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/FileSystemImpl/FileSystemImpl.h>
#include <Common/Logging/ConsoleLoggingSetup.h>
#include <ManagementAgent/StatusCacheImpl/StatusCache.h>
#include <ManagementAgent/StatusReceiverImpl/StatusTask.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <tests/Common/Helpers/FileSystemReplaceAndRestore.h>
#include <tests/Common/Helpers/MockFileSystem.h>
#include <tests/Common/TaskQueueImpl/FakeQueue.h>

class TestStatusTask : public ::testing::Test
{
    Common::Logging::ConsoleLoggingSetup m_loggingSetup;
};

TEST_F(TestStatusTask, Construction) // NOLINT
{
    std::shared_ptr<ManagementAgent::StatusCache::IStatusCache> cache =
        std::make_shared<ManagementAgent::StatusCacheImpl::StatusCache>();
    EXPECT_NO_THROW // NOLINT
        (ManagementAgent::StatusReceiverImpl::StatusTask task(
            cache, "APPID", "StatusWithTimestamp", "StatusWithoutTimestamp", "tempDir", "statusDir"));
}

TEST_F(TestStatusTask, checkTaskWritesOutNewStatusToFile) // NOLINT
{
    auto filesystemMock = new StrictMock<MockFileSystem>();
    EXPECT_CALL(*filesystemMock, writeFileAtomically("statusDir/APPID_status.xml", "StatusWithTimestamp", "tempDir", S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP))
        .WillOnce(Return());
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};
    std::shared_ptr<ManagementAgent::StatusCache::IStatusCache> cache =
        std::make_shared<ManagementAgent::StatusCacheImpl::StatusCache>();
    std::string appId("APPID"), contents("StatusWithoutTimeStamp");
    ManagementAgent::StatusReceiverImpl::StatusTask task(
        cache, appId, "StatusWithTimestamp", contents, "tempDir", "statusDir");
    std::string fullPath = Common::FileSystem::join(
        Common::ApplicationConfiguration::applicationPathManager().getManagementAgentStatusCacheFilePath(),
        appId + ".xml");
    EXPECT_CALL(*filesystemMock, exists(fullPath)).WillOnce(Return(false));
    EXPECT_CALL(*filesystemMock, writeFile(fullPath, contents));
    task.run();
}

TEST_F(TestStatusTask, checkTwoIdentialTasksDontWriteTwice) // NOLINT
{
    auto filesystemMock = new StrictMock<MockFileSystem>();

    std::string appId("APPID"), contents("StatusWithoutTimeStamp");
    std::string fullPath = Common::FileSystem::join(
        Common::ApplicationConfiguration::applicationPathManager().getManagementAgentStatusCacheFilePath(),
        appId + ".xml");
    EXPECT_CALL(*filesystemMock, writeFileAtomically("statusDir/APPID_status.xml", "StatusWithTimestamp", "tempDir", S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP))
        .WillOnce(Return());
    EXPECT_CALL(*filesystemMock, exists(fullPath)).WillOnce(Return(false));
    EXPECT_CALL(*filesystemMock, writeFile(fullPath, contents)).WillOnce(Return());

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};
    std::shared_ptr<ManagementAgent::StatusCache::IStatusCache> cache =
        std::make_shared<ManagementAgent::StatusCacheImpl::StatusCache>();
    ManagementAgent::StatusReceiverImpl::StatusTask task(
        cache, appId, "StatusWithTimestamp", contents, "tempDir", "statusDir");
    task.run();

    ManagementAgent::StatusReceiverImpl::StatusTask task2(
        cache, appId, "StatusWithTimestamp", contents, "tempDir", "statusDir");
    task2.run();
}

TEST_F(TestStatusTask, checkTaskWorksWithEmptyAppIdAndStatusArguments) // NOLINT
{
    auto filesystemMock = new StrictMock<MockFileSystem>();
    std::string appId;
    std::string contents;
    std::string fullPath = Common::FileSystem::join(
        Common::ApplicationConfiguration::applicationPathManager().getManagementAgentStatusCacheFilePath(),
        appId + ".xml");
    EXPECT_CALL(*filesystemMock, writeFileAtomically("statusDir/_status.xml", "", "tempDir", S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)).WillOnce(Return());
    EXPECT_CALL(*filesystemMock, exists(fullPath)).WillOnce(Return(false));
    EXPECT_CALL(*filesystemMock, writeFile(fullPath, contents)).WillOnce(Return());
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};
    std::shared_ptr<ManagementAgent::StatusCache::IStatusCache> cache =
        std::make_shared<ManagementAgent::StatusCacheImpl::StatusCache>();
    ManagementAgent::StatusReceiverImpl::StatusTask task(cache, appId, "", contents, "tempDir", "statusDir");
    task.run();
}