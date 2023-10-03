// Copyright 2021 Sophos Limited. All rights reserved.

#ifdef SPL_BAZEL
#include "eventjournaler/tests/Helpers/MockEventQueue.h"
#else
#include "tests/Helpers/MockEventQueue.h"
#endif

#include "SubscriberLib/EventQueuePusher.h"

#ifdef SPL_BAZEL
#include "base/tests/Common/Helpers/LogInitializedTests.h"
#else
#include "Common/Helpers/LogInitializedTests.h"
#endif
#include "Common/FileSystem/IFileSystem.h"
#include "Common/TelemetryHelperImpl/TelemetryHelper.h"


#include <gtest/gtest.h>


class TestEventQueuePusher : public LogOffInitializedTests{};

using namespace SubscriberLib;

TEST_F(TestEventQueuePusher, testPushPassesCorrectArguementsToItsPushMethod) // NOLINT
{
    MockEventQueue* mockQueue = new StrictMock<MockEventQueue>();
    std::shared_ptr<IEventQueue> mockQueuePtr(mockQueue);
    JournalerCommon::Event testData = {JournalerCommon::EventType::THREAT_EVENT, "testdata"};
    SubscriberLib::EventQueuePusher eventQueuePusher(mockQueuePtr);

    EXPECT_CALL(*mockQueue, push(testData)).Times(1);
    eventQueuePusher.handleEvent(testData);
}

TEST_F(TestEventQueuePusher, testDroppedEventsTriggerTelemetryIncrement) // NOLINT
{
    MockEventQueue* mockQueue = new StrictMock<MockEventQueue>();
    std::shared_ptr<IEventQueue> mockQueuePtr(mockQueue);
    JournalerCommon::Event testData = {JournalerCommon::EventType::THREAT_EVENT, "testdata"};
    SubscriberLib::EventQueuePusher eventQueuePusher(mockQueuePtr);
    auto& telemetry = Common::Telemetry::TelemetryHelper::getInstance();

    EXPECT_CALL(*mockQueue, push(testData)).WillRepeatedly(Return(false));

    telemetry.reset();
    eventQueuePusher.handleEvent(testData);
    EXPECT_EQ(telemetry.serialise(), "{\"dropped-av-events\":1}");
    eventQueuePusher.handleEvent(testData);
    EXPECT_EQ(telemetry.serialiseAndReset(), "{\"dropped-av-events\":2}");
}