// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include <Common/Logging/ConsoleLoggingSetup.h>
#include <ManagementAgent/EventReceiverImpl/EventReceiverImpl.h>
#include <tests/Common/TaskQueueImpl/FakeQueue.h>

#include <gtest/gtest.h>

namespace
{
    class TestEventReceiverImpl : public ::testing::Test
    {
    public:
        TestEventReceiverImpl() = default;

    private:
        Common::Logging::ConsoleLoggingSetup m_loggingSetup;
    };
}

TEST_F(TestEventReceiverImpl, Construction)
{
    std::string mcs_dir = "test/mcs";
    Common::TaskQueue::ITaskQueueSharedPtr queue(new FakeQueue);
    EXPECT_NO_THROW( // NOLINT
        ManagementAgent::EventReceiverImpl::EventReceiverImpl foo(queue));
}

TEST_F(TestEventReceiverImpl, ReceivingEventCausesATaskToBeQueued)
{
    std::shared_ptr<FakeQueue> queue(new FakeQueue);
    ManagementAgent::EventReceiverImpl::EventReceiverImpl foo(queue);
    foo.receivedSendEvent("APPID", "EventXML");
    EXPECT_FALSE(queue->empty());
}
