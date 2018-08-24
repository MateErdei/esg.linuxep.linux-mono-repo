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
#include <modules/Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <modules/ManagementAgent/LoggerImpl/LoggingSetup.h>

class TestStatusTask : public ::testing::Test
{

public:

    TestStatusTask()
            : m_loggingSetup(std::unique_ptr<ManagementAgent::LoggerImpl::LoggingSetup>(new ManagementAgent::LoggerImpl::LoggingSetup(1)))
    {

    }

private:
    std::unique_ptr<ManagementAgent::LoggerImpl::LoggingSetup> m_loggingSetup;

};

TEST_F(TestStatusTask, Construction) //NOLINT
{
    std::shared_ptr<ManagementAgent::StatusCache::IStatusCache> cache = std::make_shared<ManagementAgent::StatusCacheImpl::StatusCache>();
    EXPECT_NO_THROW // NOLINT
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

TEST_F(TestStatusTask, checkTaskWritesOutNewStatusToFile) //NOLINT
{
    auto filesystemMock = new StrictMock<MockFileSystem>();
    EXPECT_CALL(*filesystemMock, writeFileAtomically("statusDir/APPID_status.xml","StatusWithTimestamp","tempDir")).WillOnce(Return());
    Common::FileSystem::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));
    std::shared_ptr<ManagementAgent::StatusCache::IStatusCache> cache = std::make_shared<ManagementAgent::StatusCacheImpl::StatusCache>();
    std::string appId("APPID"), contents("StatusWithoutTimeStamp");
    ManagementAgent::StatusReceiverImpl::StatusTask task(
            cache,
            appId,
            "StatusWithTimestamp",
            contents,
            "tempDir",
            "statusDir"
    );
    std::string fullPath = Common::FileSystem::join(Common::ApplicationConfiguration::applicationPathManager().getManagementAgentStatusCacheFilePath(), appId + ".xml");
    EXPECT_CALL(*filesystemMock, writeFile(fullPath, contents));
    task.run();
    Common::FileSystem::restoreFileSystem();
}

TEST_F(TestStatusTask, checkTwoIdentialTasksDontWriteTwice) //NOLINT
{

    auto filesystemMock = new StrictMock<MockFileSystem>();

    std::string appId("APPID"), contents("StatusWithoutTimeStamp");
    std::string fullPath = Common::FileSystem::join(Common::ApplicationConfiguration::applicationPathManager().getManagementAgentStatusCacheFilePath(), appId + ".xml");
    EXPECT_CALL(*filesystemMock, writeFileAtomically("statusDir/APPID_status.xml","StatusWithTimestamp","tempDir")).WillOnce(Return());
    EXPECT_CALL(*filesystemMock, writeFile(fullPath, contents)).WillOnce(Return());

    Common::FileSystem::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));
    std::shared_ptr<ManagementAgent::StatusCache::IStatusCache> cache = std::make_shared<ManagementAgent::StatusCacheImpl::StatusCache>();
    ManagementAgent::StatusReceiverImpl::StatusTask task(
            cache,
            appId,
            "StatusWithTimestamp",
            contents,
            "tempDir",
            "statusDir"
    );
    task.run();

    ManagementAgent::StatusReceiverImpl::StatusTask task2(
            cache,
            appId,
            "StatusWithTimestamp",
            contents,
            "tempDir",
            "statusDir"
    );
    task2.run();


    Common::FileSystem::restoreFileSystem();
}

TEST_F(TestStatusTask, checkTaskWorksWithEmptyAppIdAndStatusArguments) // NOLINT
{
    auto filesystemMock = new StrictMock<MockFileSystem>();
    std::string appId;
    std::string contents;
    std::string fullPath =
            Common::FileSystem::join(
                    Common::ApplicationConfiguration::applicationPathManager().getManagementAgentStatusCacheFilePath(),
                    appId + ".xml"
            );
    EXPECT_CALL(*filesystemMock, writeFileAtomically("statusDir/_status.xml","","tempDir")).WillOnce(Return());
    EXPECT_CALL(*filesystemMock, writeFile(fullPath, contents)).WillOnce(Return());
    Common::FileSystem::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));
    std::shared_ptr<ManagementAgent::StatusCache::IStatusCache> cache = std::make_shared<ManagementAgent::StatusCacheImpl::StatusCache>();
    ManagementAgent::StatusReceiverImpl::StatusTask task(
            cache,
            appId,
            "",
            contents,
            "tempDir",
            "statusDir"
    );
    task.run();
    Common::FileSystem::restoreFileSystem();
}