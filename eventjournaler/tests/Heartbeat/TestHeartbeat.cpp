/******************************************************************************************************
Copyright 2021, Sophos Limited.  All rights reserved.
******************************************************************************************************/

#include <Heartbeat.h>
#include <gtest/gtest.h>
#include "../../redist/pluginapi/include/Common/UtilityImpl/TimeUtils.h"
#include "FakeTimeUtils.h"

TEST(TestHeartbeat, TestPingRemovesIdsFromMissedHeartbeats) // NOLINT
{
    std::vector<std::string> ids = {"a", "b"};
    Heartbeat::Heartbeat heartbeat;
    std::shared_ptr<Heartbeat::HeartbeatPinger> a = heartbeat.getPingHandleForId("a");
    std::shared_ptr<Heartbeat::HeartbeatPinger> b = heartbeat.getPingHandleForId("b");

    auto missed = heartbeat.getMissedHeartbeats();
    ASSERT_EQ(missed.size(), 2);
    ASSERT_EQ(missed[0], "a");
    ASSERT_EQ(missed[1], "b");

    a->ping();
    missed = heartbeat.getMissedHeartbeats();
    ASSERT_EQ(missed.size(), 1);
    ASSERT_EQ(missed[0], "b");

    b->ping();
    missed = heartbeat.getMissedHeartbeats();
    ASSERT_EQ(missed.size(), 0);
}

TEST(TestHeartbeat, TestDeregisterIdRemovesId) // NOLINT
{
    std::vector<std::string> ids = {"a", "b"};
    Heartbeat::Heartbeat heartbeat;
    std::shared_ptr<Heartbeat::HeartbeatPinger> a = heartbeat.getPingHandleForId("a");
    std::shared_ptr<Heartbeat::HeartbeatPinger> b = heartbeat.getPingHandleForId("b");

    auto missed = heartbeat.getMissedHeartbeats();
    ASSERT_EQ(missed.size(), 2);
    ASSERT_EQ(missed[0], "a");
    ASSERT_EQ(missed[1], "b");

    heartbeat.deregisterId("a");
    missed = heartbeat.getMissedHeartbeats();
    ASSERT_EQ(missed.size(), 1);
    ASSERT_EQ(missed[0], "b");
}

TEST(TestHeartbeat, TestIdIsAddedWhenHandleRequested) // NOLINT
{
    Heartbeat::Heartbeat heartbeat;
    std::shared_ptr<Heartbeat::HeartbeatPinger> a = heartbeat.getPingHandleForId("a");
    std::shared_ptr<Heartbeat::HeartbeatPinger> b = heartbeat.getPingHandleForId("b");

    auto missed = heartbeat.getMissedHeartbeats();
    ASSERT_EQ(missed.size(), 2);
    ASSERT_EQ(missed[0], "a");
    ASSERT_EQ(missed[1], "b");

    std::shared_ptr<Heartbeat::HeartbeatPinger> c = heartbeat.getPingHandleForId("c");
    missed = heartbeat.getMissedHeartbeats();
    ASSERT_EQ(missed.size(), 3);
    ASSERT_EQ(missed[0], "a");
    ASSERT_EQ(missed[1], "b");
    ASSERT_EQ(missed[2], "c");
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
    auto missed = heartbeat.getMissedHeartbeats();
    ASSERT_EQ(missed.size(), 0);
}

TEST(TestHeartbeat, testHeartbeatsAreDeadWhenPingedMoreThan15SecondsAgo) // NOLINT
{
    Common::UtilityImpl::ScopedReplaceITime scopedReplaceITime(std::unique_ptr<Common::UtilityImpl::ITime>(
            new SequenceOfFakeTime({ 10, 10, 30, 20 }, std::chrono::milliseconds(0), []() {})));

    Heartbeat::Heartbeat heartbeat;
    std::shared_ptr<Heartbeat::HeartbeatPinger> a = heartbeat.getPingHandleForId("a");
    std::shared_ptr<Heartbeat::HeartbeatPinger> b = heartbeat.getPingHandleForId("b");

    a->ping();
    b->ping();
    auto missed = heartbeat.getMissedHeartbeats();
    ASSERT_EQ(missed.size(), 1);
    ASSERT_EQ(missed[0], "a");
}

