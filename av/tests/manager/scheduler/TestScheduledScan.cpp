/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "manager/scheduler/ScheduledScan.h"

#include <gtest/gtest.h>

using namespace manager::scheduler;

TEST(ScheduledScan, TestConstructionInvalid) //NOLINT
{
    auto invalidScan = ScheduledScan();

    EXPECT_FALSE(invalidScan.valid());
    EXPECT_FALSE(invalidScan.isScanNow());
    EXPECT_FALSE(invalidScan.archiveScanning());

    EXPECT_EQ(invalidScan.name(), "INVALID");
    EXPECT_EQ(invalidScan.days().size(), 0);
    EXPECT_EQ(invalidScan.times().size(), 0);

    EXPECT_EQ(invalidScan.calculateNextTime(time_t(nullptr)), static_cast<time_t>(-1));
}

TEST(ScheduledScan, TestConstructionScanNow) //NOLINT
{
    std::string name = "Scan Now";
    auto scanNowScan = ScheduledScan(name);

    EXPECT_TRUE(scanNowScan.valid());
    EXPECT_TRUE(scanNowScan.isScanNow());
    EXPECT_FALSE(scanNowScan.archiveScanning());

    EXPECT_EQ(scanNowScan.name(), name);
    EXPECT_EQ(scanNowScan.days().size(), 0);
    EXPECT_EQ(scanNowScan.times().size(), 0);

    EXPECT_EQ(scanNowScan.calculateNextTime(time_t(nullptr)), static_cast<time_t>(-1));
}

TEST(ScheduledScan, TestConstructionScheduledScan) //NOLINT
{
    auto attributeMap = Common::XmlUtilities::parseXml(
            R"MULTILINE(
      <scanId>
        <name>Sophos Cloud Scheduled Scan</name>
        <schedule>
          <daySet>
            <!-- for day in {{scheduledScanDays}} -->
            <day>saturday</day>
            <day>thursday</day>
          </daySet>
          <timeSet>
            <time>00:00:00</time>
          </timeSet>
        </schedule>
      </scanId>
)MULTILINE");

    std::basic_string id = "scanId";
    auto scheduledScan = ScheduledScan(attributeMap, id);

    EXPECT_TRUE(scheduledScan.valid());
    EXPECT_FALSE(scheduledScan.isScanNow());
    EXPECT_FALSE(scheduledScan.archiveScanning());

    EXPECT_EQ(scheduledScan.name(), "Sophos Cloud Scheduled Scan");

    ASSERT_EQ(scheduledScan.days().size(), 2);
    EXPECT_EQ(scheduledScan.days().days()[0], THURSDAY);
    EXPECT_EQ(scheduledScan.days().days()[1], SATURDAY);

    ASSERT_EQ(scheduledScan.times().size(), 1);
    EXPECT_EQ(scheduledScan.times().times()[0].hour(),0);
    EXPECT_EQ(scheduledScan.times().times()[0].minute(),0);
}

TEST(ScheduledScan, DaySet) // NOLINT
{

    auto attributeMap = Common::XmlUtilities::parseXml(
            R"MULTILINE(<?xml version="1.0"?>
<config xmlns="http://www.sophos.com/EE/EESavConfiguration">
  <csc:Comp xmlns:csc="com.sophos\msys\csc" RevID="" policyType="2"/>
  <onDemandScan>
    <scanSet>
      <scan>
        <name>Sophos Cloud Scheduled Scan</name>
        <schedule>
          <daySet>
            <!-- for day in {{scheduledScanDays}} -->
            <day>saturday</day>
            <day>thursday</day>
          </daySet>
          <timeSet>
            <time>00:00:00</time>
          </timeSet>
        </schedule>
      </scan>
    </scanSet>
  </onDemandScan>
</config>
)MULTILINE");

    auto scanIds = attributeMap.entitiesThatContainPath("config/onDemandScan/scanSet/scan", false);
    ASSERT_EQ(scanIds.size(), 1);
    ASSERT_EQ(scanIds[0], "config/onDemandScan/scanSet/scan");

    auto days_from_xml = attributeMap.lookupMultiple("config/onDemandScan/scanSet/scan/schedule/daySet/day");
    ASSERT_EQ(days_from_xml.size(), 2);


    // And with the real code
    auto scan = ScheduledScan(attributeMap, scanIds[0]);
    EXPECT_EQ(scan.name(), "Sophos Cloud Scheduled Scan");
    const auto& days = scan.days();
    ASSERT_EQ(days.size(), 2);

}

