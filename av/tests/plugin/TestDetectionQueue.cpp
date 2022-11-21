// Copyright 2022, Sophos Limited. All rights reserved.

#include "pluginimpl/DetectionQueue.h"
#include "pluginimpl/PluginCallback.h"
#include "scan_messages/ThreatDetected.h"

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"

#include <Common/Helpers/LogInitializedTests.h>
#include <gtest/gtest.h>
#include <thirdparty/nlohmann-json/json.hpp>

#include <future>
#include <thread>

using json = nlohmann::json;
using namespace common::CentralEnums;
using namespace std::chrono_literals;

class TestDetectionQueue : public LogOffInitializedTests
{
    void SetUp() override
    {
        auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
        appConfig.setData("SOPHOS_INSTALL", "/tmp");
        appConfig.setData("PLUGIN_INSTALL", "/tmp/av");
    }
};

scan_messages::ThreatDetected basicDetection()
{
    scan_messages::ThreatDetected basicDetection(
        "root",
        1,
        ThreatType::virus,
        "threatName",
        scan_messages::E_SCAN_TYPE_UNKNOWN,
        scan_messages::E_NOTIFICATION_STATUS_NOT_CLEANUPABLE,
        "/path",
        scan_messages::E_SMT_THREAT_ACTION_UNKNOWN,
        "sha256",
        "01234567-89ab-cdef-0123-456789abcdef",
        false,
        ReportSource::ml,
        datatypes::AutoFd());
    return basicDetection;
}

TEST_F(TestDetectionQueue, TestQueueIntialisesToEmpty) // NOLINT
{
    Plugin::DetectionQueue queue;
    ASSERT_TRUE(queue.isEmpty());
}

TEST_F(TestDetectionQueue, TestQueueIsFullOnceMaxSizeIsReached) // NOLINT
{
    Plugin::DetectionQueue queue;
    queue.setMaxSize(1);

    auto detection = basicDetection();

    ASSERT_FALSE(queue.isFull());
    queue.push(detection);
    ASSERT_TRUE(queue.isFull());
}

TEST_F(TestDetectionQueue, TestQueueCannotBePushedToOnceFullAndTelemetryIsIncremented)
{
    Plugin::DetectionQueue queue;
    std::shared_ptr<Plugin::TaskQueue> task = nullptr;
    std::shared_ptr<Plugin::PluginCallback> m_pluginCallback = std::make_shared<Plugin::PluginCallback>(task);
    json initialTelemetry = json::parse(m_pluginCallback->getTelemetry());
    EXPECT_EQ(initialTelemetry["detections-dropped-from-safestore-queue"], 0);

    queue.setMaxSize(1);

    auto detection1 = basicDetection();
    auto detection2 = basicDetection();

    ASSERT_FALSE(queue.isFull());
    ASSERT_TRUE(queue.push(detection1));
    ASSERT_TRUE(queue.isFull());
    ASSERT_FALSE(queue.push(detection2));

    json modifiedTelemetry = json::parse(m_pluginCallback->getTelemetry());
    EXPECT_EQ(modifiedTelemetry["detections-dropped-from-safestore-queue"], 1);
}

TEST_F(TestDetectionQueue, TestQueueClearsFullAndEmptyStatesWhenPopulated) // NOLINT
{
    Plugin::DetectionQueue queue;
    queue.setMaxSize(2);

    auto detection = basicDetection();

    ASSERT_TRUE(queue.isEmpty());
    ASSERT_TRUE(queue.push(detection));
    ASSERT_FALSE(queue.isFull());
    ASSERT_FALSE(queue.isEmpty());
}

TEST_F(TestDetectionQueue, TestQueueClearsIsFullStateAfterPop) // NOLINT
{
    Plugin::DetectionQueue queue;
    queue.setMaxSize(1);

    auto detection = basicDetection();

    ASSERT_FALSE(queue.isFull());
    ASSERT_TRUE(queue.push(detection));
    ASSERT_TRUE(queue.isFull());
    queue.pop();
    ASSERT_FALSE(queue.isFull());
}

TEST_F(TestDetectionQueue, TestQueueSetsIsEmptyStateAfterPoppingLastDetection) // NOLINT
{
    Plugin::DetectionQueue queue;
    queue.setMaxSize(2);

    auto detection1 = basicDetection();
    auto detection2 = basicDetection();

    ASSERT_TRUE(queue.isEmpty());
    ASSERT_TRUE(queue.push(detection1));
    ASSERT_TRUE(queue.push(detection2));
    ASSERT_FALSE(queue.isEmpty());
    queue.pop();
    ASSERT_FALSE(queue.isEmpty());
    queue.pop();
    ASSERT_TRUE(queue.isEmpty());
}

TEST_F(TestDetectionQueue, DetectionsSentToTestQueueAreStillValidIfPushFails) // NOLINT
{
    Plugin::DetectionQueue queue;
    queue.setMaxSize(1);

    auto detection1 = basicDetection();
    auto detection2 = basicDetection();

    ASSERT_TRUE(queue.push(detection1));
    ASSERT_FALSE(queue.push(detection2));
    queue.pop();
    ASSERT_TRUE(queue.push(detection2));
}

TEST_F(TestDetectionQueue, TestDetectionsQueuePopBlocksUntilToldToStop) // NOLINT
{
    Plugin::DetectionQueue queue;
    auto result = std::async(std::launch::async, &Plugin::DetectionQueue::pop, &queue);

    ASSERT_EQ(result.wait_for(200ms), std::future_status::timeout);

    queue.requestStop();
    ASSERT_EQ(result.wait_for(100ms), std::future_status::ready);
    EXPECT_FALSE(result.get().has_value());
}

TEST_F(TestDetectionQueue, TestDetectionsQueuePopReturnsImmediately) // NOLINT
{
    Plugin::DetectionQueue queue;
    auto result = std::async(std::launch::async, &Plugin::DetectionQueue::pop, &queue);
    auto detection = basicDetection();

    queue.push(detection);
    ASSERT_EQ(result.wait_for(100ms), std::future_status::ready);
    EXPECT_TRUE(result.get().has_value());
}

TEST_F(TestDetectionQueue, testPushedDataIsCorrectlyQueuedAndReturnedWhenPopped) // NOLINT
{
    Plugin::DetectionQueue queue;

    auto detectionToPush = basicDetection();
    auto detectionPopped = basicDetection();

    queue.push(detectionToPush);
    auto poppedData = queue.pop();

    ASSERT_EQ(poppedData, detectionPopped);
}