TEST(TestHeartbeat, testHeartbeatIsAliveWhenPingedExactly15sAgo) // NOLINT
{
    Common::UtilityImpl::ScopedReplaceITime scopedReplaceITime(std::unique_ptr<Common::UtilityImpl::ITime>(
            new SequenceOfFakeTime({ 10, 25 }, std::chrono::milliseconds(0), []() {})));

    Heartbeat::Heartbeat heartbeat;
    std::shared_ptr<Heartbeat::HeartbeatPinger> a = heartbeat.getPingHandleForId("a");

    a->ping();
    auto missed = heartbeat.getMissedHeartbeats();
    ASSERT_EQ(missed.size(), 0);
}

TEST(TestHeartbeat, testHeartbeatIsAliveWhenLastPingWasSameSecond) // NOLINT
{
    Common::UtilityImpl::ScopedReplaceITime scopedReplaceITime(std::unique_ptr<Common::UtilityImpl::ITime>(
            new SequenceOfFakeTime({ 50, 50 }, std::chrono::milliseconds(0), []() {})));

    Heartbeat::Heartbeat heartbeat;
    std::shared_ptr<Heartbeat::HeartbeatPinger> a = heartbeat.getPingHandleForId("a");

    a->ping();
    auto missed = heartbeat.getMissedHeartbeats();
    ASSERT_EQ(missed.size(), 0);
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
    time_t hour = 60*60;
    Heartbeat::HeartbeatPinger pinger;

    // push an event and # should be 1
    {
        Common::UtilityImpl::ScopedReplaceITime scopedReplaceITime(std::unique_ptr<Common::UtilityImpl::ITime>(
                new SequenceOfFakeTime({ hour, 10*hour}, std::chrono::milliseconds(0), []() {})));
        pinger.pushDroppedEvent();
        EXPECT_EQ(pinger.getNumDroppedEventsInLast24h(), 1);
    }

    // push two more within the 24h window and # should be 3
    {
        Common::UtilityImpl::ScopedReplaceITime scopedReplaceITime(std::unique_ptr<Common::UtilityImpl::ITime>(
                new SequenceOfFakeTime({ 12*hour, 20*hour, 23*hour}, std::chrono::milliseconds(0), []() {})));
        pinger.pushDroppedEvent();
        pinger.pushDroppedEvent();
        EXPECT_EQ(pinger.getNumDroppedEventsInLast24h(), 3);
    }

    // roll into the next day enough to drop off the first event
    {
        Common::UtilityImpl::ScopedReplaceITime scopedReplaceITime(std::unique_ptr<Common::UtilityImpl::ITime>(
                new SequenceOfFakeTime({30*hour}, std::chrono::milliseconds(0), []() {})));
        EXPECT_EQ(pinger.getNumDroppedEventsInLast24h(), 2);
    }

    // many days later without new events # should be 0
    {
        Common::UtilityImpl::ScopedReplaceITime scopedReplaceITime(std::unique_ptr<Common::UtilityImpl::ITime>(
                new SequenceOfFakeTime({100*hour}, std::chrono::milliseconds(0), []() {})));
        EXPECT_EQ(pinger.getNumDroppedEventsInLast24h(), 0);
    }
}


TEST(TestHeartbeat, testIsAlive)
{
    Common::UtilityImpl::ScopedReplaceITime scopedReplaceITime(std::unique_ptr<Common::UtilityImpl::ITime>(
            new SequenceOfFakeTime({100, 105, 110, 120, 121, 130, 131}, std::chrono::milliseconds(0), []() {})));
    Heartbeat::HeartbeatPinger pinger;
    // starts fakse
    ASSERT_EQ(pinger.isAlive(), false);
    // true after ping
    pinger.ping();
    ASSERT_EQ(pinger.isAlive(), true);
    // still alive after 15s
    ASSERT_EQ(pinger.isAlive(), true);
    // stops being alive after >15s
    ASSERT_EQ(pinger.isAlive(), false);
    // returns to being alive when repinged;
    pinger.ping();
    ASSERT_EQ(pinger.isAlive(), true);
}
