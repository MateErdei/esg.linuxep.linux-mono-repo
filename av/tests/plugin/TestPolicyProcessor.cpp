// Copyright 2020-2022, Sophos Limited.  All rights reserved.

#include "TestPolicyProcessor.h"

#include "PluginMemoryAppenderUsingTests.h"

#include "datatypes/sophos_filesystem.h"

#include <Common/Helpers/MockFileSystem.h>
#include <Common/Helpers/FileSystemReplaceAndRestore.h>
#include <Common/FileSystem/IFileSystemException.h>
#include <Common/TelemetryHelperImpl/TelemetryHelper.h>

#include <thirdparty/nlohmann-json/json.hpp>

#include <gtest/gtest.h>

namespace fs = sophos_filesystem;

namespace
{
    class TestPolicyProcessor : public PluginMemoryAppenderUsingTests
    {
    protected:
        fs::path m_testDir;
        void SetUp() override
        {
            const ::testing::TestInfo* const test_info = ::testing::UnitTest::GetInstance()->current_test_info();
            m_testDir = fs::temp_directory_path();
            m_testDir /= test_info->test_case_name();
            m_testDir /= test_info->name();
            fs::remove_all(m_testDir);
            fs::create_directories(m_testDir);

            auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
            appConfig.setData(Common::ApplicationConfiguration::SOPHOS_INSTALL, m_testDir );
            appConfig.setData("PLUGIN_INSTALL", m_testDir );
            
            m_susiStartupConfigPath = m_testDir / "var/susi_startup_settings.json";
            m_susiStartupConfigChrootPath = std::string(m_testDir / "chroot") + m_susiStartupConfigPath;
            m_soapConfigPath = m_testDir / "var/soapd_config.json";
            m_soapFlagConfigPath = m_testDir / "var/oa_flag.json";
            m_customerIdPath = m_testDir / "var/customer_id.txt";
            m_mockIFileSystemPtr = std::make_unique<StrictMock<MockFileSystem>>();
        }

        void TearDown() override
        {
            fs::remove_all(m_testDir);
        }

        std::string m_susiStartupConfigPath;
        std::string m_susiStartupConfigChrootPath;
        std::string m_soapConfigPath;
        std::string m_soapFlagConfigPath;
        std::string m_customerIdPath;
        std::unique_ptr<StrictMock<MockFileSystem>> m_mockIFileSystemPtr;
    };

}

static const std::string ALC_FULL_POLICY //NOLINT
    {
    R"sophos(<?xml version="1.0"?>
<AUConfigurations xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:csc="com.sophos\msys\csc" xmlns="http://www.sophos.com/EE/AUConfig">
  <csc:Comp RevID="4d77291c5f5be04f62f04b6f379cf5d80e62c9a13d149c1acce06aba819b83cc" policyType="1"/>
  <AUConfig platform="Linux">
    <sophos_address address="http://es-web.sophos.com/update"/>
    <primary_location>
      <server BandwidthLimit="256" AutoDial="false" Algorithm="AES256" UserPassword="CCD8CFFX8bdCDFtU0+hv6MvL3FoxA0YeSNjJyZJWxz1b3uTsBu5p8GJqsfp3SAByOZw=" UserName="CSP201127101443" UseSophos="true" UseHttps="true" UseDelta="true" AllowLocalConfig="false"/>
      <proxy ProxyType="0" ProxyUserPassword="" ProxyUserName="" ProxyPortNumber="0" ProxyAddress="" AllowLocalConfig="false"/>
    </primary_location>
    <secondary_location>
      <server BandwidthLimit="256" AutoDial="false" Algorithm="" UserPassword="" UserName="" UseSophos="false" UseHttps="true" UseDelta="true" ConnectionAddress="" AllowLocalConfig="false"/>
      <proxy ProxyType="0" ProxyUserPassword="" ProxyUserName="" ProxyPortNumber="0" ProxyAddress="" AllowLocalConfig="false"/>
    </secondary_location>
    <schedule AllowLocalConfig="false" SchedEnable="true" Frequency="60" DetectDialUp="false"/>
    <logging AllowLocalConfig="false" LogLevel="50" LogEnable="true" MaxLogFileSize="1"/>
    <bootstrap Location="" UsePrimaryServerAddress="true"/>
    <cloud_subscription RigidName="ServerProtectionLinux-Base" Tag="BETA"/>
    <cloud_subscriptions>
      <subscription Id="Base" RigidName="ServerProtectionLinux-Base" Tag="BETA"/>
      <subscription Id="CloudAV" RigidName="ServerProtectionLinux-Plugin-AV" Tag="RECOMMENDED"/>
      <subscription Id="LiveQuery" RigidName="ServerProtectionLinux-Plugin-EDR" Tag="BETA"/>
      <subscription Id="MDR" RigidName="ServerProtectionLinux-Plugin-MDR" Tag="BETA"/>
    </cloud_subscriptions>
    <delay_supplements enabled="true"/>
  </AUConfig>
  <Features>
    <Feature id="APPCNTRL"/>
    <Feature id="AV"/>
    <Feature id="CORE"/>
    <Feature id="DLP"/>
    <Feature id="DVCCNTRL"/>
    <Feature id="EFW"/>
    <Feature id="HBT"/>
    <Feature id="LIVEQUERY"/>
    <Feature id="LIVETERMINAL"/>
    <Feature id="MDR"/>
    <Feature id="MTD"/>
    <Feature id="NTP"/>
    <Feature id="SAV"/>
    <Feature id="SDU"/>
    <Feature id="WEBCNTRL"/>
  </Features>
  <automatic_sdu_submission Enabled="true"/>
  <intelligent_updating Enabled="false" SubscriptionPolicy="2DD71664-8D18-42C5-B3A0-FF0D289265BF"/>
  <customer id="b67ee4d2-baef-b4b6-6bf9-19b5ddcb2ef7"/>
</AUConfigurations>
)sophos"
};


