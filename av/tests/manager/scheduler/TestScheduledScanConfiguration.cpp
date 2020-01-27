/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <gtest/gtest.h>

#include "manager/scheduler/ScheduledScanConfiguration.h"

using namespace manager::scheduler;


TEST(ScheduledScanConfiguration, constructionWithArg) // NOLINT
{
    auto attributeMap = Common::XmlUtilities::parseXml("<xml></xml>");
    auto m = std::make_unique<ScheduledScanConfiguration>(attributeMap);
}

TEST(ScheduledScanConfiguration, testWholePolicy) // NOLINT
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

TEST(ScheduledScanConfiguration, justOndemandPolicy) // NOLINT
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

TEST(ScheduledScanConfiguration, multipleExclusions) // NOLINT
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

TEST(ScheduledScanConfiguration, TestSimpleParsing) //NOLINT
{
    auto attributeMap = Common::XmlUtilities::parseXml("<xml><key>value</key></xml>");
    auto attributes = attributeMap.lookup("xml/key");
    EXPECT_EQ(attributes.value(attributes.TextId), "value");
}

TEST(ScheduledScanConfiguration, TestTextIdAttribute) //NOLINT
{
    auto attributeMap = Common::XmlUtilities::parseXml("<xml><key TestId=\"foo\">bar</key></xml>");
    auto attributes = attributeMap.lookup("xml/key");
    EXPECT_EQ(attributes.value(attributes.TextId), "bar"); // Rather than foo
}

TEST(ScheduledScanConfiguration, DaySet) // NOLINT
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
    auto days = scan.days();
    ASSERT_EQ(days.size(), 2);

}

TEST(ScheduledScanConfiguration, MultipleScans) // NOLINT
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
}