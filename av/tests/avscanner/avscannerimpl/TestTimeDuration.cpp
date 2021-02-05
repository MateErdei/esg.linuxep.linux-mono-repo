/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <pluginapi/include/Common/Logging/SophosLoggerMacros.h>

#include "avscanner/avscannerimpl/TimeDuration.h"
#include "tests/common/LogInitializedTests.h"
#include <gtest/gtest.h>

using namespace avscanner::avscannerimpl;

class TestTimeDuration : public LogInitializedTests
{
};

TEST_F(TestTimeDuration, TestScanDurationZeroSeconds) // NOLINT
{
    TimeDuration timeDuration;
    TimeDuration result = timeDuration.timeConversion(0);

    EXPECT_EQ(result.sec, 0);
    EXPECT_EQ(result.min, 0);
    EXPECT_EQ(result.hour, 0);
}

TEST_F(TestTimeDuration, TestScanDurationLowerBoundarySecond) // NOLINT
{
    TimeDuration timeDuration;
    TimeDuration result = timeDuration.timeConversion(1);

    EXPECT_EQ(result.sec, 1);
    EXPECT_EQ(result.min, 0);
    EXPECT_EQ(result.hour, 0);
}

TEST_F(TestTimeDuration, TestScanDurationNormalSecond) // NOLINT
{
    TimeDuration timeDuration;
    TimeDuration result = timeDuration.timeConversion(2);

    EXPECT_EQ(result.sec, 2);
    EXPECT_EQ(result.min, 0);
    EXPECT_EQ(result.hour, 0);
}

TEST_F(TestTimeDuration, TestScanDurationUpperBoundarySecond) // NOLINT
{
    TimeDuration timeDuration;
    TimeDuration result = timeDuration.timeConversion(59);

    EXPECT_EQ(result.sec, 59);
    EXPECT_EQ(result.min, 0);
    EXPECT_EQ(result.hour, 0);
}

TEST_F(TestTimeDuration, TestScanDurationLowerBoundaryMinute) // NOLINT
{
    TimeDuration timeDuration;
    TimeDuration result = timeDuration.timeConversion(60);

    EXPECT_EQ(result.sec, 0);
    EXPECT_EQ(result.min, 1);
    EXPECT_EQ(result.hour, 0);
}

TEST_F(TestTimeDuration, TestScanDurationNormalMinute) // NOLINT
{
    TimeDuration timeDuration;
    TimeDuration result = timeDuration.timeConversion(61);

    EXPECT_EQ(result.sec, 1);
    EXPECT_EQ(result.min, 1);
    EXPECT_EQ(result.hour, 0);
}

TEST_F(TestTimeDuration, TestScanDurationLowerBoundaryTwoMinutes) // NOLINT
{
    TimeDuration timeDuration;
    TimeDuration result = timeDuration.timeConversion(120);

    EXPECT_EQ(result.sec, 0);
    EXPECT_EQ(result.min, 2);
    EXPECT_EQ(result.hour, 0);
}

TEST_F(TestTimeDuration, TestScanDurationNormalTwoMinutes) // NOLINT
{
    TimeDuration timeDuration;
    TimeDuration result = timeDuration.timeConversion(121);

    EXPECT_EQ(result.sec, 1);
    EXPECT_EQ(result.min, 2);
    EXPECT_EQ(result.hour, 0);
}

TEST_F(TestTimeDuration, TestScanDurationUpperBoundaryMinutes) // NOLINT
{
    TimeDuration timeDuration;
    TimeDuration result = timeDuration.timeConversion(3599);

    EXPECT_EQ(result.sec, 59);
    EXPECT_EQ(result.min, 59);
    EXPECT_EQ(result.hour, 0);
}

TEST_F(TestTimeDuration, TestScanDurationLowerBoundaryHour) // NOLINT
{
    TimeDuration timeDuration;

    TimeDuration result = timeDuration.timeConversion(3600);

    EXPECT_EQ(result.sec, 0);
    EXPECT_EQ(result.min, 0);
    EXPECT_EQ(result.hour, 1);
}

TEST_F(TestTimeDuration, TestScanDurationNormalHour) // NOLINT
{
    TimeDuration timeDuration;

    TimeDuration result = timeDuration.timeConversion(3601);

    EXPECT_EQ(result.sec, 1);
    EXPECT_EQ(result.min, 0);
    EXPECT_EQ(result.hour, 1);
}

TEST_F(TestTimeDuration, TestScanDurationNormalHourAndMinute) // NOLINT
{
    TimeDuration timeDuration;

    TimeDuration result = timeDuration.timeConversion(3660);

    EXPECT_EQ(result.sec, 0);
    EXPECT_EQ(result.min, 1);
    EXPECT_EQ(result.hour, 1);
}

TEST_F(TestTimeDuration, TestScanDurationNormalHourMinuteSecond) // NOLINT
{
    TimeDuration timeDuration;

    TimeDuration result = timeDuration.timeConversion(3661);

    EXPECT_EQ(result.sec, 1);
    EXPECT_EQ(result.min, 1);
    EXPECT_EQ(result.hour, 1);
}

TEST_F(TestTimeDuration, TestScanDurationNormalTwoHour) // NOLINT
{
    TimeDuration timeDuration;

    TimeDuration result = timeDuration.timeConversion(7200);

    EXPECT_EQ(result.sec, 0);
    EXPECT_EQ(result.min, 0);
    EXPECT_EQ(result.hour, 2);
}

TEST_F(TestTimeDuration, TestScanDurationNormalTwoHourSecond) // NOLINT
{
    TimeDuration timeDuration;

    TimeDuration result = timeDuration.timeConversion(7201);

    EXPECT_EQ(result.sec, 1);
    EXPECT_EQ(result.min, 0);
    EXPECT_EQ(result.hour, 2);
}