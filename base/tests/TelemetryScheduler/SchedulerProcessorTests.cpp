/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

/**
 * Component tests for Telemetry Scheduler
 */

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

// TEST_F(SchedulerProcessorTests, CanBeStopped) // NOLINT
//{
//    using namespace std::chrono;
//    const milliseconds delay(10);
//
//    auto queue = std::make_shared<TaskQueue>();
//    SchedulerProcessor processor(queue, <#initializer #>, <#initializer #>);
//    std::atomic<bool> done(false);
//
//    std::thread processorThread([&] {
//        processor.run();
//        done = true;
//    });
//
//    EXPECT_FALSE(done);
//
//    queue->push(Task::WaitToRunTelemetry);
//    std::this_thread::sleep_for(milliseconds(delay));
//
//    EXPECT_FALSE(done);
//
//    queue->push(Task::Shutdown);
//    std::this_thread::sleep_for(milliseconds(delay));
//
//    EXPECT_TRUE(done);
//
//    processorThread.join();
//}
//
// TEST_F(SchedulerProcessorTests, CanBeStoppedViaPlugin) // NOLINT
//{
//    auto queue = std::make_shared<TaskQueue>();
//    SchedulerProcessor processor(queue, <#initializer #>, <#initializer #>);
//    PluginCallback pluginCallback(queue);
//
//    pluginCallback.onShutdown();
//    processor.run();
//}
