/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "avscanner/avscannerimpl/TimeDuration.h"
#include "tests/common/LogInitializedTests.h"

#include <gtest/gtest.h>
#include <pluginapi/include/Common/Logging/SophosLoggerMacros.h>
#include <tests/googletest/googlemock/include/gmock/gmock-matchers.h>

using namespace avscanner::avscannerimpl;

class TestTimeDuration : public LogInitializedTests
{
};

TEST_F(TestTimeDuration, TestScanDurationZeroSeconds) // NOLINT
{
    TimeDuration result(0);

    EXPECT_EQ(result.getSeconds(), 0);
    EXPECT_EQ(result.getMinutes(), 0);
    EXPECT_EQ(result.getHours(), 0);

    std::string time = result.toString();

    EXPECT_EQ(time, "less than a second");
    EXPECT_THAT(time, Not(testing::HasSubstr("hour")));
    EXPECT_THAT(time, Not(testing::HasSubstr("minute")));
    EXPECT_THAT(time, Not(testing::HasSubstr("seconds")));
    EXPECT_THAT(time, Not(testing::HasSubstr("second.")));
}

TEST_F(TestTimeDuration, TestScanDurationLowerBoundarySecond) // NOLINT
{
    TimeDuration result(1);

    EXPECT_EQ(result.getSeconds(), 1);
    EXPECT_EQ(result.getMinutes(), 0);
    EXPECT_EQ(result.getHours(), 0);

    std::string time = result.toString();

    EXPECT_EQ(time, "1 second.");
    EXPECT_THAT(time, Not(testing::HasSubstr("hour")));
    EXPECT_THAT(time, Not(testing::HasSubstr("minute")));
    EXPECT_THAT(time, Not(testing::HasSubstr("seconds")));
}

TEST_F(TestTimeDuration, TestScanDurationNormalSecond) // NOLINT
{
    TimeDuration result(2);

    EXPECT_EQ(result.getSeconds(), 2);
    EXPECT_EQ(result.getMinutes(), 0);
    EXPECT_EQ(result.getHours(), 0);

    std::string time = result.toString();

    EXPECT_EQ(time, "2 seconds.");
    EXPECT_THAT(time, Not(testing::HasSubstr("hour")));
    EXPECT_THAT(time, Not(testing::HasSubstr("minute")));
    EXPECT_THAT(time, Not(testing::HasSubstr("second.")));
}

TEST_F(TestTimeDuration, TestScanDurationUpperBoundarySecond) // NOLINT
{
    TimeDuration result(59);

    EXPECT_EQ(result.getSeconds(), 59);
    EXPECT_EQ(result.getMinutes(), 0);
    EXPECT_EQ(result.getHours(), 0);

    std::string time = result.toString();

    EXPECT_EQ(time, "59 seconds.");
    EXPECT_THAT(time, Not(testing::HasSubstr("hour")));
    EXPECT_THAT(time, Not(testing::HasSubstr("minute")));
    EXPECT_THAT(time, Not(testing::HasSubstr("second.")));
}

TEST_F(TestTimeDuration, TestScanDurationLowerBoundaryMinute) // NOLINT
{
    TimeDuration result(60);

    EXPECT_EQ(result.getSeconds(), 0);
    EXPECT_EQ(result.getMinutes(), 1);
    EXPECT_EQ(result.getHours(), 0);

    std::string time = result.toString();

    EXPECT_EQ(time, "1 minute, 0 seconds.");
    EXPECT_THAT(time, Not(testing::HasSubstr("hour")));
    EXPECT_THAT(time, Not(testing::HasSubstr("minutes")));
    EXPECT_THAT(time, Not(testing::HasSubstr("second.")));
}

TEST_F(TestTimeDuration, TestScanDurationNormalMinute) // NOLINT
{
    TimeDuration result(61);

    EXPECT_EQ(result.getSeconds(), 1);
    EXPECT_EQ(result.getMinutes(), 1);
    EXPECT_EQ(result.getHours(), 0);

    std::string time = result.toString();

    EXPECT_EQ(time, "1 minute, 1 second.");
    EXPECT_THAT(time, Not(testing::HasSubstr("hour")));
    EXPECT_THAT(time, Not(testing::HasSubstr("minutes")));
    EXPECT_THAT(time, Not(testing::HasSubstr("seconds")));
}

TEST_F(TestTimeDuration, TestScanDurationLowerBoundaryTwoMinutes) // NOLINT
{
    TimeDuration result(120);

    EXPECT_EQ(result.getSeconds(), 0);
    EXPECT_EQ(result.getMinutes(), 2);
    EXPECT_EQ(result.getHours(), 0);

    std::string time = result.toString();

    EXPECT_EQ(time, "2 minutes, 0 seconds.");
    EXPECT_THAT(time, Not(testing::HasSubstr("hour")));
    EXPECT_THAT(time, Not(testing::HasSubstr("minute,")));
    EXPECT_THAT(time, Not(testing::HasSubstr("second.")));
}

