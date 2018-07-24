/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <ManagementAgent/EventReceiverImpl/EventReceiverImpl.h>
#include <Common/FileSystemImpl/FileSystemImpl.h>

#include <tests/Common/TaskQueueImpl/FakeQueue.h>

#include <tests/Common/FileSystemImpl/MockFileSystem.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

TEST(TestEventReceiverImpl, Construction) //NOLINT
{
    std::string mcs_dir = "test/mcs";
    Common::TaskQueue::ITaskQueueSharedPtr queue(
            new FakeQueue
            );
    ManagementAgent::EventReceiverImpl::EventReceiverImpl foo
            (
                    mcs_dir,
                    queue
                    );
}

TEST(TestEventReceiverImpl, EventCausesTask) //NOLINT
{
    std::string mcs_dir = "test/mcs";
    std::shared_ptr<FakeQueue> queue(
            new FakeQueue
    );
    ManagementAgent::EventReceiverImpl::EventReceiverImpl foo
            (
                    mcs_dir,
                    queue
            );
    foo.receivedSendEvent("APPID","EventXML");
    EXPECT_FALSE(queue->empty());
}
