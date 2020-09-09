/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gtest/gtest.h>

#include "manager/scheduler/ScheduledScanConfiguration.h"
#include "tests/common/LogInitializedTests.h"

#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>

using namespace manager::scheduler;

class TestScheduledScanConfiguration : public LogInitializedTests
{
public:
    void SetUp() override
    {
        auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
        appConfig.setData("PLUGIN_INSTALL", "/tmp/TestScheduledScanConfiguration");
    }
};

TEST_F(TestScheduledScanConfiguration, constructionWithArg) // NOLINT
{
    auto attributeMap = Common::XmlUtilities::parseXml("<xml></xml>");
    auto m = std::make_unique<ScheduledScanConfiguration>(attributeMap);
}

TEST_F(TestScheduledScanConfiguration, testWholePolicy) // NOLINT
{
    auto attributeMap = Common::XmlUtilities::parseXml(
            R"MULTILINE(<?xml version="1.0"?>
<config xmlns="http://www.sophos.com/EE/EESavConfiguration">
  <csc:Comp xmlns:csc="com.sophos\msys\csc" RevID="" policyType="2"/>
  <entity>
    <productId/>
    <product-version/>
    <entityInfo/>
  </entity>
  <onAccessScan>
    <enabled>{{onAccessEnabled}}</enabled>
    <audit>false</audit>
    <scanBehaviour>
      <level>normal</level>
      <archives>false</archives>
      <pua>true</pua>
      <suspiciousFileDetection>false</suspiciousFileDetection>
      <scanForMacViruses>false</scanForMacViruses>
      <anti-rootkits>false</anti-rootkits>
      <aggressiveEmailScan>{{aggressiveEmailScan}}</aggressiveEmailScan>
    </scanBehaviour>
    <extensions>
      <allFiles>false</allFiles>
      <excludeSophosDefined/>
      <userDefined/>
      <noExtensions>true</noExtensions>
    </extensions>
    <exclusions>
      <filePathSet>{{exclusions}}</filePathSet>
      <processPathSet>{{processExclusions}}</processPathSet>
      <excludeRemoteFiles>{{excludeRemoteFiles}}</excludeRemoteFiles>
    </exclusions>
    <macExclusions>
      <filePathSet>{{posixExclusions}}</filePathSet>
      <excludeRemoteFiles>{{excludeRemoteFiles}}</excludeRemoteFiles>
    </macExclusions>
    <linuxExclusions>
      <filePathSet>{{posixExclusions}}</filePathSet>
      <excludeRemoteFiles>{{excludeRemoteFiles}}</excludeRemoteFiles>
    </linuxExclusions>
    <actions>
      <disinfect>true</disinfect>
      <puaRemoval>true</puaRemoval>
      <fileAction>doNothing</fileAction>
      <destination/>
      <suspiciousFiles>
        <fileAction>doNothing</fileAction>
        <destination/>
      </suspiciousFiles>
    </actions>
    <onAccess>
      <fileRead>true</fileRead>
      <fileWrite>true</fileWrite>
      <fileRename>true</fileRename>
      <accessInfectedRMBootSectors>false</accessInfectedRMBootSectors>
      <useNetworkChecksums>false</useNetworkChecksums>
    </onAccess>
  </onAccessScan>
  <onDemandScan>
    <extensions>
      <allFiles>false</allFiles>
      <excludeSophosDefined/>
      <userDefined/>
      <noExtensions>true</noExtensions>
    </extensions>
    <exclusions>
      <filePathSet>{{exclusions}}</filePathSet>
      <excludeRemoteFiles>{{excludeRemoteFiles}}</excludeRemoteFiles>
    </exclusions>
    <posixExclusions>
      <filePathSet>{{posixExclusions}}</filePathSet>
      <excludeRemoteFiles>{{excludeRemoteFiles}}</excludeRemoteFiles>
    </posixExclusions>
    <macExclusions>
      <filePathSet>{{posixExclusions}}</filePathSet>
      <excludeRemoteFiles>{{excludeRemoteFiles}}</excludeRemoteFiles>
    </macExclusions>
    <scanSet>
      <!-- if {{scheduledScanEnabled}} -->
      <scan>
        <name>Sophos Cloud Scheduled Scan</name>
        <schedule>
          <daySet>
            <!-- for day in {{scheduledScanDays}} -->
            <day>{{day}}</day>
          </daySet>
          <timeSet>
            <time>{{scheduledScanTime}}</time>
          </timeSet>
        </schedule>
        <settings>
          <scanObjectSet>
            <CDDVDDrives>false</CDDVDDrives>
            <hardDrives>true</hardDrives>
            <networkDrives>false</networkDrives>
            <removableDrives>false</removableDrives>
            <kernelMemory>true</kernelMemory>
          </scanObjectSet>
          <scanBehaviour>
            <level>normal</level>
            <archives>{{scheduledScanArchives}}</archives>
            <pua>true</pua>
            <suspiciousFileDetection>false</suspiciousFileDetection>
            <scanForMacViruses>false</scanForMacViruses>
            <anti-rootkits>true</anti-rootkits>
          </scanBehaviour>
          <actions>
            <disinfect>true</disinfect>
            <puaRemoval>false</puaRemoval>
            <fileAction>doNothing</fileAction>
            <destination/>
            <suspiciousFiles>
              <fileAction>doNothing</fileAction>
              <destination/>
            </suspiciousFiles>
          </actions>
          <on-demand-options>
            <minimise-scan-impact>true</minimise-scan-impact>
          </on-demand-options>
        </settings>
      </scan>
    </scanSet>
    <fileReputation>{{fileReputationCollectionDuringOnDemandScan}}</fileReputation>
  </onDemandScan>
  <autoExclusions>{{autoExclusions}}</autoExclusions>
  <appDetection>{{appDetection}}</appDetection>
  <approvedList>{{puaExclusions}}</approvedList>
  <alerting>
    <desktop>
      <report>
        <virus>true</virus>
        <scanErrors>true</scanErrors>
        <SAVErrors>true</SAVErrors>
        <pua>true</pua>
        <suspiciousBehaviour>false</suspiciousBehaviour>
        <suspiciousFiles>false</suspiciousFiles>
      </report>
      <message/>
    </desktop>
    <smtp>
      <smtpConfig>
        <server/>
        <sender/>
        <replyTo/>
        <language>english</language>
      </smtpConfig>
      <report>
        <virus>false</virus>
        <scanErrors>false</scanErrors>
        <SAVErrors>false</SAVErrors>
        <pua>false</pua>
        <suspiciousBehaviour>false</suspiciousBehaviour>
        <suspiciousFiles>false</suspiciousFiles>
      </report>
      <subjectLine/>
      <recipientSet/>
    </smtp>
    <snmp>
      <snmpConfig>
        <trapDestination/>
        <communityName/>
        <filterCtrlChars>false</filterCtrlChars>
      </snmpConfig>
      <report>
        <virus>false</virus>
        <scanErrors>false</scanErrors>
        <SAVErrors>false</SAVErrors>
        <pua>false</pua>
        <suspiciousBehaviour>false</suspiciousBehaviour>
        <suspiciousFiles>false</suspiciousFiles>
      </report>
    </snmp>
    <eventLog>
      <enabled>true</enabled>
      <report>
        <virus>true</virus>
        <scanErrors>false</scanErrors>
        <SAVErrors>false</SAVErrors>
        <pua>true</pua>
        <suspiciousBehaviour>true</suspiciousBehaviour>
        <suspiciousFiles>true</suspiciousFiles>
      </report>
    </eventLog>
  </alerting>
  <runtimeBehaviourInspection>
    <alertOnly>true</alertOnly>
    <resourceShielding>false</resourceShielding>
    <bufferOverrunProtection>false</bufferOverrunProtection>
  </runtimeBehaviourInspection>
  <runtimeBehaviourInspection2>
    <enabled>true</enabled>
    <audit>false</audit>
    <malicious>
      <enabled>true</enabled>
    </malicious>
    <suspicious>
      <enabled>false</enabled>
      <alertOnly>true</alertOnly>
      <skipInstallers>{{skipInstallers}}</skipInstallers>
    </suspicious>
    <bops>
      <enabled>false</enabled>
      <alertOnly>true</alertOnly>
    </bops>
    <!-- Optional: used by OPM only -->
    <mtd>
      <enabled>true</enabled>
    </mtd>
  </runtimeBehaviourInspection2>
  <fileReputation>
    <enabled>{{fileReputationEnabled}}</enabled>
    <level>{{fileReputationLevel}}</level>
    <action>{{fileReputationAction}}</action>
  </fileReputation>
  <SIPSApprovedList/>
  <webScanning>
    <mode>{{webScanningMode}}</mode>
  </webScanning>
  <webFiltering>
    <enabled>{{webFilteringMode}}</enabled>
    <approvedSites repeat="for site in approvedWebsites">
      <sitePattern>{{site}}</sitePattern>
    </approvedSites>
  </webFiltering>
  <detectionFeedback>
    <sendData>true</sendData>
    <sendFiles>false</sendFiles>
  </detectionFeedback>
  <continuousScan>
    <kernelMemoryScan>
      <enabled>true</enabled>
    </kernelMemoryScan>
  </continuousScan>
 <!-- Optional: not used by OPM -->
  <quarantineManager reportInStatus="true"/>
</config>
)MULTILINE");

    auto attributes = attributeMap.lookup("config/continuousScan/kernelMemoryScan/enabled");
    EXPECT_EQ(attributes.value(attributes.TextId), "true");

    auto m = std::make_unique<ScheduledScanConfiguration>(attributeMap);
}