static const std::string GL_POLICY_2 =  // NOLINT
    R"sophos(<?xml version="1.0"?>
<AUConfigurations>
  <AUConfig>
    <primary_location>
      <server Algorithm="Clear" UserPassword="A" UserName="B"/>
    </primary_location>
  </AUConfig>
</AUConfigurations>
)sophos";

static const std::string GL_CORC_POLICY = R"sophos(<?xml version="1.0"?>
<policy RevID="revisionid" policyType="37">
  <proxy>
    <address>address</address>
    <port>3456</port>
    <credentials>credentials</credentials>
  </proxy>
  <onAccessScan>
  	<autoExclusions>false</autoExclusions>
  </onAccessScan>
  <applicationManagement>
    <autoExclusions>false</autoExclusions>
	<appDetection>false</appDetection>
  </applicationManagement>

  <onDemandScan>
    <exclusions>
      <excludeRemoteFiles>false</excludeRemoteFiles>
      <filePathSet>
        <filePath>/tmp/test</filePath>
      </filePathSet>
      <processPathSet/>
    </exclusions>
  </onDemandScan>

  <detectionFeedback>
    <sendData>true</sendData>
    <sendFiles>false</sendFiles>
    <onDemandEnable>true</onDemandEnable>
  </detectionFeedback>
  <whitelist>
    <item type="path">/tmp/a/path</item>
    <item type="sha256">a651a4b1cda12a3bccde8e5c8fb83b3cff1f40977dfe562883808000ffe3f798</item>
    <item type="sha256">42268ef08462e645678ce738bd26518bc170a0404a186062e8b1bec2dc578673</item>
    <item type="cert-signer">SignerName</item>
  </whitelist>

  <blocklist>
    <item type="sha256">bd32855f94147bb8d2212770f84e42ac29379ee4617c552723bfbc9fc4dbb911</item>
  </blocklist>

  <eventJournalSizeLimit>5250</eventJournalSizeLimit>
  <eventJournalDiskSpaceLimit>10</eventJournalDiskSpaceLimit>

  <sampleUploadUri>dummy.sophos.com</sampleUploadUri>
  <sampleUploadHeaders>
    !-- <header><name>{{headerName5}}</name><value>{{headerValue5}}</value></header> -->
  </sampleUploadHeaders>

  <applicationControl>
    <messaging enabled="0" type="snmp"/>
    <messaging enabled="0" type="smtp"/>
    <messaging enabled="1" type="desktop">Custom message</messaging>
    <reportOnlyOnce enabled="1"/>
    <onAccess value="0"/>
    <onDemand value="0"/>
    <detectionAction value="0"/>
    <classSet behaviour="block"/>
    <applicationSet behaviour="block"/>
    <applicationSet behaviour="allow"/>
  </applicationControl>

  <detectionSuppressions>
    !-- <detectionKey reportSource="{{reportSource}}" name="{{detectionName}}" thumbprint="{{thumbprint}}"/> -->
  </detectionSuppressions>

  <!--  Modern web exclusions -->
  <webHttpsDecrypt>
    <!-- on/off - EAP windows endpoints only -->
    <enabled>false</enabled>
    <exclusions>
        <categoryExclusionSet>
          <category>14</category>
          <category>2</category>
        </categoryExclusionSet>
        <siteExclusionSet>
          <!-- <site>{{hostname|ip_address|ip_address_range}}</site> -->
        </siteExclusionSet>
    </exclusions>
  </webHttpsDecrypt>

  <intelix>
    <!-- <url>{{intelix_url}}</url> -->
    <!-- <lookupUrl>{{intelix_lookup_url}}</lookupUrl> -->
  </intelix>
