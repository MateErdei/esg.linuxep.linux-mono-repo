// Copyright 2021-2023 Sophos Limited. All rights reserved.

#include "FakeTimeUtils.h"
#ifdef SPL_BAZEL
#include "Heartbeat/Heartbeat.h"
#include "JournalerCommon/TimeConsts.h"
#else
#include "Heartbeat.h"
#include "modules/JournalerCommon/TimeConsts.h"
#endif

#include "EventWriterWorkerLib/EventWriterWorker.h"
#include "SubscriberLib/Subscriber.h"

#include "Common/UtilityImpl/TimeUtils.h"

#include <gtest/gtest.h>

TEST(TestHeartbeat, TestGetMapOfIdsAgainstIsAliveShowsHeartbeatsAliveCorrectly) // NOLINT
{
    std::vector<std::string> ids = { "a", "b" };
    Heartbeat::Heartbeat heartbeat;
    std::shared_ptr<Heartbeat::HeartbeatPinger> a = heartbeat.getPingHandleForId("a");
    std::shared_ptr<Heartbeat::HeartbeatPinger> b = heartbeat.getPingHandleForId("b");

    auto map = heartbeat.getMapOfIdsAgainstIsAlive();
    ASSERT_EQ(map.size(), 2);
    ASSERT_EQ(map["a"], false);
    ASSERT_EQ(map["b"], false);

    a->ping();
    map = heartbeat.getMapOfIdsAgainstIsAlive();
    ASSERT_EQ(map.size(), 2);
    ASSERT_EQ(map["a"], true);
    ASSERT_EQ(map["b"], false);

    b->ping();
    map = heartbeat.getMapOfIdsAgainstIsAlive();
    ASSERT_EQ(map.size(), 2);
    ASSERT_EQ(map["a"], true);
    ASSERT_EQ(map["b"], true);
}

TEST(TestHeartbeat, TestDeregisterIdRemovesId) // NOLINT
{
    std::vector<std::string> ids = { "a", "b" };
    Heartbeat::Heartbeat heartbeat;
    std::shared_ptr<Heartbeat::HeartbeatPinger> a = heartbeat.getPingHandleForId("a");
    std::shared_ptr<Heartbeat::HeartbeatPinger> b = heartbeat.getPingHandleForId("b");

    auto map = heartbeat.getMapOfIdsAgainstIsAlive();
    ASSERT_EQ(map.size(), 2);
    ASSERT_EQ(map["a"], false);
    ASSERT_EQ(map["b"], false);

    heartbeat.deregisterId("a");
    map = heartbeat.getMapOfIdsAgainstIsAlive();
    ASSERT_EQ(map.size(), 1);
    ASSERT_EQ(map["b"], false);
}

TEST(TestHeartbeat, TestIdIsAddedWhenHandleRequested) // NOLINT
{
    Heartbeat::Heartbeat heartbeat;
    std::shared_ptr<Heartbeat::HeartbeatPinger> a = heartbeat.getPingHandleForId("a");
    std::shared_ptr<Heartbeat::HeartbeatPinger> b = heartbeat.getPingHandleForId("b");

    auto map = heartbeat.getMapOfIdsAgainstIsAlive();
    ASSERT_EQ(map.size(), 2);
    ASSERT_EQ(map["a"], false);
    ASSERT_EQ(map["b"], false);

    std::shared_ptr<Heartbeat::HeartbeatPinger> c = heartbeat.getPingHandleForId("c");
    map = heartbeat.getMapOfIdsAgainstIsAlive();
    ASSERT_EQ(map.size(), 3);
    ASSERT_EQ(map["a"], false);
    ASSERT_EQ(map["b"], false);
    ASSERT_EQ(map["c"], false);
}

TEST(TestHeartbeat, testHeartbeatsAreAliveWhenPingedWithin15Seconds) // NOLINT
{
    Common::UtilityImpl::ScopedReplaceITime scopedReplaceITime(std::unique_ptr<Common::UtilityImpl::ITime>(
        new SequenceOfFakeTime({ 10, 10, 20, 20 }, std::chrono::milliseconds(0), []() {})));

    Heartbeat::Heartbeat heartbeat;
    std::shared_ptr<Heartbeat::HeartbeatPinger> a = heartbeat.getPingHandleForId("a");
    std::shared_ptr<Heartbeat::HeartbeatPinger> b = heartbeat.getPingHandleForId("b");

    a->ping();
    b->ping();
    auto map = heartbeat.getMapOfIdsAgainstIsAlive();
    ASSERT_EQ(map.size(), 2);
    ASSERT_EQ(map["a"], true);
    ASSERT_EQ(map["b"], true);
}