TEST_F(TestScheduledScanConfiguration, justOndemandPolicy) // NOLINT
{
    auto attributeMap = Common::XmlUtilities::parseXml(
            R"MULTILINE(<?xml version="1.0"?>
<config xmlns="http://www.sophos.com/EE/EESavConfiguration">
  <csc:Comp xmlns:csc="com.sophos\msys\csc" RevID="" policyType="2"/>
  <onDemandScan>
    <extensions>
      <allFiles>false</allFiles>
      <excludeSophosDefined/>
      <userDefined/>
      <noExtensions>true</noExtensions>
    </extensions>
    <exclusions>
      <filePathSet>{{exclusions}}</filePathSet>
      <excludeRemoteFiles>{{excludeRemoteFiles}}</excludeRemoteFiles>
    </exclusions>
    <posixExclusions>
      <filePathSet><filePath>Exclusion1</filePath></filePathSet>
      <excludeRemoteFiles>{{excludeRemoteFiles}}</excludeRemoteFiles>
    </posixExclusions>
    <macExclusions>
      <filePathSet>{{posixExclusions}}</filePathSet>
      <excludeRemoteFiles>{{excludeRemoteFiles}}</excludeRemoteFiles>
    </macExclusions>
    <scanSet>
      <!-- if {{scheduledScanEnabled}} -->
      <scan>
        <name>Sophos Cloud Scheduled Scan</name>
        <schedule>
          <daySet>
            <!-- for day in {{scheduledScanDays}} -->
            <day>{{day}}</day>
          </daySet>
          <timeSet>
            <time>{{scheduledScanTime}}</time>
          </timeSet>
        </schedule>
        <settings>
          <scanObjectSet>
            <CDDVDDrives>false</CDDVDDrives>
            <hardDrives>true</hardDrives>
            <networkDrives>false</networkDrives>
            <removableDrives>false</removableDrives>
            <kernelMemory>true</kernelMemory>
          </scanObjectSet>
          <scanBehaviour>
            <level>normal</level>
            <archives>{{scheduledScanArchives}}</archives>
            <pua>true</pua>
            <suspiciousFileDetection>false</suspiciousFileDetection>
            <scanForMacViruses>false</scanForMacViruses>
            <anti-rootkits>true</anti-rootkits>
          </scanBehaviour>
          <actions>
            <disinfect>true</disinfect>
            <puaRemoval>false</puaRemoval>
            <fileAction>doNothing</fileAction>
            <destination/>
            <suspiciousFiles>
              <fileAction>doNothing</fileAction>
              <destination/>
            </suspiciousFiles>
          </actions>
          <on-demand-options>
            <minimise-scan-impact>true</minimise-scan-impact>
          </on-demand-options>
        </settings>
      </scan>
    </scanSet>
    <fileReputation>{{fileReputationCollectionDuringOnDemandScan}}</fileReputation>
  </onDemandScan>
</config>
)MULTILINE");

    auto m = std::make_unique<ScheduledScanConfiguration>(attributeMap);
    auto exclusions = m->exclusions();
    ASSERT_EQ(exclusions.size(), 1);
    EXPECT_EQ(exclusions.at(0),"Exclusion1");
}

