// Copyright 2020-2023 Sophos Limited. All rights reserved.

#include "TestPolicyProcessor.h"

#include "datatypes/sophos_filesystem.h"

#include <Common/Helpers/FileSystemReplaceAndRestore.h>
#include <Common/FileSystem/IFileSystemException.h>

#include <gtest/gtest.h>

namespace fs = sophos_filesystem;

namespace
{
    class TestPolicyProcessor_SAV_policy : public TestPolicyProcessorBase
    {
    protected:
        void SetUp() override
        {
            setupBase();
            
            m_soapConfigPath = m_testDir / "var/on_access_policy.json";
        }

        void expectWriteSoapdConfig()
        {
#ifdef USE_ON_ACCESS_EXCLUSIONS_FROM_SAV_POLICY
            EXPECT_CALL(*m_mockIFileSystemPtr, writeFileAtomically(m_soapConfigPath,
                                                                   _,
                                                                   _,
                                                                   0640)).WillRepeatedly(Return());
#endif
        }

        void expectReadSoapdConfig()
        {
#ifdef USE_ON_ACCESS_EXCLUSIONS_FROM_SAV_POLICY
            EXPECT_CALL(*m_mockIFileSystemPtr, readFile(m_soapConfigPath)).WillRepeatedly(
                    Throw(Common::FileSystem::IFileSystemException("Test exception"))
                );
#endif
        }

        void expectWriteSusiConfigFromBool(bool sxlEnabled)
        {
            if (sxlEnabled)
            {
                expectWriteSusiConfigFromString(R"sophos({"enableSxlLookup":true,"machineLearning":true,"pathAllowList":[],"puaApprovedList":[],"shaAllowList":[]})sophos");
            }
            else
            {
                expectWriteSusiConfigFromString(R"sophos({"enableSxlLookup":false,"machineLearning":true,"pathAllowList":[],"puaApprovedList":[],"shaAllowList":[]})sophos");
            }
        }

        void expectWriteSusiConfig(bool sxlEnabled, const std::vector<std::string>& puaApprovedList)
        {
            nlohmann::json settings;
            settings["enableSxlLookup"] = sxlEnabled;
            settings["machineLearning"] = true;
            settings["puaApprovedList"] = puaApprovedList;
            settings["pathAllowList"] = nlohmann::json::array();
            settings["shaAllowList"] = nlohmann::json::array();
            expectWriteSusiConfigFromString(settings.dump());
        }

        std::string m_soapConfigPath;
    };
}

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
    <posixExclusions>f
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

TEST_F(TestPolicyProcessor_SAV_policy, processSavPolicy)
{
    expectWriteSoapdConfig();
    expectReadSoapdConfig();
    expectConstructorCalls();
    expectWriteSusiConfigFromBool(false);

    Tests::ScopedReplaceFileSystem replacer(std::move(m_mockIFileSystemPtr));

    PolicyProcessorUnitTestClass proc;

    std::string policyXml = R"sophos(<?xml version="1.0"?>
<config>
    <detectionFeedback>
        <sendData>false</sendData>
    </detectionFeedback>
</config>
)sophos";

    auto attributeMap = Common::XmlUtilities::parseXml(policyXml);
    proc.processSavPolicy(attributeMap);
    EXPECT_TRUE(proc.restartThreatDetector());
}

TEST_F(TestPolicyProcessor_SAV_policy, defaultSXL4lookupValueIsTrue)
{
    Plugin::PolicyProcessor proc{nullptr};
    EXPECT_TRUE(proc.getSXL4LookupsEnabled());
}

TEST_F(TestPolicyProcessor_SAV_policy, sxl_changes_cause_restart)
{
    expectReadSoapdConfig();
    expectConstructorCalls();
    expectWriteSoapdConfig();
    {
        InSequence seq;
        expectWriteSusiConfigFromBool(true);
        expectWriteSusiConfigFromBool(false);
        expectWriteSusiConfigFromBool(true);
    }

    Tests::ScopedReplaceFileSystem replacer(std::move(m_mockIFileSystemPtr));

    PolicyProcessorUnitTestClass proc;

    std::string policyXmlTrue = R"sophos(<?xml version="1.0"?>
<config>
    <detectionFeedback>
        <sendData>true</sendData>
    </detectionFeedback>
</config>
)sophos";


    std::string policyXmlFalse = R"sophos(<?xml version="1.0"?>
