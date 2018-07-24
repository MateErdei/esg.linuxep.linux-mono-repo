/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <ManagementAgent/StatusReceiverImpl/StatusReceiverImpl.h>
#include <Common/FileSystemImpl/FileSystemImpl.h>

#include <tests/Common/TaskQueueImpl/FakeQueue.h>

#include <tests/Common/FileSystemImpl/MockFileSystem.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

TEST(TestStatusReceiverImpl, Construction) //NOLINT
{
    std::string mcs_dir="test/mcs";
    Common::TaskQueue::ITaskQueueSharedPtr fakeQueue(new FakeQueue);
    EXPECT_NO_THROW
    (
        ManagementAgent::StatusReceiverImpl::StatusReceiverImpl foo(mcs_dir, fakeQueue)
    );
}

TEST(TestStatusReceiverImpl, newStatusCausesTask) //NOLINT
{
    std::string mcs_dir="test/mcs";
    Common::TaskQueue::ITaskQueueSharedPtr fakeQueue(
            new FakeQueue
    );
    ManagementAgent::StatusReceiverImpl::StatusReceiverImpl foo(mcs_dir, fakeQueue);
    foo.receivedChangeStatus("APPID",{"WithTimestamp","WithoutTimestamp"});
    Common::TaskQueueImpl::ITaskPtr task = fakeQueue->popTask();
    EXPECT_NE(task.get(),nullptr);
}

TEST(TestStatusReceiverImpl, newStatusCausesTaskThatWrites) //NOLINT
{
    std::string mcs_dir="test/mcs";
    Common::TaskQueue::ITaskQueueSharedPtr fakeQueue(
            new FakeQueue
    );
    ManagementAgent::StatusReceiverImpl::StatusReceiverImpl foo(mcs_dir, fakeQueue);
    foo.receivedChangeStatus("APPID",{"WithTimestamp","WithoutTimestamp"});
    Common::TaskQueueImpl::ITaskPtr task = fakeQueue->popTask();
    EXPECT_NE(task.get(),nullptr);

    auto filesystemMock = new MockFileSystem();
    Common::FileSystem::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));

    EXPECT_CALL(*filesystemMock, join("test/mcs/status","APPID_status.xml")).WillOnce(Return("test/mcs/status/APPID_status.xml"));
    EXPECT_CALL(*filesystemMock, writeFileAtomically("test/mcs/status/APPID_status.xml","WithTimestamp","test/mcs/tmp")).WillOnce(Return());

    task->run();

    Common::FileSystem::restoreFileSystem();
}