TEST_F(TestScheduledScanConfiguration, multipleExclusions) // NOLINT
{
    auto attributeMap = Common::XmlUtilities::parseXml(
            R"MULTILINE(<?xml version="1.0"?>
<config xmlns="http://www.sophos.com/EE/EESavConfiguration">
  <csc:Comp xmlns:csc="com.sophos\msys\csc" RevID="" policyType="2"/>
  <onDemandScan>
    <posixExclusions>
      <filePathSet>
        <filePath>Exclusion1</filePath>
        <filePath>Exclusion2</filePath>
      </filePathSet>
    </posixExclusions>
  </onDemandScan>
</config>
)MULTILINE");

    auto m = std::make_unique<ScheduledScanConfiguration>(attributeMap);
    auto exclusions = m->exclusions();
    EXPECT_EQ(exclusions.size(), 2);
    EXPECT_EQ(exclusions.at(0),"Exclusion1");
    EXPECT_EQ(exclusions.at(1),"Exclusion2");
}

TEST_F(TestScheduledScanConfiguration, TestEmptyXml) // NOLINT
{
    auto attributeMap = Common::XmlUtilities::parseXml(
            R"MULTILINE(<?xml version="1.0"?>
<config xmlns="http://www.sophos.com/EE/EESavConfiguration">
</config>
)MULTILINE");

    auto m = std::make_unique<ScheduledScanConfiguration>(attributeMap);
    auto scans = m->scans();
    EXPECT_EQ(scans.size(), 0);
    auto exclusions = m->exclusions();
    EXPECT_EQ(exclusions.size(), 0);

    EXPECT_FALSE(m->scanAllFileExtensions());
    EXPECT_FALSE(m->scanFilesWithNoExtensions());
}

