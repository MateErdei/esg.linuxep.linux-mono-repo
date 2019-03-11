/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/FileSystemImpl/FileSystemImpl.h>
#include <ManagementAgent/StatusCacheImpl/StatusCache.h>
#include <ManagementAgent/StatusReceiverImpl/StatusReceiverImpl.h>
#include <tests/Common/Helpers/FileSystemReplaceAndRestore.h>
#include <tests/Common/Helpers/MockFileSystem.h>
#include <Common/Logging/ConsoleLoggingSetup.h>
#include <tests/Common/TaskQueueImpl/FakeQueue.h>

class TestStatusReceiverImpl : public ::testing::Test
{
public:
    TestStatusReceiverImpl() {}

private:
    Common::Logging::ConsoleLoggingSetup m_loggingSetup;
};

TEST_F(TestStatusReceiverImpl, Construction) // NOLINT
{
    Common::TaskQueue::ITaskQueueSharedPtr fakeQueue(new FakeQueue);
    EXPECT_NO_THROW // NOLINT
        (std::shared_ptr<ManagementAgent::StatusCache::IStatusCache> statusCache =
             std::make_shared<ManagementAgent::StatusCacheImpl::StatusCache>();
         ManagementAgent::StatusReceiverImpl::StatusReceiverImpl foo(fakeQueue, statusCache));
}

TEST_F(TestStatusReceiverImpl, checkNewStatusCausesATaskToBeQueued) // NOLINT
{
    Common::TaskQueue::ITaskQueueSharedPtr fakeQueue(new FakeQueue);
    std::shared_ptr<ManagementAgent::StatusCache::IStatusCache> statusCache =
        std::make_shared<ManagementAgent::StatusCacheImpl::StatusCache>();
    ManagementAgent::StatusReceiverImpl::StatusReceiverImpl foo(fakeQueue, statusCache);
    foo.receivedChangeStatus("APPID", { "WithTimestamp", "WithoutTimestamp", "" });
    Common::TaskQueueImpl::ITaskPtr task = fakeQueue->popTask();
    EXPECT_NE(task.get(), nullptr);
}

TEST_F(TestStatusReceiverImpl, checkNewStatusCausesATaskToBeQueuedThatWritesToAStatusFile) // NOLINT
{
    Common::TaskQueue::ITaskQueueSharedPtr fakeQueue(new FakeQueue);
    std::shared_ptr<ManagementAgent::StatusCache::IStatusCache> statusCache =
        std::make_shared<ManagementAgent::StatusCacheImpl::StatusCache>();
    ManagementAgent::StatusReceiverImpl::StatusReceiverImpl foo(fakeQueue, statusCache);
    std::string appId("APPID"), contents("WithoutTimeStamp");
    foo.receivedChangeStatus(appId, { "WithTimestamp", contents, "" });
    Common::TaskQueueImpl::ITaskPtr task = fakeQueue->popTask();
    EXPECT_NE(task.get(), nullptr);

    auto filesystemMock = new StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));

    EXPECT_CALL(
        *filesystemMock,
        writeFileAtomically("/opt/sophos-spl/base/mcs/status/APPID_status.xml", "WithTimestamp", "/opt/sophos-spl/tmp"))
        .WillOnce(Return());

    std::string fullPath =
        Common::FileSystem::join(
            Common::ApplicationConfiguration::applicationPathManager().getManagementAgentStatusCacheFilePath(), appId) +
        ".xml";
    EXPECT_CALL(*filesystemMock, writeFile(fullPath, contents));

    task->run();

    Tests::restoreFileSystem();
}