</policy>
)sophos";

static const std::string GL_SAV_POLICY = R"sophos(<?xml version="1.0"?>
<config xmlns="http://www.sophos.com/EE/EESavConfiguration">
  <csc:Comp xmlns:csc="com.sophos\msys\csc" RevID="cffa72668ae45abe2c1caa5dff4523770e257d49e02e47848b424977c6fb3994" policyType="2"/>
  <entity>
    <productId/>
    <product-version/>
    <entityInfo/>
  </entity>
  <onAccessScan>
    <enabled>true</enabled>
    <scanBehaviour>
      <level>normal</level>
      <archives>false</archives>
      <pua>true</pua>
      <suspiciousFileDetection>false</suspiciousFileDetection>
      <scanForMacViruses>false</scanForMacViruses>
      <anti-rootkits>false</anti-rootkits>
      <aggressiveEmailScan>false</aggressiveEmailScan>
    </scanBehaviour>
    <extensions>
      <allFiles>false</allFiles>
      <excludeSophosDefined/>
      <userDefined/>
      <noExtensions>true</noExtensions>
    </extensions>
    <exclusions>
      <filePathSet/>
      <excludeRemoteFiles>false</excludeRemoteFiles>
    </exclusions>
    <macExclusions>
      <filePathSet/>
      <excludeRemoteFiles>false</excludeRemoteFiles>
    </macExclusions>
    <linuxExclusions>
      <filePathSet>
        <filePath>/mnt/</filePath>
        <filePath>/uk-filer5/</filePath>
        <filePath>*excluded*</filePath>
        <filePath>/opt/test/inputs/test_scripts/</filePath>
      </filePathSet>
      <excludeRemoteFiles>false</excludeRemoteFiles>
    </linuxExclusions>
    <virtualExclusions>
      <filePathSet/>
      <excludeRemoteFiles>false</excludeRemoteFiles>
    </virtualExclusions>
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
    <onAccess>
      <fileRead>true</fileRead>
      <fileWrite>true</fileWrite>
      <fileRename>true</fileRename>
      <accessInfectedRMBootSectors>false</accessInfectedRMBootSectors>
      <useNetworkChecksums>false</useNetworkChecksums>
      <cxmailScanAllFiles>true</cxmailScanAllFiles>
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
      <filePathSet/>
      <excludeRemoteFiles>false</excludeRemoteFiles>
    </exclusions>
    <posixExclusions>
      <filePathSet/>
      <excludeRemoteFiles>false</excludeRemoteFiles>
    </posixExclusions>
    <virtualExclusions>
      <filePathSet/>
      <excludeRemoteFiles>false</excludeRemoteFiles>
    </virtualExclusions>
    <macExclusions>
      <filePathSet/>
      <excludeRemoteFiles>false</excludeRemoteFiles>
    </macExclusions>
    <scanSet/>
    <fileReputation>true</fileReputation>
  </onDemandScan>
  <autoExclusions>true</autoExclusions>
  <appDetection>true</appDetection>
  <approvedList/>
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
    <enabled>false</enabled>
    <malicious>
      <enabled>true</enabled>
    </malicious>
    <suspicious>
      <enabled>false</enabled>
      <alertOnly>true</alertOnly>
      <skipInstallers>false</skipInstallers>
    </suspicious>
    <bops>
      <enabled>false</enabled>
      <alertOnly>true</alertOnly>
    </bops>
  </runtimeBehaviourInspection2>
  <fileReputation>
    <enabled>true</enabled>
    <level>recommended</level>
    <action>prompt</action>
  </fileReputation>
  <SIPSApprovedList/>
  <webScanning>
    <mode>on</mode>
  </webScanning>
  <webFiltering>
    <enabled>true</enabled>
    <approvedSites/>
  </webFiltering>
  <detectionFeedback>
    <sendData>true</sendData>
    <sendFiles>true</sendFiles>
    <onDemandEnable>true</onDemandEnable>
  </detectionFeedback>
  <continuousScan>
    <kernelMemoryScan>
      <enabled>true</enabled>
    </kernelMemoryScan>
  </continuousScan>
  <quarantineManager reportInStatus="false"/>