TEST_F(TestScheduledScanConfiguration, TestSimpleParsing) //NOLINT
{
    auto attributeMap = Common::XmlUtilities::parseXml("<xml><key>value</key></xml>");
    auto attributes = attributeMap.lookup("xml/key");
    EXPECT_EQ(attributes.value(attributes.TextId), "value");
}

TEST_F(TestScheduledScanConfiguration, TestTextIdAttribute) //NOLINT
{
    auto attributeMap = Common::XmlUtilities::parseXml("<xml><key TestId=\"foo\">bar</key></xml>");
    auto attributes = attributeMap.lookup("xml/key");
    EXPECT_EQ(attributes.value(attributes.TextId), "bar"); // Rather than foo
}

TEST_F(TestScheduledScanConfiguration, ScanNowConstructed) //NOLINT
{
    auto attributeMap = Common::XmlUtilities::parseXml("<xml><key TestId=\"foo\">bar</key></xml>");
    auto scanConfiguration = ScheduledScanConfiguration(attributeMap);

    // Check scanNowScan is constructed correctly
    auto scanNowScan = scanConfiguration.scanNowScan();

    EXPECT_TRUE(scanNowScan.valid());
    EXPECT_EQ(scanNowScan.name(), "Scan Now");
    EXPECT_EQ(scanNowScan.calculateNextTime(::time(nullptr)), static_cast<time_t>(-1));

    const auto& scanNowTimes = scanNowScan.times();
    EXPECT_EQ(scanNowTimes.size(), 0);

    const auto& scanNowDays = scanNowScan.days();
    EXPECT_EQ(scanNowDays.size(), 0);

    EXPECT_EQ(scanNowScan.isScanNow(), true);
}

