/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include <gtest/gtest.h>
#include "manager/scheduler/TimeSet.h"
#include <tests/common/LogInitializedTests.h>

using namespace manager::scheduler;

namespace
{
    class TestTimeSet : public LogInitializedTests
    {
        public:
            Common::XmlUtilities::AttributesMap m_time = Common::XmlUtilities::parseXml(
                    R"MULTILINE(<?xml version="1.0" encoding="utf-8"?>
                        <timeSet>
                            <time>00:00:00</time>
                            <time>01:00:00</time>
                            <time>03:00:00</time>
                            <time>04:00:00</time>
                            <time>18:00:00</time>
                            <time>22:00:00</time>
                            <time>23:00:00</time>
                        </timeSet>)MULTILINE");

        Common::XmlUtilities::AttributesMap m_time_empty = Common::XmlUtilities::parseXml(
            R"MULTILINE(<?xml version="1.0" encoding="utf-8"?>
                        <timeSet />)MULTILINE");

    };
}

TEST_F(TestTimeSet, construction) // NOLINT
{
    TimeSet set(m_time, "timeSet/time");
    ASSERT_EQ(set.str(), "Times: 00:00:00 01:00:00 03:00:00 04:00:00 18:00:00 22:00:00 23:00:00 ");
}

TEST_F(TestTimeSet, getNextTimeIsToday) // NOLINT
{
    TimeSet set(m_time, "timeSet/time");

    struct tm now {};
    now.tm_hour = 2;
    bool forceTomorrow = false;

    auto next = set.getNextTime(now, forceTomorrow);

    EXPECT_EQ(next.str(), "03:00:00");
    EXPECT_EQ(forceTomorrow, false);
}

TEST_F(TestTimeSet, getNextTimeIsTomorrow) // NOLINT
{
    TimeSet set(m_time, "timeSet/time");

    struct tm now {};
    now.tm_hour = 23;
    now.tm_min = 30;
    bool forceTomorrow = false;

    auto next = set.getNextTime(now, forceTomorrow);

    EXPECT_EQ(next.str(), "00:00:00");
    EXPECT_EQ(forceTomorrow, true);
}

TEST_F(TestTimeSet, getNextTimeWithNoTimes) // NOLINT
{
    TimeSet set(m_time_empty, "timeSet/time");
    ASSERT_EQ(set.size(), 0);

    struct tm now {};
    bool forceTomorrow = false;

    EXPECT_THROW(set.getNextTime(now, forceTomorrow), std::out_of_range);
}

TEST_F(TestTimeSet, lessOperatorEqualValues) // NOLINT
{
    Time lhs("00:00:00");
    Time rhs("00:00:00");

    ASSERT_FALSE(lhs < rhs);
}

TEST_F(TestTimeSet, lessOperatorLhsMoreHours) // NOLINT
{
    Time lhs("01:00:00");
    Time rhs("00:00:00");

    ASSERT_FALSE(lhs < rhs);
}

TEST_F(TestTimeSet, lessOperatorLhsMoreMinutes) // NOLINT
{
    Time lhs("00:01:00");
    Time rhs("00:00:00");

    ASSERT_FALSE(lhs < rhs);
}

TEST_F(TestTimeSet, lessOperatorLhsMoreSeconds) // NOLINT
{
    Time lhs("00:00:01");
    Time rhs("00:00:00");

    ASSERT_FALSE(lhs < rhs);
}

TEST_F(TestTimeSet, lessOperatorRhsMoreHours) // NOLINT
{
    Time lhs("00:00:00");
    Time rhs("01:00:00");

    ASSERT_TRUE(lhs < rhs);
}

TEST_F(TestTimeSet, lessOperatorRhsMoreMinutes) // NOLINT
{
    Time lhs("00:00:00");
    Time rhs("00:01:00");

    ASSERT_TRUE(lhs < rhs);
}

TEST_F(TestTimeSet, lessOperatorrhsMoreSeconds) // NOLINT
{
    Time lhs("00:00:00");
    Time rhs("00:00:01");

    ASSERT_TRUE(lhs < rhs);
}

TEST_F(TestTimeSet, stillDueTodayReturnsTrue)
{
    Time fakeScanTime("11:00:00");
    struct tm now_struct{};
    now_struct.tm_hour = 10;
    ASSERT_TRUE(fakeScanTime.stillDueToday(now_struct));
}

TEST_F(TestTimeSet, stillDueTodayReturnsFalse)
{
    Time fakeScanTime("10:00:00");
    struct tm now_struct{};
    now_struct.tm_hour = 11;
    ASSERT_FALSE(fakeScanTime.stillDueToday(now_struct));
}

TEST_F(TestTimeSet, stillDueTodayMinuteComparisonReturnsTrue)
{
    Time fakeScanTime("10:35:00");
    struct tm now_struct{};
    now_struct.tm_hour = 10;
    now_struct.tm_min = 30;
    ASSERT_TRUE(fakeScanTime.stillDueToday(now_struct));
}

TEST_F(TestTimeSet, stillDueTodayMinuteComparisonReturnsFalse)
{
    Time fakeScanTime("10:35:00");
    struct tm now_struct{};
    now_struct.tm_hour = 10;
    now_struct.tm_min = 35;
    ASSERT_FALSE(fakeScanTime.stillDueToday(now_struct));
}