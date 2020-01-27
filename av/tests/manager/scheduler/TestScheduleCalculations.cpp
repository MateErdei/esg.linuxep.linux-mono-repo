/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gtest/gtest.h>

#include "manager/scheduler/ScheduledScanConfiguration.h"

#include <Common/Logging/ConsoleLoggingSetup.h>

using namespace manager::scheduler;

static Common::Logging::ConsoleLoggingSetup consoleLoggingSetup;

TEST(ScheduleCalculations, tomorrow) // NOLINT
{
    auto attributeMap = Common::XmlUtilities::parseXml(
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
    ScheduledScan scan(attributeMap, "scan");

    time_t now = 1580281980; // Wednesday 07:13:00 GMT - should we Wednesday or Tuesday everywhere
    auto result = scan.calculateNextTime(now);

    // should be Thursday 00:00 in localtime
    struct tm* resulttm = localtime(&result);
    EXPECT_EQ(resulttm->tm_hour, 0);
    EXPECT_EQ(resulttm->tm_min, 0);
    EXPECT_EQ(resulttm->tm_wday, 4);
}