TEST_F(TestScheduledScanConfiguration, MultipleScans) // NOLINT
{

    auto attributeMap = Common::XmlUtilities::parseXml(
            R"MULTILINE(<?xml version="1.0"?>
<config xmlns="http://www.sophos.com/EE/EESavConfiguration">
  <csc:Comp xmlns:csc="com.sophos\msys\csc" RevID="" policyType="2"/>
  <onDemandScan>
    <scanSet>
      <!-- if {{scheduledScanEnabled}} -->
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
        <settings>
          <scanObjectSet>
            <CDDVDDrives>false</CDDVDDrives>
            <hardDrives>true</hardDrives>
            <networkDrives>false</networkDrives>
            <removableDrives>false</removableDrives>
            <kernelMemory>true</kernelMemory>
          </scanObjectSet>
          <scanBehaviour>
            <level>normal</level>
            <archives>{{scheduledScanArchives}}</archives>
            <pua>true</pua>
            <suspiciousFileDetection>false</suspiciousFileDetection>
            <scanForMacViruses>false</scanForMacViruses>
            <anti-rootkits>true</anti-rootkits>
          </scanBehaviour>
          <actions>
            <disinfect>true</disinfect>
            <puaRemoval>false</puaRemoval>
            <fileAction>doNothing</fileAction>
            <destination/>
            <suspiciousFiles>
              <fileAction>doNothing</fileAction>
              <destination/>
            </suspiciousFiles>
          </actions>
          <on-demand-options>
            <minimise-scan-impact>true</minimise-scan-impact>
          </on-demand-options>
        </settings>
      </scan>
      <scan>
        <name>Another scan!</name>
        <schedule>
          <daySet>
            <!-- for day in {{scheduledScanDays}} -->
            <day>friday</day>
          </daySet>
          <timeSet>
            <time>13:00:00</time>
            <time>17:00:00</time>
          </timeSet>
        </schedule>
        <settings>
          <scanObjectSet>
            <CDDVDDrives>false</CDDVDDrives>
            <hardDrives>true</hardDrives>
            <networkDrives>false</networkDrives>
            <removableDrives>false</removableDrives>
            <kernelMemory>true</kernelMemory>
          </scanObjectSet>
          <scanBehaviour>
            <level>normal</level>
            <archives>{{scheduledScanArchives}}</archives>
            <pua>true</pua>
            <suspiciousFileDetection>false</suspiciousFileDetection>
            <scanForMacViruses>false</scanForMacViruses>
            <anti-rootkits>true</anti-rootkits>
          </scanBehaviour>
          <actions>
            <disinfect>true</disinfect>
            <puaRemoval>false</puaRemoval>
            <fileAction>doNothing</fileAction>
            <destination/>
            <suspiciousFiles>
              <fileAction>doNothing</fileAction>
              <destination/>
            </suspiciousFiles>
          </actions>
          <on-demand-options>
            <minimise-scan-impact>true</minimise-scan-impact>
          </on-demand-options>
        </settings>
      </scan>
    </scanSet>
    <fileReputation>{{fileReputationCollectionDuringOnDemandScan}}</fileReputation>
  </onDemandScan>
</config>
)MULTILINE");

    auto attributes = attributeMap.lookupMultiple("config/onDemandScan/scanSet/scan");
    EXPECT_EQ(attributes.size(), 2);

    auto scanIds = attributeMap.entitiesThatContainPath("config/onDemandScan/scanSet/scan", false);
    ASSERT_EQ(scanIds.size(), 2);
//    ASSERT_EQ(scanIds[0], "config/onDemandScan/scanSet/scan");
//    ASSERT_EQ(scanIds[1], "config/onDemandScan/scanSet/scan_0");

    // Check scan name 1 is "Sophos Cloud Scheduled Scan"
    auto attr = attributeMap.lookup(scanIds[0] + "/name");
    EXPECT_EQ(attr.contents(), "Sophos Cloud Scheduled Scan");

    attr = attributeMap.lookup(scanIds[1] + "/name");
    EXPECT_EQ(attr.contents(), "Another scan!");

    auto m = std::make_unique<ScheduledScanConfiguration>(attributeMap);
    auto scans = m->scans();
    ASSERT_EQ(scans.size(), 2);
    EXPECT_EQ(scans[0].name(), "Sophos Cloud Scheduled Scan");
    EXPECT_EQ(scans[1].name(), "Another scan!");

    EXPECT_EQ(scans[0].isScanNow(), false);
    EXPECT_EQ(scans[1].isScanNow(), false);

    const auto& days = scans[0].days();
    ASSERT_EQ(days.size(), 2);
    // Days are sorted while being processed
    EXPECT_EQ(days.days()[0], THURSDAY);
    EXPECT_EQ(days.days()[1], SATURDAY);

    const auto& times = scans[1].times();
    ASSERT_EQ(times.size(), 2);
    EXPECT_EQ(times.times()[0].hour(), 13);
    EXPECT_EQ(times.times()[0].minute(), 0);
    EXPECT_EQ(times.times()[1].hour(), 17);
    EXPECT_EQ(times.times()[1].minute(), 0);

    // Check scanNowScan is constructed correctly
    auto scanNowScan = m->scanNowScan();

    EXPECT_TRUE(scanNowScan.valid());
    EXPECT_EQ(scanNowScan.name(), "Scan Now");
    EXPECT_EQ(scanNowScan.calculateNextTime(::time(nullptr)), static_cast<time_t>(-1));

    const auto& scanNowTimes = scanNowScan.times();
    EXPECT_EQ(scanNowTimes.size(), 0);

    const auto& scanNowDays = scanNowScan.days();
    EXPECT_EQ(scanNowDays.size(), 0);

    EXPECT_EQ(scanNowScan.isScanNow(), true);
}

