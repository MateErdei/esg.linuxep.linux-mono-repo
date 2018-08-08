/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <ManagementAgent/StatusReceiverImpl/StatusReceiverImpl.h>
#include <modules/ManagementAgent/StatusCacheImpl/StatusCache.h>
#include <Common/FileSystemImpl/FileSystemImpl.h>

#include <tests/Common/TaskQueueImpl/FakeQueue.h>

#include <tests/Common/FileSystemImpl/MockFileSystem.h>
#include <modules/Common/ApplicationConfiguration/IApplicationPathManager.h>

TEST(TestStatusReceiverImpl, Construction) //NOLINT
{
    Common::TaskQueue::ITaskQueueSharedPtr fakeQueue(new FakeQueue);
    EXPECT_NO_THROW
    (
            std::shared_ptr<ManagementAgent::StatusCache::IStatusCache> statusCache = std::make_shared<ManagementAgent::StatusCacheImpl::StatusCache>();
            ManagementAgent::StatusReceiverImpl::StatusReceiverImpl
            foo(fakeQueue, statusCache)
    );
}

TEST(TestStatusReceiverImpl, checkNewStatusCausesATaskToBeQueued) //NOLINT
{
    Common::TaskQueue::ITaskQueueSharedPtr fakeQueue(
            new FakeQueue
    );
    std::shared_ptr<ManagementAgent::StatusCache::IStatusCache> statusCache = std::make_shared<ManagementAgent::StatusCacheImpl::StatusCache>();
    ManagementAgent::StatusReceiverImpl::StatusReceiverImpl foo(fakeQueue, statusCache);
    foo.receivedChangeStatus("APPID",{"WithTimestamp","WithoutTimestamp"});
    Common::TaskQueueImpl::ITaskPtr task = fakeQueue->popTask();
    EXPECT_NE(task.get(),nullptr);
}

TEST(TestStatusReceiverImpl, checkNewStatusCausesATaskToBeQueuedThatWritesToAStatusFile) //NOLINT
{
    Common::TaskQueue::ITaskQueueSharedPtr fakeQueue(new FakeQueue);
    std::shared_ptr<ManagementAgent::StatusCache::IStatusCache> statusCache = std::make_shared<ManagementAgent::StatusCacheImpl::StatusCache>();
    ManagementAgent::StatusReceiverImpl::StatusReceiverImpl foo(fakeQueue, statusCache);
    std::string appId("APPID"), contents("WithoutTimeStamp");
    foo.receivedChangeStatus(appId,{"WithTimestamp",contents});
    Common::TaskQueueImpl::ITaskPtr task = fakeQueue->popTask();
    EXPECT_NE(task.get(),nullptr);

    auto filesystemMock = new StrictMock<MockFileSystem>();
    Common::FileSystem::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));


    EXPECT_CALL(*filesystemMock,
                writeFileAtomically("/opt/sophos-spl/base/mcs/status/APPID_status.xml", "WithTimestamp",
                                    "/opt/sophos-spl/tmp"
                )).WillOnce(Return());

    std::string fullPath = Common::FileSystem::join(Common::ApplicationConfiguration::applicationPathManager().getManagementAgentStatusCacheFilePath(), appId) + ".xml";
    EXPECT_CALL(*filesystemMock, writeFile(fullPath, contents));

    task->run();

    Common::FileSystem::restoreFileSystem();
}