<config>
    <detectionFeedback>
        <sendData>false</sendData>
    </detectionFeedback>
</config>
)sophos";

    auto attributeMapTrue = Common::XmlUtilities::parseXml(policyXmlTrue);
    auto attributeMapFalse = Common::XmlUtilities::parseXml(policyXmlFalse);

    proc.processSavPolicy(attributeMapTrue);
    EXPECT_FALSE(proc.restartThreatDetector()); // Matches built in default so no need to restart
    EXPECT_TRUE(proc.getSXL4LookupsEnabled());

    proc.processSavPolicy(attributeMapFalse);
    EXPECT_TRUE(proc.restartThreatDetector());
    EXPECT_FALSE(proc.getSXL4LookupsEnabled());

    proc.processSavPolicy(attributeMapTrue);
    EXPECT_TRUE(proc.restartThreatDetector());
    EXPECT_TRUE(proc.getSXL4LookupsEnabled());
}

TEST_F(TestPolicyProcessor_SAV_policy, processSavPolicyMaintainsSXL4state)
{
    expectReadSoapdConfig();
    expectConstructorCalls();
    expectWriteSoapdConfig();
    {
        InSequence seq;
        expectWriteSusiConfigFromBool(true);
        expectWriteSusiConfigFromBool(false);
    }

    Tests::ScopedReplaceFileSystem replacer(std::move(m_mockIFileSystemPtr));

    PolicyProcessorUnitTestClass proc;

    std::string policyXmlTrue = R"sophos(<?xml version="1.0"?>
<config>
    <detectionFeedback>
        <sendData>true</sendData>
    </detectionFeedback>
</config>
)sophos";


    std::string policyXmlFalse = R"sophos(<?xml version="1.0"?>
<config>
    <detectionFeedback>
        <sendData>false</sendData>
    </detectionFeedback>
</config>
)sophos";

    auto attributeMapTrue = Common::XmlUtilities::parseXml(policyXmlTrue);
    auto attributeMapFalse = Common::XmlUtilities::parseXml(policyXmlFalse);

    proc.processSavPolicy(attributeMapTrue);
    EXPECT_FALSE(proc.restartThreatDetector()); // Change if the default changes to false
    EXPECT_TRUE(proc.getSXL4LookupsEnabled());

    proc.processSavPolicy(attributeMapTrue);
    EXPECT_FALSE(proc.restartThreatDetector());
    EXPECT_TRUE(proc.getSXL4LookupsEnabled());

    proc.processSavPolicy(attributeMapFalse);
    EXPECT_TRUE(proc.restartThreatDetector());
    EXPECT_FALSE(proc.getSXL4LookupsEnabled());

    proc.processSavPolicy(attributeMapFalse);
    EXPECT_FALSE(proc.restartThreatDetector());
    EXPECT_FALSE(proc.getSXL4LookupsEnabled());
}

TEST_F(TestPolicyProcessor_SAV_policy, processSavPolicyMissing)
{
    expectReadSoapdConfig();
    expectConstructorCalls();
    expectWriteSusiConfigFromBool(false);
    expectWriteSusiConfigFromBool(true);
    expectWriteSoapdConfig();

    Tests::ScopedReplaceFileSystem replacer(std::move(m_mockIFileSystemPtr));

    PolicyProcessorUnitTestClass proc;

    std::string policyXml = R"sophos(<?xml version="1.0"?>
<config>
    <detectionFeedback>
        <sendData>false</sendData>
    </detectionFeedback>
</config>
)sophos";

    std::string policyXmlInvalid = R"sophos(<?xml version="1.0"?>
<config>
</config>
)sophos";

    auto attributeMap = Common::XmlUtilities::parseXml(policyXml);
    auto attributeMapInvalid = Common::XmlUtilities::parseXml(policyXmlInvalid);
    proc.processSavPolicy(attributeMap);

    proc.processSavPolicy(attributeMapInvalid);
    EXPECT_TRUE(proc.restartThreatDetector());
}