TEST_F(TestScheduledScanConfiguration, TestArchiveSettingTrue) // NOLINT
{
    auto attributeMap = Common::XmlUtilities::parseXml(
            R"MULTILINE(<?xml version="1.0"?>
<config xmlns="http://www.sophos.com/EE/EESavConfiguration">
  <csc:Comp xmlns:csc="com.sophos\msys\csc" RevID="" policyType="2"/>
  <onDemandScan>
    <scanSet>
      <scan>
        <name>Sophos Cloud Scheduled Scan</name>
        <settings>
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
    ASSERT_EQ(scans.size(), 1);
    const auto& scan = scans[0];
    auto scanArchives = scan.archiveScanning();
    EXPECT_TRUE(scanArchives);
}

TEST_F(TestScheduledScanConfiguration, TestArchiveSettingFalse) // NOLINT
{
    auto attributeMap = Common::XmlUtilities::parseXml(
            R"MULTILINE(<?xml version="1.0"?>
<config xmlns="http://www.sophos.com/EE/EESavConfiguration">
  <csc:Comp xmlns:csc="com.sophos\msys\csc" RevID="" policyType="2"/>
  <onDemandScan>
    <scanSet>
      <scan>
        <name>Sophos Cloud Scheduled Scan</name>
        <settings>
          <scanBehaviour>
            <archives>false</archives>
          </scanBehaviour>
        </settings>
      </scan>
    </scanSet>
  </onDemandScan>
</config>
)MULTILINE");

    auto m = std::make_unique<ScheduledScanConfiguration>(attributeMap);
    auto scans = m->scans();
    ASSERT_EQ(scans.size(), 1);
    const auto& scan = scans[0];
    auto scanArchives = scan.archiveScanning();
    EXPECT_FALSE(scanArchives);
}

TEST_F(TestScheduledScanConfiguration, MissingArchiveSettings) // NOLINT
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
    auto scanArchives = scan.archiveScanning();
    EXPECT_FALSE(scanArchives);
}

TEST_F(TestScheduledScanConfiguration, allFilesFalse) // NOLINT
{
    auto attributeMap = Common::XmlUtilities::parseXml(
            R"MULTILINE(<?xml version="1.0"?>
<config xmlns="http://www.sophos.com/EE/EESavConfiguration">
  <csc:Comp xmlns:csc="com.sophos\msys\csc" RevID="" policyType="2"/>
  <onDemandScan>
    <extensions>
      <allFiles>false</allFiles>
      <excludeSophosDefined/>
      <userDefined/>
      <noExtensions>true</noExtensions>
    </extensions>
  </onDemandScan>
</config>
)MULTILINE");

    auto m = std::make_unique<ScheduledScanConfiguration>(attributeMap);
    auto allFiles = m->scanAllFileExtensions();
    EXPECT_FALSE(allFiles);
}

TEST_F(TestScheduledScanConfiguration, allFilesTrue) // NOLINT
{
    auto attributeMap = Common::XmlUtilities::parseXml(
            R"MULTILINE(<?xml version="1.0"?>
<config xmlns="http://www.sophos.com/EE/EESavConfiguration">
  <csc:Comp xmlns:csc="com.sophos\msys\csc" RevID="" policyType="2"/>
  <onDemandScan>
    <extensions>
      <allFiles>true</allFiles>
    </extensions>
  </onDemandScan>
</config>
)MULTILINE");

    auto m = std::make_unique<ScheduledScanConfiguration>(attributeMap);
    auto allFiles = m->scanAllFileExtensions();
    EXPECT_TRUE(allFiles);
}

TEST_F(TestScheduledScanConfiguration, noExtensionsFalse) // NOLINT
{
    auto attributeMap = Common::XmlUtilities::parseXml(
            R"MULTILINE(<?xml version="1.0"?>
<config xmlns="http://www.sophos.com/EE/EESavConfiguration">
  <csc:Comp xmlns:csc="com.sophos\msys\csc" RevID="" policyType="2"/>
  <onDemandScan>
    <extensions>
      <noExtensions>false</noExtensions>
    </extensions>
  </onDemandScan>
</config>
)MULTILINE");

    auto m = std::make_unique<ScheduledScanConfiguration>(attributeMap);
    EXPECT_FALSE(m->scanFilesWithNoExtensions());
}

