// Copyright 2022, Sophos Limited.  All rights reserved.

#include "pluginimpl/DetectionsQueue.h"
#include "scan_messages/ThreatDetected.h"

#include <Common/Helpers/LogInitializedTests.h>
#include <gtest/gtest.h>

#include <future>
#include <thread>

class TestDetectionsQueue : public LogOffInitializedTests
{
};

scan_messages::ThreatDetected basicDetection()
{
    scan_messages::ThreatDetected basicDetection(
        "root",
        1,
        scan_messages::E_VIRUS_THREAT_TYPE,
        "threatName",
        scan_messages::E_SCAN_TYPE_UNKNOWN,
        scan_messages::E_NOTIFICATION_STATUS_NOT_CLEANUPABLE,
        "/path",
        scan_messages::E_SMT_THREAT_ACTION_UNKNOWN,
        "sha256",
        "threatId",
        datatypes::AutoFd());
    return basicDetection;
}

TEST_F(TestDetectionsQueue, TestDetectionsQueueSizeFunctions) // NOLINT
{
    Plugin::DetectionsQueue queue;
    queue.setMaxSize(2);

    auto detection1 = basicDetection();
    auto detection2 = basicDetection();
    auto detection3 = basicDetection();

    ASSERT_TRUE(queue.isEmpty());
    ASSERT_FALSE(queue.isFull());
    queue.push(std::move(detection1));
    ASSERT_FALSE(queue.isEmpty());
    ASSERT_FALSE(queue.isFull());
    queue.push(std::move(detection2));
    ASSERT_FALSE(queue.isEmpty());
    ASSERT_TRUE(queue.isFull());
    queue.push(std::move(detection3));
    ASSERT_FALSE(queue.isEmpty());
    ASSERT_TRUE(queue.isFull());
    queue.pop();
    ASSERT_FALSE(queue.isEmpty());
    ASSERT_TRUE(queue.isFull());
    queue.pop();
    ASSERT_FALSE(queue.isEmpty());
    ASSERT_FALSE(queue.isFull());
    queue.pop();
    ASSERT_TRUE(queue.isEmpty());
    ASSERT_FALSE(queue.isFull());
}

TEST_F(TestDetectionsQueue, TestDetectionsQueuePopBlocksUntilToldToStop) // NOLINT
{
    Plugin::DetectionsQueue queue;
    auto result = std::async(std::launch::async, &Plugin::DetectionsQueue::pop, &queue);

    std::chrono::milliseconds before =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    queue.requestStop();
    result.wait();
    std::chrono::milliseconds after =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());

    EXPECT_FALSE(result.get().has_value());
    int duration = after.count() - before.count();
    EXPECT_NEAR(duration, 500, 5);
}

TEST_F(TestDetectionsQueue, TestDetectionsQueuePopReturnsImmediately) // NOLINT
{
    Plugin::DetectionsQueue queue;
    auto result = std::async(std::launch::async, &Plugin::DetectionsQueue::pop, &queue);
    auto detection = basicDetection();

    std::chrono::milliseconds before =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
    queue.push(std::move(detection));
    result.wait();
    std::chrono::milliseconds after =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());

    EXPECT_TRUE(result.get().has_value());
    int duration = after.count() - before.count();
    EXPECT_LE(duration, 5);
}

TEST_F(TestDetectionsQueue, testPushedDataIsCorrectlyQueuedAndReturnedWhenPopped) // NOLINT
{
    Plugin::DetectionsQueue queue;

    auto detectionToPush = basicDetection();
    auto detectionPopped = basicDetection();

    queue.push(std::move(detectionToPush));
    auto poppedData = queue.pop();

    ASSERT_EQ(poppedData, detectionPopped);
}