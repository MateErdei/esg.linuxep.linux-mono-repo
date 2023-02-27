/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/FileSystemImpl/FileSystemImpl.h>
#include <Common/Logging/ConsoleLoggingSetup.h>
#include <ManagementAgent/EventReceiverImpl/EventReceiverImpl.h>
#include <modules/Common/Logging/ConsoleLoggingSetup.h>
#include <tests/Common/Helpers/MockFileSystem.h>
#include <tests/Common/TaskQueueImpl/FakeQueue.h>

class TestEventReceiverImpl : public ::testing::Test
{
public:
    TestEventReceiverImpl() {}

private:
    Common::Logging::ConsoleLoggingSetup m_loggingSetup;
};

TEST_F(TestEventReceiverImpl, Construction) // NOLINT
{
    std::string mcs_dir = "test/mcs";
    Common::TaskQueue::ITaskQueueSharedPtr queue(new FakeQueue);
    EXPECT_NO_THROW( // NOLINT
        ManagementAgent::EventReceiverImpl::EventReceiverImpl foo(queue));
}

TEST_F(TestEventReceiverImpl, ReceivingEventCausesATaskToBeQueued) // NOLINT
{
    std::shared_ptr<FakeQueue> queue(new FakeQueue);
    ManagementAgent::EventReceiverImpl::EventReceiverImpl foo(queue);
    foo.receivedSendEvent("APPID", "EventXML");
    EXPECT_FALSE(queue->empty());
}
