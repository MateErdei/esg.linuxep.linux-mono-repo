//Copyright 2020-2022, Sophos Limited.  All rights reserved.

#include <gtest/gtest.h>
#include <datatypes/Print.h>
#include "manager/scheduler/DaySet.h"
#include <tests/common/LogInitializedTests.h>

using namespace manager::scheduler;

namespace
{
    class TestDaySet : public LogInitializedTests
    {
        public:
            Common::XmlUtilities::AttributesMap m_days = Common::XmlUtilities::parseXml(
                    R"MULTILINE(<?xml version="1.0" encoding="utf-8"?>
                    <daySet>
                        <day>monday</day>
                        <day>tuesday</day>
                        <day>wednesday</day>
                        <day>thursday</day>
                        <day>friday</day>
                        <day>saturday</day>
                        <day>sunday</day>
                    </daySet>)MULTILINE");

            Common::XmlUtilities::AttributesMap m_days_invalid = Common::XmlUtilities::parseXml(
                    R"MULTILINE(<?xml version="1.0" encoding="utf-8"?>
                    <daySet>
                        <day>monday</day>
                        <day>tuesday</day>
                        <day>wednesday</day>
                        <day>thursday</day>
                        <day>friday</day>
                        <day>saturday</day>
                        <day>sunday</day>
                        <day>not a day</day>
                    </daySet>)MULTILINE");

            Common::XmlUtilities::AttributesMap m_one_invalid_day = Common::XmlUtilities::parseXml(
                R"MULTILINE(<?xml version="1.0" encoding="utf-8"?>
                    <daySet>
                        <day>not a day</day>
                    </daySet>)MULTILINE");

            Common::XmlUtilities::AttributesMap m_sunday = Common::XmlUtilities::parseXml(
                    R"MULTILINE(<?xml version="1.0" encoding="utf-8"?>
                    <daySet>
                        <day>sunday</day>
                    </daySet>)MULTILINE");

        Common::XmlUtilities::AttributesMap m_days_empty = Common::XmlUtilities::parseXml(
            R"MULTILINE(<?xml version="1.0" encoding="utf-8"?>
                    <daySet />)MULTILINE");
    };
}

TEST_F(TestDaySet, construction)
{
    DaySet set(m_days_invalid, "daySet/day");
    ASSERT_EQ(set.str(), "Days: INVALID Sunday Monday Tuesday Wednesday Thursday Friday Saturday ");
}

TEST_F(TestDaySet, getNextScanTimeDeltaTommorow)
{
    DaySet set(m_days, "daySet/day");
    time_t now = ::time(nullptr);
    struct tm now_struct{};
    ::localtime_r(&now, &now_struct);

    ASSERT_EQ(set.getNextScanTimeDelta(now_struct, true), 1);
}

TEST_F(TestDaySet, getNextScanTimeDeltaToday)
{
    DaySet set(m_days, "daySet/day");
    time_t now = ::time(nullptr);
    struct tm now_struct{};
    ::localtime_r(&now, &now_struct);

    ASSERT_EQ(set.getNextScanTimeDelta(now_struct, false), 0);
}

TEST_F(TestDaySet, getNextScanTimeDeltaMondaytoSunday)
{
    DaySet set(m_sunday, "daySet/day");
    struct tm now_struct{};
    //Monday
    now_struct.tm_wday = 1;
    ASSERT_EQ(set.getNextScanTimeDelta(now_struct, true), 6);
}

TEST_F(TestDaySet, getNextScanTimeDeltaSundaytoSunday)
{
    DaySet set(m_sunday, "daySet/day");
    struct tm now_struct{};
    //Sunday
    now_struct.tm_wday = 0;
    ASSERT_EQ(set.getNextScanTimeDelta(now_struct, true), 7);
}

TEST_F(TestDaySet, getNextScanTimeDeltaNoDays)
{
    DaySet set(m_days_empty, "daySet/day");
    ASSERT_EQ(set.size(), 0);
    time_t now = ::time(nullptr);
    struct tm now_struct{};
    ::localtime_r(&now, &now_struct);

    EXPECT_THROW(set.getNextScanTimeDelta(now_struct, true), std::out_of_range);
}

TEST_F(TestDaySet, returnsInvalidIfDaySetHasAInvalidEntry)
{
    DaySet set(m_days_invalid, "daySet/day");
    EXPECT_EQ(set.size(), 8);
    EXPECT_EQ(set.isValid(), false);
}

TEST_F(TestDaySet, returnsInvalidIfDaySetSizeOneIsInvalid)
{
    DaySet set(m_one_invalid_day, "daySet/day");
    EXPECT_EQ(set.size(), 1);
    EXPECT_EQ(set.isValid(), false);
}

TEST_F(TestDaySet, returnsValidWhenDaysAreValid)
{
    DaySet set(m_days, "daySet/day");
    EXPECT_EQ(set.size(), 7);
    EXPECT_EQ(set.isValid(), true);
}