/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <ManagementAgent/EventReceiverImpl/EventReceiverImpl.h>

#include <Common/FileSystemImpl/FileSystemImpl.h>
#include <tests/Common/Helpers/MockFileSystem.h>

#include <tests/Common/TaskQueueImpl/FakeQueue.h>
#include <tests/Common/Logging/TestConsoleLoggingSetup.h>

class TestEventReceiverImpl : public ::testing::Test
{

public:

    TestEventReceiverImpl()
            : m_loggingSetup(new TestLogging::TestConsoleLoggingSetup())
    {

    }

private:
    TestLogging::TestConsoleLoggingSetupPtr m_loggingSetup;

};


TEST_F(TestEventReceiverImpl, Construction) //NOLINT
{
    std::string mcs_dir = "test/mcs";
    Common::TaskQueue::ITaskQueueSharedPtr queue(
            new FakeQueue
            );
    EXPECT_NO_THROW( //NOLINT
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
