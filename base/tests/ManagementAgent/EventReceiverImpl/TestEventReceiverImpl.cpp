// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "Common/Logging/ConsoleLoggingSetup.h"
#include "ManagementAgent/EventReceiverImpl/EventReceiverImpl.h"
#include "tests/Common/TaskQueueImpl/FakeQueue.h"

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
    Common::TaskQueue::ITaskQueueSharedPtr queue = std::make_shared<FakeQueue>();
    EXPECT_NO_THROW(
        ManagementAgent::EventReceiverImpl::EventReceiverImpl foo(queue));
}

TEST_F(TestEventReceiverImpl, ReceivingEventCausesATaskToBeQueued)
{
    auto queue = std::make_shared<FakeQueue>();
    ManagementAgent::EventReceiverImpl::EventReceiverImpl foo(queue);
    foo.receivedSendEvent("APPID", "EventXML");
    EXPECT_FALSE(queue->empty());
}

TEST_F(TestEventReceiverImpl, ReceivingActionDoesNotThrow)
{
    auto queue = std::make_shared<FakeQueue>();
    ManagementAgent::EventReceiverImpl::EventReceiverImpl foo(queue);
    EXPECT_NO_THROW(foo.handleAction(""));
}

TEST_F(TestEventReceiverImpl, NotInOutbreakModeOnInitialisation)
{
    auto queue = std::make_shared<FakeQueue>();
    ManagementAgent::EventReceiverImpl::EventReceiverImpl foo(queue);
    EXPECT_FALSE(foo.outbreakMode());
}