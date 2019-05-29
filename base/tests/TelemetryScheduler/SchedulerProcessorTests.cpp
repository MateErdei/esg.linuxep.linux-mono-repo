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

using TelemetrySchedulerImpl::Task;
using TelemetrySchedulerImpl::TaskQueue;
using TelemetrySchedulerImpl::SchedulerProcessor;

class SchedulerProcessorTests : public ::testing::Test
{
public:
    void SetUp() override {}

    void TearDown() override {}
};

TEST_F(SchedulerProcessorTests, CanBeStopped) // NOLINT
{
    using namespace std::chrono;
    const milliseconds delay(100);

    auto queue = std::make_shared<TaskQueue>();
    SchedulerProcessor processor(queue);
    steady_clock::time_point start = steady_clock::now();

    std::thread stopAfterDelay([&] {
      std::this_thread::sleep_for(std::chrono::milliseconds(delay));
      queue->push(Task::Shutdown);
    });

    processor.run();
    stopAfterDelay.join();
    steady_clock::time_point end = steady_clock::now();
    milliseconds measuredDelay = duration_cast<milliseconds>(end - start);

    EXPECT_GE(measuredDelay.count(), delay.count());
}