</config>

)sophos";


TEST_F(TestPolicyProcessor, testProcessFlagSettingCatchesBadJson)
{
    UsingMemoryAppender memAppend(*this);

    PolicyProcessorUnitTestClass proc;

    proc.processFlagSettings("{\"bad\" \"json\": true and false}");

    EXPECT_TRUE(appenderContains(
        "Failed to parse FLAGS policy due to parse error, reason: "));
}

TEST_F(TestPolicyProcessor, determinePolicyTypeCorc)
{
    EXPECT_CALL(*m_mockIFileSystemPtr, readFile(_)).WillOnce(Return(""));
    Tests::ScopedReplaceFileSystem replacer(std::move(m_mockIFileSystemPtr));
    auto attributeMap = Common::XmlUtilities::parseXml(GL_CORC_POLICY);
    PolicyProcessorUnitTestClass proc;
    auto policyType = PolicyProcessorUnitTestClass::determinePolicyType(attributeMap, "CORC");
    ASSERT_EQ(policyType, Plugin::PolicyType::CORC);
}

TEST_F(TestPolicyProcessor, determinePolicyTypeAlc)
{
    EXPECT_CALL(*m_mockIFileSystemPtr, readFile(_)).WillOnce(Return(""));
    Tests::ScopedReplaceFileSystem replacer(std::move(m_mockIFileSystemPtr));
    auto attributeMap = Common::XmlUtilities::parseXml(ALC_FULL_POLICY);
    PolicyProcessorUnitTestClass proc;
    auto policyType = PolicyProcessorUnitTestClass::determinePolicyType(attributeMap, "ALC");
    ASSERT_EQ(policyType, Plugin::PolicyType::ALC);
}

TEST_F(TestPolicyProcessor, determinePolicyTypeSav)
{
    EXPECT_CALL(*m_mockIFileSystemPtr, readFile(_)).WillOnce(Return(""));
    Tests::ScopedReplaceFileSystem replacer(std::move(m_mockIFileSystemPtr));
    auto attributeMap = Common::XmlUtilities::parseXml(GL_SAV_POLICY);
    PolicyProcessorUnitTestClass proc;
    auto policyType = PolicyProcessorUnitTestClass::determinePolicyType(attributeMap, "SAV");
    ASSERT_EQ(policyType, Plugin::PolicyType::SAV);
}

TEST_F(TestPolicyProcessor, determinePolicyTypeUnknownWithAppIdSav)
{
    EXPECT_CALL(*m_mockIFileSystemPtr, readFile(_)).WillOnce(Return("")); // Constructor reads Customer ID
    Tests::ScopedReplaceFileSystem replacer(std::move(m_mockIFileSystemPtr));
    auto attributeMap = Common::XmlUtilities::parseXml("<xml></xml>");
    PolicyProcessorUnitTestClass proc;
    auto policyType = PolicyProcessorUnitTestClass::determinePolicyType(attributeMap, "SAV");
    ASSERT_EQ(policyType, Plugin::PolicyType::UNKNOWN);
}

TEST_F(TestPolicyProcessor, determinePolicyTypeUnknownWithUnkownAppId)
{
    Tests::ScopedReplaceFileSystem replacer(std::move(m_mockIFileSystemPtr));
    auto attributeMap = Common::XmlUtilities::parseXml("<xml></xml>");
    auto policyType = PolicyProcessorUnitTestClass::determinePolicyType(attributeMap, "not a known APP ID");
    ASSERT_EQ(policyType, Plugin::PolicyType::UNKNOWN);
}

