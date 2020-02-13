/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#include <gtest/gtest.h>

#include "manager/scheduler/ScanSerialiser.h"
#include "manager/scheduler/ScheduledScanConfiguration.h"

#include <NamedScan.capnp.h>

#include <capnp/serialize.h>

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
    ASSERT_FALSE(scan.archiveScanning());

    std::string dataAsString = ScanSerialiser::serialiseScan(*m, scan);
    ASSERT_FALSE(dataAsString.empty());


    const kj::ArrayPtr<const capnp::word> view(
            reinterpret_cast<const capnp::word*>(&(*std::begin(dataAsString))),
            reinterpret_cast<const capnp::word*>(&(*std::end(dataAsString))));

    capnp::FlatArrayMessageReader messageInput(view);
    Sophos::ssplav::NamedScan::Reader requestReader =
            messageInput.getRoot<Sophos::ssplav::NamedScan>();

    EXPECT_FALSE(requestReader.getScanArchives());
    EXPECT_EQ(requestReader.getName(), "Sophos Cloud Scheduled Scan");
}
