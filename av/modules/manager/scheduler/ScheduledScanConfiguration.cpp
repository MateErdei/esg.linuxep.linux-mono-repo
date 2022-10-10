//Copyright 2020-2022, Sophos Limited.  All rights reserved.

#include "ScheduledScanConfiguration.h"

#include <algorithm>
#include <sstream>

using namespace manager::scheduler;

/*
 * https://wiki.sophos.net/display/SophosCloud/EMP%3A+policy-sav-malware
 *
 * <?xml version="1.0"?>
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
 */

static std::vector<std::string> collectList(Common::XmlUtilities::AttributesMap& savPolicy, const std::string& entityFullPath)
{
    std::vector<std::string> results;
    auto attrs = savPolicy.lookupMultiple(entityFullPath);
    results.reserve(attrs.size());
    for (const auto& attr : attrs)
    {
        results.emplace_back(attr.contents());
    }
    return results;
}

static bool collectBool(Common::XmlUtilities::AttributesMap& savPolicy, const std::string& entityFullPath)
{
    return savPolicy.lookup(entityFullPath).contents() == "true";
}

ScheduledScanConfiguration::ScheduledScanConfiguration(Common::XmlUtilities::AttributesMap& savPolicy)
    : m_allFiles(false)
{
    // We are interested in the onDemandScan section
/*
 *
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
 */
    m_exclusions = collectList(savPolicy, "config/onDemandScan/posixExclusions/filePathSet/filePath");
    m_sophosExtensionExclusions = collectList(savPolicy, "config/onDemandScan/extensions/excludeSophosDefined/extension");
    m_userDefinedExtensionInclusions = collectList(savPolicy, "config/onDemandScan/extensions/userDefined/extension");
    m_allFiles = collectBool(savPolicy, "config/onDemandScan/extensions/allFiles");
    m_scanFilesWithNoExtensions = collectBool(savPolicy, "config/onDemandScan/extensions/noExtensions");

    auto scanIds = savPolicy.entitiesThatContainPath("config/onDemandScan/scanSet/scan", false);
    m_scans.reserve(scanIds.size());
    for (const auto& id : scanIds)
    {
        m_scans.emplace_back(ScheduledScan(savPolicy, id));
    }

    m_scanNowScan = ScheduledScan("Scan Now");
}

std::string ScheduledScanConfiguration::scanSummary() const
{
    std::stringstream scanNames;
    scanNames << "Scan Now: " << m_scanNowScan.name();
    for (auto itr : m_scans)
    {
        scanNames << "\n" << itr.str();
    }
   return scanNames.str();
}

bool ScheduledScanConfiguration::isValid() const
{
    auto invalidScanConfig = std::find_if(m_scans.cbegin(), m_scans.cend(), [](ScheduledScan scheduledScan){
                                              return !scheduledScan.valid();});

    return m_scanNowScan.valid() && invalidScanConfig == m_scans.cend();
}
