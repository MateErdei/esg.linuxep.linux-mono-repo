/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <tests/Common/TaskQueueImpl/FakeQueue.h>
#include <tests/Common/FileSystemImpl/MockFileSystem.h>
#include <ManagementAgent/LoggerImpl/LoggingSetup.h>
#include <ManagementAgent/EventReceiverImpl/EventReceiverImpl.h>
#include <Common/FileSystemImpl/FileSystemImpl.h>

class TestEventReceiverImpl : public ::testing::Test
{

public:

    TestEventReceiverImpl()
            : m_loggingSetup(std::unique_ptr<ManagementAgent::LoggerImpl::LoggingSetup>(new ManagementAgent::LoggerImpl::LoggingSetup(1)))
    {

    }

private:
    std::unique_ptr<ManagementAgent::LoggerImpl::LoggingSetup> m_loggingSetup;

};


TEST_F(TestEventReceiverImpl, Construction) //NOLINT
{
    std::string mcs_dir = "test/mcs";
    Common::TaskQueue::ITaskQueueSharedPtr queue(
            new FakeQueue
            );
    EXPECT_NO_THROW(
    ManagementAgent::EventReceiverImpl::EventReceiverImpl foo
            (
                    queue
                    )
        );
}

TEST_F(TestEventReceiverImpl, ReceivingEventCausesATaskToBeQueued) //NOLINT
{
    std::shared_ptr<FakeQueue> queue(
            new FakeQueue
    );
    ManagementAgent::EventReceiverImpl::EventReceiverImpl foo
            (
                    queue
            );
    foo.receivedSendEvent("APPID","EventXML");
    EXPECT_FALSE(queue->empty());
}