TEST(TestHeartbeat, testHeartbeatsAreDeadWhenPingedMoreThanTheMaxPingTimeoutSecondsAgo)
{
    // the first two 10s will be the pinged times, the 3rd value will be repeatedly returned on calls and used for
    // the current time to compare the ping times against, so it needs to be greater than MAX_PING_TIMEOUT.
    Common::UtilityImpl::ScopedReplaceITime scopedReplaceITime(
        std::unique_ptr<Common::UtilityImpl::ITime>(new SequenceOfFakeTime(
            { 10, 10, 2 * JournalerCommon::MAX_PING_TIMEOUT_SECONDS }, std::chrono::milliseconds(0), []() {})));

    Heartbeat::Heartbeat heartbeat;
    std::shared_ptr<Heartbeat::HeartbeatPinger> a = heartbeat.getPingHandleForId("a");
    std::shared_ptr<Heartbeat::HeartbeatPinger> b = heartbeat.getPingHandleForId("b");

    a->ping();
    b->ping();
    auto map = heartbeat.getMapOfIdsAgainstIsAlive();
    ASSERT_EQ(map.size(), 2);
    ASSERT_EQ(map["a"], false);
    ASSERT_EQ(map["b"], false);
}

TEST(TestHeartbeat, testHeartbeatIsAliveWhenPingedExactly15sAgo) // NOLINT
{
    Common::UtilityImpl::ScopedReplaceITime scopedReplaceITime(std::unique_ptr<Common::UtilityImpl::ITime>(
        new SequenceOfFakeTime({ 10, 25 }, std::chrono::milliseconds(0), []() {})));

    Heartbeat::Heartbeat heartbeat;
    std::shared_ptr<Heartbeat::HeartbeatPinger> a = heartbeat.getPingHandleForId("a");

    a->ping();
    auto map = heartbeat.getMapOfIdsAgainstIsAlive();
    ASSERT_EQ(map.size(), 1);
    ASSERT_EQ(map["a"], true);
}

TEST(TestHeartbeat, testHeartbeatIsAliveWhenLastPingWasSameSecond) // NOLINT
{
    Common::UtilityImpl::ScopedReplaceITime scopedReplaceITime(std::unique_ptr<Common::UtilityImpl::ITime>(
        new SequenceOfFakeTime({ 50, 50 }, std::chrono::milliseconds(0), []() {})));

    Heartbeat::Heartbeat heartbeat;
    std::shared_ptr<Heartbeat::HeartbeatPinger> a = heartbeat.getPingHandleForId("a");

    a->ping();
    auto map = heartbeat.getMapOfIdsAgainstIsAlive();
    ASSERT_EQ(map.size(), 1);
    ASSERT_EQ(map["a"], true);
}

TEST(TestHeartbeat, testGetAllHeartbeatIdsReturnsAllIds) // NOLINT
{
    Heartbeat::Heartbeat heartbeat;
    std::shared_ptr<Heartbeat::HeartbeatPinger> a = heartbeat.getPingHandleForId("a");
    std::shared_ptr<Heartbeat::HeartbeatPinger> b = heartbeat.getPingHandleForId("b");
    std::shared_ptr<Heartbeat::HeartbeatPinger> c = heartbeat.getPingHandleForId("c");

    auto ids = heartbeat.getAllHeartbeatIds();
    ASSERT_EQ(ids.size(), 3);
    ASSERT_EQ(ids[0], "a");
    ASSERT_EQ(ids[1], "b");
    ASSERT_EQ(ids[2], "c");

    // pinging should not affect this function
    a->ping();
    b->ping();
    ids = heartbeat.getAllHeartbeatIds();
    ASSERT_EQ(ids.size(), 3);
    ASSERT_EQ(ids[0], "a");
    ASSERT_EQ(ids[1], "b");
    ASSERT_EQ(ids[2], "c");
}

TEST(TestHeartbeat, testgetMapOfIdsAgainstIsAlive) // NOLINT
{
    Heartbeat::Heartbeat heartbeat;
    std::shared_ptr<Heartbeat::HeartbeatPinger> a = heartbeat.getPingHandleForId("a");
    std::shared_ptr<Heartbeat::HeartbeatPinger> b = heartbeat.getPingHandleForId("b");
    std::shared_ptr<Heartbeat::HeartbeatPinger> c = heartbeat.getPingHandleForId("c");

    a->ping();
    b->ping();
    auto map = heartbeat.getMapOfIdsAgainstIsAlive();
    ASSERT_EQ(map.size(), 3);
    ASSERT_EQ(map["a"], true);
    ASSERT_EQ(map["b"], true);
    ASSERT_EQ(map["c"], false);
}