// TO BE DELETED ------------------------------------------


TEST_F(TestPolicyProcessor, DISABLED_processOnAccessPolicyEnabled)
{
    EXPECT_CALL(*m_mockIFileSystemPtr, readFile(_)).WillOnce(Return(""));
    EXPECT_CALL(*m_mockIFileSystemPtr, writeFileAtomically(m_soapConfigPath,
                                                           R"sophos({"enabled":"true","excludeRemoteFiles":"false","exclusions":["x","y"]})sophos",
                                                           _,
                                                           0640));

    Tests::ScopedReplaceFileSystem replacer(std::move(m_mockIFileSystemPtr));

    PolicyProcessorUnitTestClass proc;

    std::string policyXml = R"sophos(<?xml version="1.0"?>
<config>
    <onAccessScan>
        <enabled>true</enabled>
        <linuxExclusions>
          <filePathSet>
            <filePath>x</filePath>
            <filePath>y</filePath>
          </filePathSet>
          <excludeRemoteFiles>false</excludeRemoteFiles>
        </linuxExclusions>
    </onAccessScan>
</config>
)sophos";
    auto attributeMap = Common::XmlUtilities::parseXml(policyXml);


    auto telemetryStr = Common::Telemetry::TelemetryHelper::getInstance().serialiseAndReset();
    auto telemetry = nlohmann::json::parse(telemetryStr);
    EXPECT_EQ(telemetry["onAccessConfigured"], true);
}

TEST_F(TestPolicyProcessor, DISABLED_processOnAccessPolicyDisabled_DISABLED)
{
    EXPECT_CALL(*m_mockIFileSystemPtr, readFile(_)).WillOnce(Return(""));
    EXPECT_CALL(*m_mockIFileSystemPtr, writeFileAtomically(m_soapConfigPath,
                                                           R"sophos({"enabled":"false","excludeRemoteFiles":"false","exclusions":["x","y"]})sophos",
                                                           _,
                                                           0640));

    Tests::ScopedReplaceFileSystem replacer(std::move(m_mockIFileSystemPtr));

    PolicyProcessorUnitTestClass proc;

    std::string policyXml = R"sophos(<?xml version="1.0"?>
<config>
    <onAccessScan>
        <enabled>false</enabled>
        <linuxExclusions>
          <filePathSet>
            <filePath>x</filePath>
            <filePath>y</filePath>
          </filePathSet>
          <excludeRemoteFiles>false</excludeRemoteFiles>
        </linuxExclusions>
    </onAccessScan>
</config>
)sophos";
    auto attributeMap = Common::XmlUtilities::parseXml(policyXml);


    // Verify the telemetry is updated
    auto telemetryStr = Common::Telemetry::TelemetryHelper::getInstance().serialiseAndReset();
    auto telemetry = nlohmann::json::parse(telemetryStr);
    EXPECT_EQ(telemetry["onAccessConfigured"], false);
}

TEST_F(TestPolicyProcessor, DISABLED_processInvalidOnAccessPolicy_DISABLED)
{
    EXPECT_CALL(*m_mockIFileSystemPtr, readFile(_)).WillOnce(Return(""));
    EXPECT_CALL(*m_mockIFileSystemPtr, writeFileAtomically(m_soapConfigPath,
                                                           R"sophos({"enabled":"false","excludeRemoteFiles":"false","exclusions":["x","y"]})sophos",
                                                           _,
                                                           0640));

    Tests::ScopedReplaceFileSystem replacer(std::move(m_mockIFileSystemPtr));

    PolicyProcessorUnitTestClass proc;

    std::string policyXml = R"sophos(<?xml version="1.0"?>
<config>
    <onAccessScan>
        <enabled>this is supposed to be true/false</enabled>
        <linuxExclusions>
          <filePathSet>
            <filePath>x</filePath>
            <filePath>y</filePath>
          </filePathSet>
          <excludeRemoteFiles>this is supposed to be true/false</excludeRemoteFiles>
        </linuxExclusions>
    </onAccessScan>
</config>
)sophos";
    auto attributeMap = Common::XmlUtilities::parseXml(policyXml);

}
