/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "avscanner/avscannerimpl/TimeDuration.h"
#include "tests/common/LogInitializedTests.h"

#include <gtest/gtest.h>

using namespace avscanner::avscannerimpl;

class TestTimeDuration : public LogInitializedTests
{
};

TEST_F(TestTimeDuration, TestScanDurationZeroSeconds)
{
    TimeDuration result(0);
    std::string time = result.toString();

    EXPECT_EQ(time, "less than a second.");
}

TEST_F(TestTimeDuration, TestScanDurationLowerBoundarySecond)
{
    TimeDuration result(1);
    std::string time = result.toString();

    EXPECT_EQ(time, "1 second.");
}

TEST_F(TestTimeDuration, TestScanDurationNormalSecond)
{
    TimeDuration result(2);
    std::string time = result.toString();

    EXPECT_EQ(time, "2 seconds.");
}

TEST_F(TestTimeDuration, TestScanDurationUpperBoundarySecond)
{
    TimeDuration result(59);
    std::string time = result.toString();

    EXPECT_EQ(time, "59 seconds.");
}

TEST_F(TestTimeDuration, TestScanDurationLowerBoundaryMinute)
{
    TimeDuration result(60);
    std::string time = result.toString();

    EXPECT_EQ(time, "1 minute, 0 seconds.");
}

TEST_F(TestTimeDuration, TestScanDurationNormalMinute)
{
    TimeDuration result(61);
    std::string time = result.toString();

    EXPECT_EQ(time, "1 minute, 1 second.");
}

TEST_F(TestTimeDuration, TestScanDurationLowerBoundaryTwoMinutes)
{
    TimeDuration result(120);
    std::string time = result.toString();

    EXPECT_EQ(time, "2 minutes, 0 seconds.");
}

TEST_F(TestTimeDuration, TestScanDurationNormalTwoMinutes)
{
    TimeDuration result(121);
    std::string time = result.toString();

    EXPECT_EQ(time, "2 minutes, 1 second.");
}

TEST_F(TestTimeDuration, TestScanDurationUpperBoundaryMinutes)
{
    TimeDuration result(3599);
    std::string time = result.toString();

    EXPECT_EQ(time, "59 minutes, 59 seconds.");
}

TEST_F(TestTimeDuration, TestScanDurationLowerBoundaryHour)
{
    TimeDuration result(3600);
    std::string time = result.toString();

    EXPECT_EQ(time, "1 hour, 0 minutes, 0 seconds.");
}

TEST_F(TestTimeDuration, TestScanDurationNormalHour)
{
    TimeDuration result(3601);
    std::string time = result.toString();

    EXPECT_EQ(time, "1 hour, 0 minutes, 1 second.");
}

TEST_F(TestTimeDuration, TestScanDurationNormalHourAndMinute)
{
    TimeDuration result(3660);
    std::string time = result.toString();

    EXPECT_EQ(time, "1 hour, 1 minute, 0 seconds.");
}

TEST_F(TestTimeDuration, TestScanDurationNormalHourMinuteSecond)
{
    TimeDuration result(3661);
    std::string time = result.toString();

    EXPECT_EQ(time, "1 hour, 1 minute, 1 second.");
}

TEST_F(TestTimeDuration, TestScanDurationNormalTwoHour)
{
    TimeDuration result(7200);
    std::string time = result.toString();

    EXPECT_EQ(time, "2 hours, 0 minutes, 0 seconds.");
}

TEST_F(TestTimeDuration, TestScanDurationNormalTwoHourSecond)
{
    TimeDuration result(7201);
    std::string time = result.toString();

    EXPECT_EQ(time, "2 hours, 0 minutes, 1 second.");
}

TEST_F(TestTimeDuration, TestScanDurationNormalTwentyFourHourSecond)
{
    TimeDuration result(86401);
    std::string time = result.toString();

    EXPECT_EQ(time, "24 hours, 0 minutes, 1 second.");
}

TEST_F(TestTimeDuration, TestScanDurationNormalTwentyFiveHour)
{
    TimeDuration result(90000);
    std::string time = result.toString();

    EXPECT_EQ(time, "25 hours, 0 minutes, 0 seconds.");
}