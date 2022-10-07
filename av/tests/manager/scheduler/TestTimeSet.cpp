//Copyright 2020-2022, Sophos Limited.  All rights reserved.

#include <gtest/gtest.h>
#include "manager/scheduler/TimeSet.h"
#include <tests/common/MemoryAppender.h>

using namespace manager::scheduler;

namespace
{
    class TestTimeSet : public MemoryAppenderUsingTests
    {
        public:
            TestTimeSet() : MemoryAppenderUsingTests("ScanScheduler") {}
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

            Common::XmlUtilities::AttributesMap m_a_invalid_time = Common::XmlUtilities::parseXml(
                R"MULTILINE(<?xml version="1.0" encoding="utf-8"?>
                        <timeSet>
                            <time>imnotatime</time>
                            <time>01:00:00</time>
                            <time>03:00:00</time>
                            <time>04:00:00</time>
                            <time>18:00:00</time>
                            <time>22:00:00</time>
                            <time>23:00:00</time>
                        </timeSet>)MULTILINE");

            Common::XmlUtilities::AttributesMap m_just_invalid_time = Common::XmlUtilities::parseXml(
                R"MULTILINE(<?xml version="1.0" encoding="utf-8"?>
                        <timeSet>
                            <time>imnotatime</time>
                        </timeSet>)MULTILINE");

        Common::XmlUtilities::AttributesMap m_time_empty = Common::XmlUtilities::parseXml(
            R"MULTILINE(<?xml version="1.0" encoding="utf-8"?>
                        <timeSet />)MULTILINE");

    };
}

TEST_F(TestTimeSet, construction)
{
    TimeSet set(m_time, "timeSet/time");
    ASSERT_EQ(set.str(), "Times: 00:00:00 01:00:00 03:00:00 04:00:00 18:00:00 22:00:00 23:00:00 ");
}

TEST_F(TestTimeSet, getNextTimeIsToday)
{
    TimeSet set(m_time, "timeSet/time");

    struct tm now {};
    now.tm_hour = 2;
    bool forceTomorrow = false;

    auto next = set.getNextTime(now, forceTomorrow);

    EXPECT_EQ(next.str(), "03:00:00");
    EXPECT_EQ(forceTomorrow, false);
}

TEST_F(TestTimeSet, getNextTimeIsTomorrow)
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

TEST_F(TestTimeSet, getNextTimeWithNoTimes)
{
    TimeSet set(m_time_empty, "timeSet/time");
    ASSERT_EQ(set.size(), 0);

    struct tm now {};
    bool forceTomorrow = false;

    EXPECT_THROW(set.getNextTime(now, forceTomorrow), std::out_of_range);
}

TEST_F(TestTimeSet, lessOperatorEqualValues)
{
    Time lhs("00:00:00");
    Time rhs("00:00:00");

    ASSERT_FALSE(lhs < rhs);
}

TEST_F(TestTimeSet, lessOperatorLhsMoreHours)
{
    Time lhs("01:00:00");
    Time rhs("00:00:00");

    ASSERT_FALSE(lhs < rhs);
}

TEST_F(TestTimeSet, lessOperatorLhsMoreMinutes)
{
    Time lhs("00:01:00");
    Time rhs("00:00:00");

    ASSERT_FALSE(lhs < rhs);
}

TEST_F(TestTimeSet, lessOperatorLhsMoreSeconds)
{
    Time lhs("00:00:01");
    Time rhs("00:00:00");

    ASSERT_FALSE(lhs < rhs);
}

TEST_F(TestTimeSet, lessOperatorRhsMoreHours)
{
    Time lhs("00:00:00");
    Time rhs("01:00:00");

    ASSERT_TRUE(lhs < rhs);
}

TEST_F(TestTimeSet, lessOperatorRhsMoreMinutes)
{
    Time lhs("00:00:00");
    Time rhs("00:01:00");

    ASSERT_TRUE(lhs < rhs);
}

TEST_F(TestTimeSet, lessOperatorrhsMoreSeconds)
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

TEST_F(TestTimeSet, stillDueTodaySecondComparisonReturnsTrue)
{
    Time fakeScanTime("10:35:54");
    struct tm now_struct{};
    now_struct.tm_hour = 10;
    now_struct.tm_min = 35;
    now_struct.tm_sec = 53;
    ASSERT_TRUE(fakeScanTime.stillDueToday(now_struct));
}

TEST_F(TestTimeSet, stillDueTodaySecondComparisonReturnsFalse)
{
    Time fakeScanTime("10:35:54");
    struct tm now_struct{};
    now_struct.tm_hour = 10;
    now_struct.tm_min = 35;
    now_struct.tm_sec = 54;
    ASSERT_FALSE(fakeScanTime.stillDueToday(now_struct));
}

TEST_F(TestTimeSet, onlyInvalidTime)
{
    TimeSet set(m_just_invalid_time, "timeSet/time");
    EXPECT_EQ(set.size(), 1);
    ASSERT_EQ(set.isValid(), false);

    EXPECT_EQ(set.str(), "Times: INVALID ");
}

TEST_F(TestTimeSet, mixValidInvalidTimes)
{
    TimeSet set(m_a_invalid_time, "timeSet/time");
    EXPECT_EQ(set.size(), 7);
    ASSERT_EQ(set.isValid(), false);

    auto str = set.str();
    EXPECT_TRUE(str.find("INVALID") != str.npos);
}

TEST_F(TestTimeSet, validTimes)
{
    TimeSet set(m_time, "timeSet/time");
    EXPECT_EQ(set.size(), 7);
    ASSERT_EQ(set.isValid(), true);

    auto str = set.str();
    EXPECT_TRUE(str.find("INVALID") == str.npos);
}

TEST_F(TestTimeSet, logsWarningWhenTimeInvalidOnConstruction)
{
    UsingMemoryAppender memoryAppenderHolder(*this);
    TimeSet set(m_just_invalid_time, "timeSet/time");
    EXPECT_TRUE(appenderContains("Invalid time from policy: imnotatime"));
}