TEST_F(TestPolicyProcessor_SAV_policy, processSavPolicyInvalid)
{
    expectReadSoapdConfig();
    expectConstructorCalls();
    expectWriteSusiConfigFromBool(false);
    expectWriteSusiConfigFromBool(true);
    expectWriteSoapdConfig();

    Tests::ScopedReplaceFileSystem replacer(std::move(m_mockIFileSystemPtr));

    PolicyProcessorUnitTestClass proc;

    std::string policyXml = R"sophos(<?xml version="1.0"?>
<config>
    <detectionFeedback>
        <sendData>false</sendData>
    </detectionFeedback>
</config>
)sophos";

    std::string policyXmlInvalid = R"sophos(<?xml version="1.0"?>
<config>
    <detectionFeedback>
        <sendData>absolute nonsense</sendData>
    </detectionFeedback>
</config>
)sophos";

    auto attributeMap = Common::XmlUtilities::parseXml(policyXml);
    auto attributeMapInvalid = Common::XmlUtilities::parseXml(policyXmlInvalid);
    proc.processSavPolicy(attributeMap);
    proc.processSavPolicy(attributeMapInvalid);
    EXPECT_TRUE(proc.restartThreatDetector());
}

#ifdef USE_ON_ACCESS_EXCLUSIONS_FROM_SAV_POLICY

// Tests for getting on-access exclusions from SAV policy

TEST_F(TestPolicyProcessor_SAV_policy, getOnAccessExclusions)
{
    expectReadSoapdConfig();
    expectConstructorCalls();
    expectWriteSusiConfigFromBool(true);
    EXPECT_CALL(*m_mockIFileSystemPtr, writeFileAtomically(m_soapConfigPath,
                                                           R"sophos({"detectPUAs":true,"exclusions":["x","y"]})sophos",
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
    proc.processSavPolicy(attributeMap);
}

#endif


TEST_F(TestPolicyProcessor_SAV_policy, processSavPolicyWithPuas)
{
    expectWriteSoapdConfig();
    expectReadSoapdConfig();
    expectConstructorCalls();
    expectWriteSusiConfig(false, {"PUA1","PUA2"});
    Tests::ScopedReplaceFileSystem replacer(std::move(m_mockIFileSystemPtr));
    PolicyProcessorUnitTestClass proc;
    std::string policyXml = R"sophos(<?xml version="1.0"?>
<config>
    <detectionFeedback>
        <sendData>false</sendData>
    </detectionFeedback>
    <approvedList>
        <puaName>PUA1</puaName>
        <puaName>PUA2</puaName>
    </approvedList>
</config>
)sophos";

    auto attributeMap = Common::XmlUtilities::parseXml(policyXml);
    proc.processSavPolicy(attributeMap);
    EXPECT_TRUE(proc.restartThreatDetector());
}

TEST_F(TestPolicyProcessor_SAV_policy, processSavPolicyWithNewPuasNeedsThreatdetectorReload)
{
    expectWriteSoapdConfig();
    expectReadSoapdConfig();
    expectConstructorCalls();
    expectWriteSusiConfig(false, {"PUA1","PUA2"});
    expectWriteSusiConfig(false, {"PUA1"});
    Tests::ScopedReplaceFileSystem replacer(std::move(m_mockIFileSystemPtr));
    PolicyProcessorUnitTestClass proc;
    std::string policyXml1 = R"sophos(<?xml version="1.0"?>
<config>
    <detectionFeedback>
        <sendData>false</sendData>
    </detectionFeedback>
    <approvedList>
        <puaName>PUA1</puaName>
        <puaName>PUA2</puaName>
    </approvedList>
</config>
)sophos";

    std::string policyXml2 = R"sophos(<?xml version="1.0"?>
<config>
    <detectionFeedback>
        <sendData>false</sendData>
    </detectionFeedback>
    <approvedList>
        <puaName>PUA1</puaName>
    </approvedList>
</config>
)sophos";

    auto policyMap1 = Common::XmlUtilities::parseXml(policyXml1);
    auto policyMap2 = Common::XmlUtilities::parseXml(policyXml2);

    proc.processSavPolicy(policyMap1);
    EXPECT_TRUE(proc.reloadThreatDetectorConfiguration());
    // Restart may be true depending on default

    proc.processSavPolicy(policyMap2);
    EXPECT_TRUE(proc.reloadThreatDetectorConfiguration());
    EXPECT_FALSE(proc.restartThreatDetector());

    // Repeat policy, so nothing should change
    proc.processSavPolicy(policyMap2);
    EXPECT_FALSE(proc.reloadThreatDetectorConfiguration());
    EXPECT_FALSE(proc.restartThreatDetector());
}