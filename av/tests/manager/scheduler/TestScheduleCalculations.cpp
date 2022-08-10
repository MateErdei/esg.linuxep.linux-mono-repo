/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gtest/gtest.h>

#include "manager/scheduler/ScheduledScanConfiguration.h"

#include <datatypes/Print.h>

using namespace manager::scheduler;

static auto getThursdayAndSaturdayMidnightAttributeMap()
{
    return Common::XmlUtilities::parseXml(
            R"MULTILINE(<?xml version="1.0"?>
<scan>
        <name>X</name>
        <schedule>
          <daySet>
            <day>saturday</day>
            <day>thursday</day>
          </daySet>
          <timeSet>
            <time>00:00:00</time>
          </timeSet>
        </schedule>
      </scan>
)MULTILINE");
}

static auto getThursdayMiddayScheduledScan()
{
    auto attributeMap = Common::XmlUtilities::parseXml(
            R"MULTILINE(<?xml version="1.0"?>
<scan>
        <name>X</name>
        <schedule>
          <daySet>
            <day>thursday</day>
          </daySet>
          <timeSet>
            <time>12:00:00</time>
          </timeSet>
        </schedule>
      </scan>
)MULTILINE");
    return ScheduledScan(attributeMap, "scan");
}

static auto getThursdayAndSaturdayMidnightScheduledScan()
{
    auto attributeMap = getThursdayAndSaturdayMidnightAttributeMap();
    return ScheduledScan(attributeMap, "scan");
}

TEST(ScheduleCalculations, tomorrow) // NOLINT
{
    ScheduledScan scan = getThursdayAndSaturdayMidnightScheduledScan();

    time_t now = 1580281980; // Wednesday 07:13:00 GMT - should we Wednesday or Tuesday everywhere -  Wed Jan 29 07:13:00 2020
    auto result = scan.calculateNextTime(now);

    // should be Thursday 00:00 in local time
    struct tm resulttm{};
    ::localtime_r(&result, &resulttm);

    EXPECT_EQ(resulttm.tm_hour, 0);
    EXPECT_EQ(resulttm.tm_min, 0);
    EXPECT_EQ(resulttm.tm_wday, 4);
}

TEST(ScheduleCalculations, Saturday) // NOLINT
{
    ScheduledScan scan = getThursdayAndSaturdayMidnightScheduledScan();

    time_t now = 1580385600; // Thu Jan 30 12:00:00 2020 +0000
    auto result = scan.calculateNextTime(now);

    // should be Thursday 00:00 in local time
    struct tm resulttm{};
    ::localtime_r(&result, &resulttm);

    EXPECT_EQ(resulttm.tm_hour, 0);
    EXPECT_EQ(resulttm.tm_min, 0);
    EXPECT_EQ(resulttm.tm_wday, 6);
}


TEST(ScheduleCalculations, NextWeek) // NOLINT
{
    ScheduledScan scan = getThursdayAndSaturdayMidnightScheduledScan();
    time_t now = 1580558400; // Sat Feb  1 12:00:00 2020 +0000
    auto result = scan.calculateNextTime(now);

    // should be Thursday 00:00 in local time
    struct tm resulttm{};
    ::localtime_r(&result, &resulttm);
    EXPECT_EQ(resulttm.tm_hour, 0);
    EXPECT_EQ(resulttm.tm_min, 0);
    EXPECT_EQ(resulttm.tm_wday, 4); // Thursday
    EXPECT_EQ(resulttm.tm_mday, 6); // Thursday 6th
    EXPECT_EQ(resulttm.tm_mon, 1); // February
}

TEST(ScheduleCalculations, ThursdayWrapped) // NOLINT
{
    ScheduledScan scan(getThursdayMiddayScheduledScan());
    time_t now = 1580385601; // Thu Jan 30 12:00:01 2020 +0000
    // Convert to local time
    struct tm now_tm{};
    struct tm* now_tm_result = ::localtime_r(&now, &now_tm);
    ASSERT_NE(now_tm_result, nullptr);
    now_tm.tm_mon = 0;
    now_tm.tm_mday = 30;
    now_tm.tm_hour = 12;
    now_tm.tm_min = 0;
    now_tm.tm_sec = 1;
    now = ::mktime(&now_tm);

    auto result = scan.calculateNextTime(now);

    // should be Thursday 12:00 in local time
    struct tm resulttm{};
    ::localtime_r(&result, &resulttm);
    EXPECT_EQ(resulttm.tm_hour, 12);
    EXPECT_EQ(resulttm.tm_min, 0);
    EXPECT_EQ(resulttm.tm_wday, 4); // Thursday
    EXPECT_EQ(resulttm.tm_mday, 6); // Thursday 6th
    EXPECT_EQ(resulttm.tm_mon, 1); // February
}

TEST(ScheduleCalculations, ThursdayNotWrapped) // NOLINT
{
    ScheduledScan scan(getThursdayMiddayScheduledScan());
    time_t now = 1580385601; // Thu Jan 30 12:00:01 2020 +0000
    // Convert to local time
    struct tm now_tm{};
    struct tm* now_tm_result = ::localtime_r(&now, &now_tm);
    ASSERT_NE(now_tm_result, nullptr);
    now_tm.tm_mon = 0;
    now_tm.tm_mday = 30;
    now_tm.tm_hour = 11;
    now_tm.tm_min = 59;
    now_tm.tm_sec = 1;
    now = ::mktime(&now_tm);

    auto result = scan.calculateNextTime(now);

    // should be Thursday 12:00 in local time
    struct tm resulttm{};
    ::localtime_r(&result, &resulttm);
    EXPECT_EQ(resulttm.tm_hour, 12);
    EXPECT_EQ(resulttm.tm_min, 0);
    EXPECT_EQ(resulttm.tm_wday, 4); // Thursday
    EXPECT_EQ(resulttm.tm_mday, 30); // Thursday 30th
    EXPECT_EQ(resulttm.tm_mon, 0); // Jan
}