TEST_F(TestScheduledScanConfiguration, noExtensionsTrue) // NOLINT
{
    auto attributeMap = Common::XmlUtilities::parseXml(
            R"MULTILINE(<?xml version="1.0"?>
<config xmlns="http://www.sophos.com/EE/EESavConfiguration">
  <csc:Comp xmlns:csc="com.sophos\msys\csc" RevID="" policyType="2"/>
  <onDemandScan>
    <extensions>
      <noExtensions>true</noExtensions>
    </extensions>
  </onDemandScan>
</config>
)MULTILINE");

    auto m = std::make_unique<ScheduledScanConfiguration>(attributeMap);
    EXPECT_TRUE(m->scanFilesWithNoExtensions());
}

//"onDemandScan/extensions/excludeSophosDefined/extension"


TEST_F(TestScheduledScanConfiguration, extensionExclusion) // NOLINT
{
    auto attributeMap = Common::XmlUtilities::parseXml(
            R"MULTILINE(<?xml version="1.0"?>
<config xmlns="http://www.sophos.com/EE/EESavConfiguration">
  <csc:Comp xmlns:csc="com.sophos\msys\csc" RevID="" policyType="2"/>
  <onDemandScan>
    <extensions>
      <excludeSophosDefined><extension>exe</extension></excludeSophosDefined>
    </extensions>
  </onDemandScan>
</config>
)MULTILINE");

    auto m = std::make_unique<ScheduledScanConfiguration>(attributeMap);
    auto e = m->sophosExtensionExclusions();
    ASSERT_EQ(e.size(), 1);
    EXPECT_EQ(e.at(0), "exe");
}

TEST_F(TestScheduledScanConfiguration, multipleExtensionExclusion) // NOLINT
{
    auto attributeMap = Common::XmlUtilities::parseXml(
            R"MULTILINE(<?xml version="1.0"?>
<config xmlns="http://www.sophos.com/EE/EESavConfiguration">
  <csc:Comp xmlns:csc="com.sophos\msys\csc" RevID="" policyType="2"/>
  <onDemandScan>
    <extensions>
      <excludeSophosDefined>
        <extension>exe</extension>
        <extension>jpg</extension>
      </excludeSophosDefined>
    </extensions>
  </onDemandScan>
</config>
)MULTILINE");

    auto m = std::make_unique<ScheduledScanConfiguration>(attributeMap);
    auto e = m->sophosExtensionExclusions();
    ASSERT_EQ(e.size(), 2);
    EXPECT_EQ(e.at(0), "exe");
    EXPECT_EQ(e.at(1), "jpg");
}

TEST_F(TestScheduledScanConfiguration, extensionInclusion) // NOLINT
{
    auto attributeMap = Common::XmlUtilities::parseXml(
            R"MULTILINE(<?xml version="1.0"?>
<config xmlns="http://www.sophos.com/EE/EESavConfiguration">
  <csc:Comp xmlns:csc="com.sophos\msys\csc" RevID="" policyType="2"/>
  <onDemandScan>
    <extensions>
      <userDefined><extension>exe</extension></userDefined>
    </extensions>
  </onDemandScan>
</config>
)MULTILINE");

    auto m = std::make_unique<ScheduledScanConfiguration>(attributeMap);
    auto e = m->userDefinedExtensionInclusions();
    ASSERT_EQ(e.size(), 1);
    EXPECT_EQ(e.at(0), "exe");
}

