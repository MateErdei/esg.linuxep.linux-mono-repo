/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/FileSystem/IFileSystem.h>
#include <gtest/gtest.h>
#include <Common/Helpers/LogInitializedTests.h>
#include <modules/SubscriberLib/EventQueuePusher.h>
#include "tests/Helpers/MockEventQueue.h"


class TestEventQueuePusher : public LogOffInitializedTests{};

using namespace SubscriberLib;

TEST_F(TestEventQueuePusher, testPushPassesCorrectArguementsToItsPushMethod) // NOLINT
{
    MockEventQueue* mockQueue = new StrictMock<MockEventQueue>();
    std::shared_ptr<IEventQueue> mockQueuePtr(mockQueue);
    Common::ZeroMQWrapper::data_t testData = {"data", "for", "test"};
    SubscriberLib::EventQueuePusher eventQueuePusher(mockQueuePtr);

    EXPECT_CALL(*mockQueue, push(testData)).Times(1);
    eventQueuePusher.push(testData);
}
