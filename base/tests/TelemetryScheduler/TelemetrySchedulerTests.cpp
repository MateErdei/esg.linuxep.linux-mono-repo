/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

/**
 * Component tests for Telemetry Scheduler
 */

#include <TelemetryScheduler/TelemetrySchedulerImpl/Scheduler.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

class TelemetrySchedulerTest : public ::testing::Test
{
public:
    void SetUp() override
    {
    }

    void TearDown() override
    {
    }
};

TEST_F(TelemetrySchedulerTest, main_entry_hasSuccessfulExitCode) // NOLINT
{
//    EXPECT_EQ(SchedulerImpl::main_entry(), 0); // TODO: This test file will be removed as tests will be against other classes.
}
