/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#include <gtest/gtest.h>

#include "manager/scheduler/ScanSerialiser.h"
#include "manager/scheduler/ScheduledScanConfiguration.h"

using namespace manager::scheduler;

TEST(ScanSerialiser, TestEmptyScan) // NOLINT
{
    auto attributeMap = Common::XmlUtilities::parseXml(
            R"MULTILINE(<?xml version="1.0"?>
<config xmlns="http://www.sophos.com/EE/EESavConfiguration">
  <csc:Comp xmlns:csc="com.sophos\msys\csc" RevID="" policyType="2"/>
  <onDemandScan>
    <scanSet>
      <scan>
        <name>Sophos Cloud Scheduled Scan</name>
      </scan>
    </scanSet>
  </onDemandScan>
</config>
)MULTILINE");

    auto m = std::make_unique<ScheduledScanConfiguration>(attributeMap);
    auto scans = m->scans();
    ASSERT_EQ(scans.size(), 1);
    const auto& scan = scans[0];

    std::string contents = ScanSerialiser::serialiseScan(*m, scan);
    ASSERT_FALSE(contents.empty());
}
