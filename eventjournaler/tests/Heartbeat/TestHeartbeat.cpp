/******************************************************************************************************
Copyright 2021, Sophos Limited.  All rights reserved.
******************************************************************************************************/

#include <Heartbeat.h>
#include <gtest/gtest.h>

TEST(TestHeartbeat, TestHeartbeatConstructorRegistersIds) // NOLINT
{
    std::vector<std::string> ids = {"a", "b"};
    Heartbeat::Heartbeat heartbeat(ids);
    auto missed = heartbeat.getMissedHeartbeats();
    EXPECT_EQ(missed.size(), 2);
    ASSERT_EQ(missed[0], "a");
    ASSERT_EQ(missed[1], "b");
}

TEST(TestHeartbeat, TestPingRemovesIdsFromMissedHeartbeats) // NOLINT
{
    std::vector<std::string> ids = {"a", "b"};
    Heartbeat::Heartbeat heartbeat(ids);
    auto missed = heartbeat.getMissedHeartbeats();
    EXPECT_EQ(missed.size(), 2);
    ASSERT_EQ(missed[0], "a");
    ASSERT_EQ(missed[1], "b");

    Heartbeat::HeartbeatPinger a = heartbeat.getPingHandleForId("a");
    Heartbeat::HeartbeatPinger b = heartbeat.getPingHandleForId("b");

    a.ping();
    missed = heartbeat.getMissedHeartbeats();
    EXPECT_EQ(missed.size(), 1);
    ASSERT_EQ(missed[0], "b");

    b.ping();
    missed = heartbeat.getMissedHeartbeats();
    EXPECT_EQ(missed.size(), 0);
}

TEST(TestHeartbeat, TestResetAllResetsIds) // NOLINT
{
    std::vector<std::string> ids = {"a", "b"};
    Heartbeat::Heartbeat heartbeat(ids);
    Heartbeat::HeartbeatPinger a = heartbeat.getPingHandleForId("a");
    Heartbeat::HeartbeatPinger b = heartbeat.getPingHandleForId("b");

    a.ping();
    b.ping();
    auto missed = heartbeat.getMissedHeartbeats();
    EXPECT_EQ(missed.size(), 0);

    heartbeat.resetAll();
    missed = heartbeat.getMissedHeartbeats();
    EXPECT_EQ(missed.size(), 2);
    ASSERT_EQ(missed[0], "a");
    ASSERT_EQ(missed[1], "b");
}

TEST(TestHeartbeat, TestDeregisterIdRemovesId) // NOLINT
{
    std::vector<std::string> ids = {"a", "b"};
    Heartbeat::Heartbeat heartbeat(ids);
    auto missed = heartbeat.getMissedHeartbeats();
    EXPECT_EQ(missed.size(), 2);
    ASSERT_EQ(missed[0], "a");
    ASSERT_EQ(missed[1], "b");

    heartbeat.deregisterId("a");
    missed = heartbeat.getMissedHeartbeats();
    EXPECT_EQ(missed.size(), 1);
    ASSERT_EQ(missed[0], "b");
}

TEST(TestHeartbeat, TestRegisterIdAddsId) // NOLINT
{
    std::vector<std::string> ids = {"a", "b"};
    Heartbeat::Heartbeat heartbeat(ids);
    auto missed = heartbeat.getMissedHeartbeats();
    EXPECT_EQ(missed.size(), 2);
    ASSERT_EQ(missed[0], "a");
    ASSERT_EQ(missed[1], "b");

    heartbeat.registerId("c");
    missed = heartbeat.getMissedHeartbeats();
    EXPECT_EQ(missed.size(), 3);
    ASSERT_EQ(missed[0], "a");
    ASSERT_EQ(missed[1], "b");
    ASSERT_EQ(missed[2], "c");
}