TEST(TestHeartbeat, testPushDroppedEventAndGetEventsInLast24h)
{
    time_t hour = 60 * 60;
    Heartbeat::HeartbeatPinger pinger;

    // push an event and # should be 1
    {
        Common::UtilityImpl::ScopedReplaceITime scopedReplaceITime(std::unique_ptr<Common::UtilityImpl::ITime>(
            new SequenceOfFakeTime({ hour, 10 * hour }, std::chrono::milliseconds(0), []() {})));
        pinger.pushDroppedEvent();
        EXPECT_EQ(pinger.getNumDroppedEventsInLast24h(), 1);
    }

    // push two more within the 24h window and # should be 3
    {
        Common::UtilityImpl::ScopedReplaceITime scopedReplaceITime(std::unique_ptr<Common::UtilityImpl::ITime>(
            new SequenceOfFakeTime({ 12 * hour, 20 * hour, 23 * hour }, std::chrono::milliseconds(0), []() {})));
        pinger.pushDroppedEvent();
        pinger.pushDroppedEvent();
        EXPECT_EQ(pinger.getNumDroppedEventsInLast24h(), 3);
    }

    // roll into the next day enough to drop off the first event
    {
        Common::UtilityImpl::ScopedReplaceITime scopedReplaceITime(std::unique_ptr<Common::UtilityImpl::ITime>(
            new SequenceOfFakeTime({ 30 * hour }, std::chrono::milliseconds(0), []() {})));
        EXPECT_EQ(pinger.getNumDroppedEventsInLast24h(), 2);
    }

    // many days later without new events # should be 0
    {
        Common::UtilityImpl::ScopedReplaceITime scopedReplaceITime(std::unique_ptr<Common::UtilityImpl::ITime>(
            new SequenceOfFakeTime({ 100 * hour }, std::chrono::milliseconds(0), []() {})));
        EXPECT_EQ(pinger.getNumDroppedEventsInLast24h(), 0);
    }
}

TEST(TestHeartbeat, testPushDroppedEventNeverPushesAboveMax)
{
    Common::UtilityImpl::ScopedReplaceITime scopedReplaceITime(std::unique_ptr<Common::UtilityImpl::ITime>(
        new SequenceOfFakeTime({ 0, 0, 0, 0, 0, 0, 0, 1, 10 }, std::chrono::milliseconds(0), []() {})));

    Heartbeat::HeartbeatPinger pinger;
    pinger.setDroppedEventsMax(3);

    pinger.pushDroppedEvent();
    EXPECT_EQ(pinger.getNumDroppedEventsInLast24h(), 1);
    pinger.pushDroppedEvent();
    pinger.pushDroppedEvent();
    EXPECT_EQ(pinger.getNumDroppedEventsInLast24h(), 3);
    pinger.pushDroppedEvent();
    EXPECT_EQ(pinger.getNumDroppedEventsInLast24h(), 3);
    pinger.pushDroppedEvent();
    EXPECT_EQ(pinger.getNumDroppedEventsInLast24h(), 3);
}

TEST(TestHeartbeat, testPushDroppedEventTrimsEventsFromTheFuture)
{
    Common::UtilityImpl::ScopedReplaceITime scopedReplaceITime(std::unique_ptr<Common::UtilityImpl::ITime>(
        new SequenceOfFakeTime({ 0, 0, 0, 5, 50, 51, 52, 20, 25 }, std::chrono::milliseconds(0), []() {})));

    Heartbeat::HeartbeatPinger pinger;
    pinger.setDroppedEventsMax(5);

    pinger.pushDroppedEvent();
    pinger.pushDroppedEvent();
    pinger.pushDroppedEvent();
    EXPECT_EQ(pinger.getNumDroppedEventsInLast24h(), 3);
    pinger.pushDroppedEvent();
    pinger.pushDroppedEvent();
    EXPECT_EQ(pinger.getNumDroppedEventsInLast24h(), 5);

    pinger.pushDroppedEvent();
    EXPECT_EQ(pinger.getNumDroppedEventsInLast24h(), 4);
}

TEST(TestHeartbeat, testIsAlive)
{
    long start = 1600000000;
    Common::UtilityImpl::ScopedReplaceITime scopedReplaceITime(
        std::unique_ptr<Common::UtilityImpl::ITime>(new SequenceOfFakeTime(
            { start,                                                   // 1st isAlive()
              start,                                                   // pinger.ping()
              start + 10,                                              // 2nd isAlive()
              start + JournalerCommon::MAX_PING_TIMEOUT_SECONDS / 2,         // 3rd isAlive()
              start + JournalerCommon::MAX_PING_TIMEOUT_SECONDS + 1,         // 4th isAlive()
              start + JournalerCommon::MAX_PING_TIMEOUT_SECONDS * 2,         // pinger.ping()
              start + (JournalerCommon::MAX_PING_TIMEOUT_SECONDS * 2) + 1 }, // 5th and final isAlive()
            std::chrono::milliseconds(0),
            []() {})));
    Heartbeat::HeartbeatPinger pinger;
    // Starts false
    ASSERT_EQ(pinger.isAlive(), false);
    // true after ping
    pinger.ping();
    ASSERT_EQ(pinger.isAlive(), true);
    ASSERT_EQ(pinger.isAlive(), true);
    ASSERT_EQ(pinger.isAlive(), false);
    // returns to being alive once pinged
    pinger.ping();
    ASSERT_EQ(pinger.isAlive(), true);
}

TEST(TestHeartbeat, testRegisterIdsRegistersIds)
{
    Heartbeat::Heartbeat heartbeat;
    std::vector<std::string> ids = heartbeat.getAllHeartbeatIds();
    ASSERT_EQ(ids.size(), 0);

    heartbeat.registerIds({ "a", "b", "c" });
    ids = heartbeat.getAllHeartbeatIds();
    ASSERT_EQ(ids.size(), 3);
    EXPECT_EQ(ids[0], "a");
    EXPECT_EQ(ids[1], "b");
    EXPECT_EQ(ids[2], "c");
}