TEST(ScheduledScan, DaySetEmpty) // NOLINT
{
    auto attributeMap = Common::XmlUtilities::parseXml(
        R"MULTILINE(<?xml version="1.0"?>
<config xmlns="http://www.sophos.com/EE/EESavConfiguration">
  <csc:Comp xmlns:csc="com.sophos\msys\csc" RevID="" policyType="2"/>
  <onDemandScan>
    <scanSet>
      <scan>
        <name>Sophos Cloud Scheduled Scan</name>
        <schedule>
          <daySet>
          </daySet>
          <timeSet>
            <time>00:00:00</time>
          </timeSet>
        </schedule>
      </scan>
    </scanSet>
  </onDemandScan>
</config>
)MULTILINE");

    auto scanIds = attributeMap.entitiesThatContainPath("config/onDemandScan/scanSet/scan", false);
    ASSERT_EQ(scanIds.size(), 1);
    ASSERT_EQ(scanIds[0], "config/onDemandScan/scanSet/scan");

    auto days_from_xml = attributeMap.lookupMultiple("config/onDemandScan/scanSet/scan/schedule/daySet/day");
    ASSERT_EQ(days_from_xml.size(), 0);


    // And with the real code
    auto scan = ScheduledScan(attributeMap, scanIds[0]);
    EXPECT_EQ(scan.name(), "Sophos Cloud Scheduled Scan");
    const auto& days = scan.days();
    ASSERT_EQ(days.size(), 0);

    EXPECT_FALSE(scan.valid());
    EXPECT_FALSE(scan.isScanNow());
    EXPECT_EQ(scan.calculateNextTime(time_t(nullptr)), static_cast<time_t>(-1));

    EXPECT_NO_THROW((void) scan.str());
}


TEST(ScheduledScan, TimeSetEmpty) // NOLINT
{
    auto attributeMap = Common::XmlUtilities::parseXml(
        R"MULTILINE(<?xml version="1.0"?>
<config xmlns="http://www.sophos.com/EE/EESavConfiguration">
  <csc:Comp xmlns:csc="com.sophos\msys\csc" RevID="" policyType="2"/>
  <onDemandScan>
    <scanSet>
      <scan>
        <name>Sophos Cloud Scheduled Scan</name>
        <schedule>
          <daySet>
            <day>saturday</day>
            <day>thursday</day>
          </daySet>
          <timeSet>
          </timeSet>
        </schedule>
      </scan>
    </scanSet>
  </onDemandScan>
</config>
)MULTILINE");

    auto scanIds = attributeMap.entitiesThatContainPath("config/onDemandScan/scanSet/scan", false);
    ASSERT_EQ(scanIds.size(), 1);
    ASSERT_EQ(scanIds[0], "config/onDemandScan/scanSet/scan");

    auto times_from_xml = attributeMap.lookupMultiple("config/onDemandScan/scanSet/scan/schedule/timeSet/time");
    ASSERT_EQ(times_from_xml.size(), 0);


    // And with the real code
    auto scan = ScheduledScan(attributeMap, scanIds[0]);
    EXPECT_EQ(scan.name(), "Sophos Cloud Scheduled Scan");
    const auto& times = scan.times();
    EXPECT_EQ(times.size(), 0);

    EXPECT_FALSE(scan.valid());
    EXPECT_FALSE(scan.isScanNow());
    EXPECT_EQ(scan.calculateNextTime(time_t(nullptr)), static_cast<time_t>(-1));

    EXPECT_NO_THROW((void) scan.str());
}
