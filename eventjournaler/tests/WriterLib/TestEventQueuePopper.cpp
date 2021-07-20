/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/FileSystem/IFileSystem.h>
#include <gtest/gtest.h>
#include <Common/Helpers/LogInitializedTests.h>
#include <modules/EventWriterLib/EventQueuePopper.h>
#include "tests/Helpers/MockEventQueue.h"


class TestEventQueuePopper : public LogOffInitializedTests{};

using namespace EventWriterLib;

//TEST_F(TestEventQueuePopper, testGetEventCorrectlyCallsPopWithGivenArguements) // NOLINT
//{
//    MockEventQueue* mockQueue = new StrictMock<MockEventQueue>();
//    std::shared_ptr<IEventQueue> mockQueuePtr(mockQueue);
//    int testTimeOut = 10;
//    WriterLib::EventQueuePopper eventQueuePopper(mockQueuePtr);
//
//    EXPECT_CALL(*mockQueue, pop(testTimeOut)).Times(1);
//    eventQueuePopper.getEvent(testTimeOut);
//}
