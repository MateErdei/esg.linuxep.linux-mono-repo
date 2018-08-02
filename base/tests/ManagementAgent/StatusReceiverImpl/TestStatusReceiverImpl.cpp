/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <ManagementAgent/StatusReceiverImpl/StatusReceiverImpl.h>
#include <ManagementAgent/StatusReceiverImpl/StatusCache.h>
#include <Common/FileSystemImpl/FileSystemImpl.h>

#include <tests/Common/TaskQueueImpl/FakeQueue.h>

#include <tests/Common/FileSystemImpl/MockFileSystem.h>

TEST(TestStatusReceiverImpl, Construction) //NOLINT
{
    Common::TaskQueue::ITaskQueueSharedPtr fakeQueue(new FakeQueue);
    EXPECT_NO_THROW
    (
            ManagementAgent::StatusReceiverImpl::StatusCache statusCache;
            ManagementAgent::StatusReceiverImpl::StatusReceiverImpl
            foo(fakeQueue, statusCache)
    );
}

TEST(TestStatusReceiverImpl, checkNewStatusCausesATaskToBeQueued) //NOLINT
{
    Common::TaskQueue::ITaskQueueSharedPtr fakeQueue(
            new FakeQueue
    );
    ManagementAgent::StatusReceiverImpl::StatusCache statusCache;
    ManagementAgent::StatusReceiverImpl::StatusReceiverImpl foo(fakeQueue, statusCache);
    foo.receivedChangeStatus("APPID",{"WithTimestamp","WithoutTimestamp"});
    Common::TaskQueueImpl::ITaskPtr task = fakeQueue->popTask();
    EXPECT_NE(task.get(),nullptr);
}

TEST(TestStatusReceiverImpl, checkNewStatusCausesATaskToBeQueuedThatWritesToAStatusFile) //NOLINT
{
    Common::TaskQueue::ITaskQueueSharedPtr fakeQueue(new FakeQueue);
    ManagementAgent::StatusReceiverImpl::StatusCache statusCache;
    ManagementAgent::StatusReceiverImpl::StatusReceiverImpl foo(fakeQueue, statusCache);
    foo.receivedChangeStatus("APPID",{"WithTimestamp","WithoutTimestamp"});
    Common::TaskQueueImpl::ITaskPtr task = fakeQueue->popTask();
    EXPECT_NE(task.get(),nullptr);

    auto filesystemMock = new StrictMock<MockFileSystem>();
    Common::FileSystem::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));

    EXPECT_CALL(*filesystemMock,
                writeFileAtomically("/opt/sophos-spl/base/mcs/status/APPID_status.xml", "WithTimestamp",
                                    "/opt/sophos-spl/tmp"
                )).WillOnce(Return());

    task->run();

    Common::FileSystem::restoreFileSystem();
}
