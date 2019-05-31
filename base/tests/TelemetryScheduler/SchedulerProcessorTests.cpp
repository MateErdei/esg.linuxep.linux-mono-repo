/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "DerivedSchedulerProcessor.h"

#include <TelemetryScheduler/TelemetrySchedulerImpl/SchedulerProcessor.h>
#include <TelemetryScheduler/TelemetrySchedulerImpl/TaskQueue.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <chrono>
#include <thread>

using TelemetrySchedulerImpl::PluginCallback;
using TelemetrySchedulerImpl::SchedulerProcessor;
using TelemetrySchedulerImpl::Task;
using TelemetrySchedulerImpl::TaskQueue;

class SchedulerProcessorTests : public ::testing::Test
{
public:
    void SetUp() override {}

    void TearDown() override {}
};

 TEST_F(SchedulerProcessorTests, CanBeConstructed) // NOLINT
{
    auto queue = std::make_shared<TaskQueue>();
    SchedulerProcessor processor(queue, "", "", "");
}

 TEST_F(SchedulerProcessorTests, ConstructionWithNullQueue) // NOLINT
{
    std::shared_ptr<TaskQueue> queue;

    EXPECT_THROW(SchedulerProcessor processor(queue, "", "", ""), std::invalid_argument); // NOLINT
}

 TEST_F(SchedulerProcessorTests, CanBeStopped) // NOLINT
{
    using namespace std::chrono;
    const milliseconds delay(10);

    auto queue = std::make_shared<TaskQueue>();
    SchedulerProcessor processor(queue, "", "", "");
    std::atomic<bool> done(false);

    std::thread processorThread([&] {
        processor.run();
        done = true;
    });

    EXPECT_FALSE(done);

    std::this_thread::sleep_for(milliseconds(delay));

    EXPECT_FALSE(done);

    queue->pushPriority(Task::Shutdown);
    std::this_thread::sleep_for(milliseconds(delay));

    EXPECT_TRUE(done);

    processorThread.join();
}

 TEST_F(SchedulerProcessorTests, CanBeStoppedViaPlugin) // NOLINT
{
    using namespace std::chrono;
    const milliseconds delay(10);

    auto queue = std::make_shared<TaskQueue>();
    SchedulerProcessor processor(queue, "", "", "");
    PluginCallback pluginCallback(queue);
    std::atomic<bool> done(false);

    std::thread processorThread([&] {
        processor.run();
        done = true;
    });

    EXPECT_FALSE(done);

    pluginCallback.onShutdown();
    std::this_thread::sleep_for(milliseconds(delay));

    EXPECT_TRUE(done);

    processorThread.join();
}

// TODO: it might be better to test the following cases via run() or waitToRunTelemetry()!

TEST_F(SchedulerProcessorTests, getScheduledTimeUsingSupplementaryFile_ValidSupplementaryFile)
{
    // TODO: complete test!
    auto queue = std::make_shared<TaskQueue>();
    DerivedSchedulerProcessor processor(queue, "", "", "");
}

TEST_F(SchedulerProcessorTests, getScheduledTimeUsingSupplementaryFile_InvalidSupplementaryFile)
{
    // TODO: complete test!
    auto queue = std::make_shared<TaskQueue>();
    DerivedSchedulerProcessor processor(queue, "", "", "");
}

TEST_F(SchedulerProcessorTests, getScheduledTimeUsingSupplementaryFile_ZeroInterval)
{
    // TODO: complete test!
    auto queue = std::make_shared<TaskQueue>();
    DerivedSchedulerProcessor processor(queue, "", "", "");
}

TEST_F(SchedulerProcessorTests, getIntervalFromSupplementaryFile_ValidSupplementaryFile)
{
    // TODO: complete test!
    auto queue = std::make_shared<TaskQueue>();
    DerivedSchedulerProcessor processor(queue, "", "", "");
}

TEST_F(SchedulerProcessorTests, getIntervalFromSupplementaryFile_InvalidSupplementaryFile)
{
    // TODO: complete test!
    auto queue = std::make_shared<TaskQueue>();
    DerivedSchedulerProcessor processor(queue, "", "", "");
}

TEST_F(SchedulerProcessorTests, getIntervalFromSupplementaryFile_MissingSupplementaryFile)
{
    // TODO: complete test!
    auto queue = std::make_shared<TaskQueue>();
    DerivedSchedulerProcessor processor(queue, "", "", "");
}

TEST_F(SchedulerProcessorTests, waitToRunTelemetry_ValidStatusFile)
{
    // TODO: complete test!
    auto queue = std::make_shared<TaskQueue>();
    DerivedSchedulerProcessor processor(queue, "", "", "");
}

TEST_F(SchedulerProcessorTests, waitToRunTelemetry_InvalidStatusFile)
{
    // TODO: complete test!
    auto queue = std::make_shared<TaskQueue>();
    DerivedSchedulerProcessor processor(queue, "", "", "");
}

TEST_F(SchedulerProcessorTests, waitToRunTelemetry_MissingStatusFile)
{
    // TODO: complete test!
    auto queue = std::make_shared<TaskQueue>();
    DerivedSchedulerProcessor processor(queue, "", "", "");
}

TEST_F(SchedulerProcessorTests, waitToRunTelemetry_ErrorReadingStatusFile)
{
    // TODO: complete test!
    auto queue = std::make_shared<TaskQueue>();
    DerivedSchedulerProcessor processor(queue, "", "", "");
}
