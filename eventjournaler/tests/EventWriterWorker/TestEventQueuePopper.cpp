// Copyright 2021-2023 Sophos Limited. All rights reserved.

#include <Common/FileSystem/IFileSystem.h>
#include <Common/Helpers/LogInitializedTests.h>
#include <modules/EventWriterWorkerLib/EventQueuePopper.h>
#include "tests/Helpers/MockEventQueue.h"

#include <gtest/gtest.h>

namespace
{
    class TestEventQueuePopper : public LogOffInitializedTests
    {
    };
}

using namespace EventWriterLib;
using namespace EventQueueLib;
using namespace ::testing;

TEST_F(TestEventQueuePopper, testGetEventCorrectlyCallsPopWithGivenArguements)
{
    MockEventQueue* mockQueue = new StrictMock<MockEventQueue>();
    std::shared_ptr<IEventQueue> mockQueuePtr(mockQueue);
    int testTimeOut = 10;
    EventWriterLib::EventQueuePopper eventQueuePopper(mockQueuePtr);

    EXPECT_CALL(*mockQueue, pop(testTimeOut)).Times(1);
    eventQueuePopper.getEvent(testTimeOut);
}