TEST_F(TestTimeDuration, TestScanDurationNormalTwoMinutes) // NOLINT
{
    TimeDuration result(121);

    EXPECT_EQ(result.getSeconds(), 1);
    EXPECT_EQ(result.getMinutes(), 2);
    EXPECT_EQ(result.getHours(), 0);

    std::string time = result.toString();

    EXPECT_EQ(time, "2 minutes, 1 second.");
    EXPECT_THAT(time, Not(testing::HasSubstr("hour")));
    EXPECT_THAT(time, Not(testing::HasSubstr("minute,")));
    EXPECT_THAT(time, Not(testing::HasSubstr("seconds")));
}

TEST_F(TestTimeDuration, TestScanDurationUpperBoundaryMinutes) // NOLINT
{
    TimeDuration result(3599);

    EXPECT_EQ(result.getSeconds(), 59);
    EXPECT_EQ(result.getMinutes(), 59);
    EXPECT_EQ(result.getHours(), 0);

    std::string time = result.toString();

    EXPECT_EQ(time, "59 minutes, 59 seconds.");
    EXPECT_THAT(time, Not(testing::HasSubstr("hour")));
    EXPECT_THAT(time, Not(testing::HasSubstr("minute,")));
    EXPECT_THAT(time, Not(testing::HasSubstr("second.")));
}

TEST_F(TestTimeDuration, TestScanDurationLowerBoundaryHour) // NOLINT
{
    TimeDuration result(3600);

    EXPECT_EQ(result.getSeconds(), 0);
    EXPECT_EQ(result.getMinutes(), 0);
    EXPECT_EQ(result.getHours(), 1);

    std::string time = result.toString();

    EXPECT_EQ(time, "1 hour, 0 minutes, 0 seconds.");
    EXPECT_THAT(time, Not(testing::HasSubstr("hours")));
    EXPECT_THAT(time, Not(testing::HasSubstr("minute,")));
    EXPECT_THAT(time, Not(testing::HasSubstr("second.")));
}

TEST_F(TestTimeDuration, TestScanDurationNormalHour) // NOLINT
{
    TimeDuration result(3601);

    EXPECT_EQ(result.getSeconds(), 1);
    EXPECT_EQ(result.getMinutes(), 0);
    EXPECT_EQ(result.getHours(), 1);

    std::string time = result.toString();

    EXPECT_EQ(time, "1 hour, 0 minutes, 1 second.");
    EXPECT_THAT(time, Not(testing::HasSubstr("hours")));
    EXPECT_THAT(time, Not(testing::HasSubstr("minute,")));
    EXPECT_THAT(time, Not(testing::HasSubstr("seconds")));
}

TEST_F(TestTimeDuration, TestScanDurationNormalHourAndMinute) // NOLINT
{
    TimeDuration result(3660);

    EXPECT_EQ(result.getSeconds(), 0);
    EXPECT_EQ(result.getMinutes(), 1);
    EXPECT_EQ(result.getHours(), 1);

    std::string time = result.toString();

    EXPECT_EQ(time, "1 hour, 1 minute, 0 seconds.");
    EXPECT_THAT(time, Not(testing::HasSubstr("hours")));
    EXPECT_THAT(time, Not(testing::HasSubstr("minutes")));
    EXPECT_THAT(time, Not(testing::HasSubstr("second.")));
}

TEST_F(TestTimeDuration, TestScanDurationNormalHourMinuteSecond) // NOLINT
{
    TimeDuration result(3661);

    EXPECT_EQ(result.getSeconds(), 1);
    EXPECT_EQ(result.getMinutes(), 1);
    EXPECT_EQ(result.getHours(), 1);

    std::string time = result.toString();

    EXPECT_EQ(time, "1 hour, 1 minute, 1 second.");
    EXPECT_THAT(time, Not(testing::HasSubstr("hours")));
    EXPECT_THAT(time, Not(testing::HasSubstr("minutes")));
    EXPECT_THAT(time, Not(testing::HasSubstr("seconds")));
}

TEST_F(TestTimeDuration, TestScanDurationNormalTwoHour) // NOLINT
{
    TimeDuration result(7200);

    EXPECT_EQ(result.getSeconds(), 0);
    EXPECT_EQ(result.getMinutes(), 0);
    EXPECT_EQ(result.getHours(), 2);

    std::string time = result.toString();

    EXPECT_EQ(time, "2 hours, 0 minutes, 0 seconds.");
    EXPECT_THAT(time, Not(testing::HasSubstr("hour,")));
    EXPECT_THAT(time, Not(testing::HasSubstr("minute,")));
    EXPECT_THAT(time, Not(testing::HasSubstr("second.")));
}

TEST_F(TestTimeDuration, TestScanDurationNormalTwoHourSecond) // NOLINT
{
    TimeDuration result(7201);

    EXPECT_EQ(result.getSeconds(), 1);
    EXPECT_EQ(result.getMinutes(), 0);
    EXPECT_EQ(result.getHours(), 2);

    std::string time = result.toString();

    EXPECT_EQ(time, "2 hours, 0 minutes, 1 second.");
    EXPECT_THAT(time, Not(testing::HasSubstr("hour,")));
    EXPECT_THAT(time, Not(testing::HasSubstr("minute,")));
    EXPECT_THAT(time, Not(testing::HasSubstr("seconds")));
}