/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#include <gtest/gtest.h>

#include "manager/scheduler/ScanSerialiser.h"
#include "manager/scheduler/ScheduledScanConfiguration.h"

#include <NamedScan.capnp.h>

#include <capnp/serialize.h>

using namespace manager::scheduler;

namespace
{
    class Deserialise
    {
    public:
        explicit Deserialise(const std::string& dataAsString);

        const kj::ArrayPtr<const capnp::word> view;
        capnp::FlatArrayMessageReader messageInput;
        Sophos::ssplav::NamedScan::Reader requestReader;
    };
}

Deserialise::Deserialise(const std::string& dataAsString)
    : view(reinterpret_cast<const capnp::word*>(&(*std::begin(dataAsString))),
           reinterpret_cast<const capnp::word*>(&(*std::end(dataAsString)))),
           messageInput(view)
{
    requestReader = messageInput.getRoot<Sophos::ssplav::NamedScan>();
}

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

    Deserialise r(dataAsString);
    EXPECT_FALSE(r.requestReader.getScanArchives());
    EXPECT_EQ(r.requestReader.getName(), "Sophos Cloud Scheduled Scan");
}

TEST(ScanSerialiser, FullScan) // NOLINT
{
    auto attributeMap = Common::XmlUtilities::parseXml(
            R"MULTILINE(<?xml version="1.0"?>
<config xmlns="http://www.sophos.com/EE/EESavConfiguration">
  <csc:Comp xmlns:csc="com.sophos\msys\csc" RevID="" policyType="2"/>
  <onDemandScan>
    <extensions>
      <allFiles>true</allFiles>
      <excludeSophosDefined>
        <extension>exe</extension>
        <extension>com</extension>
      </excludeSophosDefined>
      <userDefined><extension>png</extension></userDefined>
      <noExtensions>false</noExtensions>
    </extensions>
    <posixExclusions>
      <filePathSet><filePath>Exclusion1</filePath></filePathSet>
      <excludeRemoteFiles>true</excludeRemoteFiles>
    </posixExclusions>
    <scanSet>
      <scan>
        <name>Sophos Cloud Scheduled Scan</name>
        <settings>
          <scanObjectSet>
            <CDDVDDrives>true</CDDVDDrives>
            <hardDrives>false</hardDrives>
            <networkDrives>true</networkDrives>
            <removableDrives>true</removableDrives>
          </scanObjectSet>
          <scanBehaviour>
            <archives>true</archives>
          </scanBehaviour>
        </settings>
      </scan>
    </scanSet>
  </onDemandScan>
</config>
)MULTILINE");

    auto m = std::make_unique<ScheduledScanConfiguration>(attributeMap);
    auto scans = m->scans();
    const auto& scan = scans[0];

    auto exclusionsIn = m->sophosExtensionExclusions();
    ASSERT_EQ(exclusionsIn.size(), 2);

    std::string dataAsString = ScanSerialiser::serialiseScan(*m, scan);


    Deserialise r(dataAsString);
    EXPECT_TRUE(r.requestReader.getScanArchives());
    EXPECT_TRUE(r.requestReader.getScanAllFiles());
    EXPECT_FALSE(r.requestReader.getScanFilesWithNoExtensions());

    EXPECT_EQ(r.requestReader.getName(), "Sophos Cloud Scheduled Scan");
    EXPECT_TRUE(r.requestReader.getScanAllFiles());

    auto exclusions = r.requestReader.getExcludePaths();
    ASSERT_EQ(exclusions.size(), 1);
    EXPECT_EQ(exclusions[0], "Exclusion1");

    auto extensions = r.requestReader.getSophosExtensionExclusions();
    ASSERT_EQ(extensions.size(), 2);
    EXPECT_EQ(extensions[0], "exe");
    EXPECT_EQ(extensions[1], "com");

    auto inclusions = r.requestReader.getUserDefinedExtensionInclusions();
    ASSERT_EQ(inclusions.size(), 1);
    EXPECT_EQ(inclusions[0], "png");
}
