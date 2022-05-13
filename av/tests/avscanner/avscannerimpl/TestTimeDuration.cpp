/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "avscanner/avscannerimpl/TimeDuration.h"
#include "tests/common/LogInitializedTests.h"

#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include <pluginapi/include/Common/Logging/SophosLoggerMacros.h>

using namespace avscanner::avscannerimpl;

class TestTimeDuration : public LogInitializedTests
{
};

TEST_F(TestTimeDuration, TestScanDurationZeroSeconds) // NOLINT
{
    TimeDuration result(0);
    std::string time = result.toString();

    EXPECT_EQ(time, "less than a second.");
}

TEST_F(TestTimeDuration, TestScanDurationLowerBoundarySecond) // NOLINT
{
    TimeDuration result(1);
    std::string time = result.toString();

    EXPECT_EQ(time, "1 second.");
}

TEST_F(TestTimeDuration, TestScanDurationNormalSecond) // NOLINT
{
    TimeDuration result(2);
    std::string time = result.toString();

    EXPECT_EQ(time, "2 seconds.");
}

TEST_F(TestTimeDuration, TestScanDurationUpperBoundarySecond) // NOLINT
{
    TimeDuration result(59);
    std::string time = result.toString();

    EXPECT_EQ(time, "59 seconds.");
}

TEST_F(TestTimeDuration, TestScanDurationLowerBoundaryMinute) // NOLINT
{
    TimeDuration result(60);
    std::string time = result.toString();

    EXPECT_EQ(time, "1 minute, 0 seconds.");
}

TEST_F(TestTimeDuration, TestScanDurationNormalMinute) // NOLINT
{
    TimeDuration result(61);
    std::string time = result.toString();

    EXPECT_EQ(time, "1 minute, 1 second.");
}

TEST_F(TestTimeDuration, TestScanDurationLowerBoundaryTwoMinutes) // NOLINT
{
    TimeDuration result(120);
    std::string time = result.toString();

    EXPECT_EQ(time, "2 minutes, 0 seconds.");
}

TEST_F(TestTimeDuration, TestScanDurationNormalTwoMinutes) // NOLINT
{
    TimeDuration result(121);
    std::string time = result.toString();

    EXPECT_EQ(time, "2 minutes, 1 second.");
}

TEST_F(TestTimeDuration, TestScanDurationUpperBoundaryMinutes) // NOLINT
{
    TimeDuration result(3599);
    std::string time = result.toString();

    EXPECT_EQ(time, "59 minutes, 59 seconds.");
}

TEST_F(TestTimeDuration, TestScanDurationLowerBoundaryHour) // NOLINT
{
    TimeDuration result(3600);
    std::string time = result.toString();

    EXPECT_EQ(time, "1 hour, 0 minutes, 0 seconds.");
}

TEST_F(TestTimeDuration, TestScanDurationNormalHour) // NOLINT
{
    TimeDuration result(3601);
    std::string time = result.toString();

    EXPECT_EQ(time, "1 hour, 0 minutes, 1 second.");
}

TEST_F(TestTimeDuration, TestScanDurationNormalHourAndMinute) // NOLINT
{
    TimeDuration result(3660);
    std::string time = result.toString();

    EXPECT_EQ(time, "1 hour, 1 minute, 0 seconds.");
}

TEST_F(TestTimeDuration, TestScanDurationNormalHourMinuteSecond) // NOLINT
{
    TimeDuration result(3661);
    std::string time = result.toString();

    EXPECT_EQ(time, "1 hour, 1 minute, 1 second.");
}

TEST_F(TestTimeDuration, TestScanDurationNormalTwoHour) // NOLINT
{
    TimeDuration result(7200);
    std::string time = result.toString();

    EXPECT_EQ(time, "2 hours, 0 minutes, 0 seconds.");
}

TEST_F(TestTimeDuration, TestScanDurationNormalTwoHourSecond) // NOLINT
{
    TimeDuration result(7201);
    std::string time = result.toString();

    EXPECT_EQ(time, "2 hours, 0 minutes, 1 second.");
}

TEST_F(TestTimeDuration, TestScanDurationNormalTwentyFourHourSecond) // NOLINT
{
    TimeDuration result(86401);
    std::string time = result.toString();

    EXPECT_EQ(time, "24 hours, 0 minutes, 1 second.");
}

TEST_F(TestTimeDuration, TestScanDurationNormalTwentyFiveHour) // NOLINT
{
    TimeDuration result(90000);
    std::string time = result.toString();

    EXPECT_EQ(time, "25 hours, 0 minutes, 0 seconds.");
}