TEST_F(TestScheduledScanConfiguration, scanLocalDisks) // NOLINT
{
    auto attributeMap = Common::XmlUtilities::parseXml(
            R"MULTILINE(<?xml version="1.0"?>
<config xmlns="http://www.sophos.com/EE/EESavConfiguration">
  <csc:Comp xmlns:csc="com.sophos\msys\csc" RevID="" policyType="2"/>
  <onDemandScan>
    <scanSet>
      <scan>
        <name>Sophos Cloud Scheduled Scan</name>
        <settings>
          <scanObjectSet>
            <CDDVDDrives>false</CDDVDDrives>
            <hardDrives>true</hardDrives>
            <networkDrives>false</networkDrives>
            <removableDrives>false</removableDrives>
          </scanObjectSet>
        </settings>
      </scan>
    </scanSet>
  </onDemandScan>
</config>
)MULTILINE");

    auto m = std::make_unique<ScheduledScanConfiguration>(attributeMap);
    auto scans = m->scans();
    ASSERT_EQ(scans.size(), 1);
    const auto& scan = scans.at(0);

    EXPECT_TRUE(scan.hardDrives());
    EXPECT_FALSE(scan.CDDVDDrives());
    EXPECT_FALSE(scan.networkDrives());
    EXPECT_FALSE(scan.removableDrives());
}

TEST_F(TestScheduledScanConfiguration, scanEverythingButLocalDisks) // NOLINT
{
    auto attributeMap = Common::XmlUtilities::parseXml(
            R"MULTILINE(<?xml version="1.0"?>
<config xmlns="http://www.sophos.com/EE/EESavConfiguration">
  <csc:Comp xmlns:csc="com.sophos\msys\csc" RevID="" policyType="2"/>
  <onDemandScan>
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
        </settings>
      </scan>
    </scanSet>
  </onDemandScan>
</config>
)MULTILINE");

    auto m = std::make_unique<ScheduledScanConfiguration>(attributeMap);
    auto scans = m->scans();
    ASSERT_EQ(scans.size(), 1);
    const auto& scan = scans.at(0);

    EXPECT_FALSE(scan.hardDrives());
    EXPECT_TRUE(scan.CDDVDDrives());
    EXPECT_TRUE(scan.networkDrives());
    EXPECT_TRUE(scan.removableDrives());
}

TEST_F(TestScheduledScanConfiguration, scanNetworkOnly) // NOLINT
{
    auto attributeMap = Common::XmlUtilities::parseXml(
            R"MULTILINE(<?xml version="1.0"?>
<config xmlns="http://www.sophos.com/EE/EESavConfiguration">
  <csc:Comp xmlns:csc="com.sophos\msys\csc" RevID="" policyType="2"/>
  <onDemandScan>
    <scanSet>
      <scan>
        <name>Sophos Cloud Scheduled Scan</name>
        <settings>
          <scanObjectSet>
            <CDDVDDrives>false</CDDVDDrives>
            <hardDrives>false</hardDrives>
            <networkDrives>true</networkDrives>
            <removableDrives>false</removableDrives>
          </scanObjectSet>
        </settings>
      </scan>
    </scanSet>
  </onDemandScan>
</config>
)MULTILINE");

    auto m = std::make_unique<ScheduledScanConfiguration>(attributeMap);
    auto scans = m->scans();
    ASSERT_EQ(scans.size(), 1);
    const auto& scan = scans.at(0);

    EXPECT_FALSE(scan.hardDrives());
    EXPECT_FALSE(scan.CDDVDDrives());
    EXPECT_TRUE(scan.networkDrives());
    EXPECT_FALSE(scan.removableDrives());
}

TEST_F(TestScheduledScanConfiguration, scanRemovableOnly) // NOLINT
{
    auto attributeMap = Common::XmlUtilities::parseXml(
            R"MULTILINE(<?xml version="1.0"?>
<config xmlns="http://www.sophos.com/EE/EESavConfiguration">
  <csc:Comp xmlns:csc="com.sophos\msys\csc" RevID="" policyType="2"/>
  <onDemandScan>
    <scanSet>
      <scan>
        <name>Sophos Cloud Scheduled Scan</name>
        <settings>
          <scanObjectSet>
            <CDDVDDrives>false</CDDVDDrives>
            <hardDrives>false</hardDrives>
            <networkDrives>false</networkDrives>
            <removableDrives>true</removableDrives>
          </scanObjectSet>
        </settings>
      </scan>
    </scanSet>
  </onDemandScan>
</config>
)MULTILINE");

    auto m = std::make_unique<ScheduledScanConfiguration>(attributeMap);
    auto scans = m->scans();
    ASSERT_EQ(scans.size(), 1);
    const auto& scan = scans.at(0);

    EXPECT_FALSE(scan.hardDrives());
    EXPECT_FALSE(scan.CDDVDDrives());
    EXPECT_FALSE(scan.networkDrives());
    EXPECT_TRUE(scan.removableDrives());
}
