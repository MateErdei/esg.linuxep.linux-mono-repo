// Copyright 2020-2022, Sophos Limited.  All rights reserved.

#include <pluginimpl/PolicyProcessor.h>

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
    class TestPolicyProcessor_SAV_policy : public PluginMemoryAppenderUsingTests
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
        std::unique_ptr<StrictMock<MockFileSystem>> m_mockIFileSystemPtr;
    };

    class PolicyProcessorUnitTestClass : public Plugin::PolicyProcessor
    {
    public:
        PolicyProcessorUnitTestClass()
            : Plugin::PolicyProcessor(nullptr)
        {}
    protected:
        void notifyOnAccessProcess(scan_messages::E_COMMAND_TYPE /*cmd*/) override
        {
            PRINT("Notified soapd");
        }
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
    EXPECT_CALL(*m_mockIFileSystemPtr, readFile(_)).WillOnce(Return(""));
    EXPECT_CALL(*m_mockIFileSystemPtr, writeFile(m_susiStartupConfigPath, R"sophos({"enableSxlLookup":false})sophos"));
    EXPECT_CALL(*m_mockIFileSystemPtr, writeFile(m_susiStartupConfigChrootPath, R"sophos({"enableSxlLookup":false})sophos"));
    EXPECT_CALL(*m_mockIFileSystemPtr, writeFileAtomically(m_soapConfigPath,
                                                           R"sophos({"enabled":"false","excludeRemoteFiles":"false","exclusions":[]})sophos",
                                                           _,
                                                           0640));

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

TEST_F(TestPolicyProcessor_SAV_policy, processSavPolicyChanged)
{
    EXPECT_CALL(*m_mockIFileSystemPtr, writeFileAtomically(m_soapConfigPath,
                                                           R"sophos({"enabled":"false","excludeRemoteFiles":"false","exclusions":[]})sophos",
                                                           _,
                                                           0640)).Times(3);


    EXPECT_CALL(*m_mockIFileSystemPtr, readFile(_)).WillRepeatedly(Return(""));
    {
        InSequence seq;
        EXPECT_CALL(*m_mockIFileSystemPtr, writeFile(m_susiStartupConfigPath, R"sophos({"enableSxlLookup":true})sophos"));
        EXPECT_CALL(*m_mockIFileSystemPtr, writeFile(m_susiStartupConfigChrootPath, R"sophos({"enableSxlLookup":true})sophos"));
        EXPECT_CALL(*m_mockIFileSystemPtr, writeFile(m_susiStartupConfigPath, R"sophos({"enableSxlLookup":false})sophos"));
        EXPECT_CALL(*m_mockIFileSystemPtr, writeFile(m_susiStartupConfigChrootPath, R"sophos({"enableSxlLookup":false})sophos"));
        EXPECT_CALL(*m_mockIFileSystemPtr, writeFile(m_susiStartupConfigPath, R"sophos({"enableSxlLookup":true})sophos"));
        EXPECT_CALL(*m_mockIFileSystemPtr, writeFile(m_susiStartupConfigChrootPath, R"sophos({"enableSxlLookup":true})sophos"));
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
    EXPECT_TRUE(proc.restartThreatDetector());
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
    EXPECT_CALL(*m_mockIFileSystemPtr, writeFileAtomically(m_soapConfigPath,
                                                           R"sophos({"enabled":"false","excludeRemoteFiles":"false","exclusions":[]})sophos",
                                                           _,
                                                           0640)).Times(4);


    EXPECT_CALL(*m_mockIFileSystemPtr, readFile(_)).WillRepeatedly(Return(""));
    {
        InSequence seq;
        EXPECT_CALL(*m_mockIFileSystemPtr, writeFile(m_susiStartupConfigPath, R"sophos({"enableSxlLookup":true})sophos"));
        EXPECT_CALL(*m_mockIFileSystemPtr, writeFile(m_susiStartupConfigChrootPath, R"sophos({"enableSxlLookup":true})sophos"));
        EXPECT_CALL(*m_mockIFileSystemPtr, writeFile(m_susiStartupConfigPath, R"sophos({"enableSxlLookup":false})sophos"));
        EXPECT_CALL(*m_mockIFileSystemPtr, writeFile(m_susiStartupConfigChrootPath, R"sophos({"enableSxlLookup":false})sophos"));
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
    EXPECT_TRUE(proc.restartThreatDetector());
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
    EXPECT_CALL(*m_mockIFileSystemPtr, readFile(_)).WillOnce(Return(""));
    EXPECT_CALL(*m_mockIFileSystemPtr, writeFile(m_susiStartupConfigPath, R"sophos({"enableSxlLookup":false})sophos"));
    EXPECT_CALL(*m_mockIFileSystemPtr, writeFile(m_susiStartupConfigChrootPath, R"sophos({"enableSxlLookup":false})sophos"));
    EXPECT_CALL(*m_mockIFileSystemPtr, writeFile(m_susiStartupConfigPath, R"sophos({"enableSxlLookup":true})sophos"));
    EXPECT_CALL(*m_mockIFileSystemPtr, writeFile(m_susiStartupConfigChrootPath, R"sophos({"enableSxlLookup":true})sophos"));
    EXPECT_CALL(*m_mockIFileSystemPtr, writeFileAtomically(m_soapConfigPath,
                                                           R"sophos({"enabled":"false","excludeRemoteFiles":"false","exclusions":[]})sophos",
                                                           _,
                                                           0640)).Times(2);

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
    EXPECT_CALL(*m_mockIFileSystemPtr, readFile(_)).WillOnce(Return(""));
    EXPECT_CALL(*m_mockIFileSystemPtr, writeFile(m_susiStartupConfigPath, R"sophos({"enableSxlLookup":false})sophos"));
    EXPECT_CALL(*m_mockIFileSystemPtr, writeFile(m_susiStartupConfigChrootPath, R"sophos({"enableSxlLookup":false})sophos"));
    EXPECT_CALL(*m_mockIFileSystemPtr, writeFile(m_susiStartupConfigPath, R"sophos({"enableSxlLookup":true})sophos"));
    EXPECT_CALL(*m_mockIFileSystemPtr, writeFile(m_susiStartupConfigChrootPath, R"sophos({"enableSxlLookup":true})sophos"));
    EXPECT_CALL(*m_mockIFileSystemPtr, writeFileAtomically(m_soapConfigPath,
                                                           R"sophos({"enabled":"false","excludeRemoteFiles":"false","exclusions":[]})sophos",
                                                           _,
                                                           0640)).Times(2);

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

TEST_F(TestPolicyProcessor_SAV_policy, testProcessFlagSettingsEnabled)
{
    UsingMemoryAppender memAppend(*this);

    EXPECT_CALL(*m_mockIFileSystemPtr, readFile(_)).WillOnce(Return(""));

    EXPECT_CALL(
        *m_mockIFileSystemPtr,
        writeFileAtomically(m_soapFlagConfigPath, R"sophos({"oa_enabled":true})sophos", _, 0640));

    Tests::ScopedReplaceFileSystem replacer(std::move(m_mockIFileSystemPtr));

    PolicyProcessorUnitTestClass proc;

    proc.processFlagSettings(R"sophos({"av.onaccess.enabled": true, "safestore.enabled": true})sophos");

    EXPECT_TRUE(appenderContains(
        "On-access is enabled in the FLAGS policy, assuming on-access policy settings"));
    EXPECT_TRUE(appenderContains("SafeStore flag set. Setting SafeStore to enabled."));
    ASSERT_TRUE(proc.isSafeStoreEnabled());
}

TEST_F(TestPolicyProcessor_SAV_policy, testProcessFlagSettingsDisabled)
{
    UsingMemoryAppender memAppend(*this);

    EXPECT_CALL(*m_mockIFileSystemPtr, readFile(_)).WillOnce(Return(""));

    EXPECT_CALL(
        *m_mockIFileSystemPtr,
        writeFileAtomically(m_soapFlagConfigPath, R"sophos({"oa_enabled":false})sophos", _, 0640));

    Tests::ScopedReplaceFileSystem replacer(std::move(m_mockIFileSystemPtr));

    PolicyProcessorUnitTestClass proc;

    proc.processFlagSettings(R"sophos({"av.onaccess.enabled": false, "safestore.enabled": false})sophos");

    EXPECT_TRUE(appenderContains(
        "On-access is disabled in the FLAGS policy, overriding on-access policy settings"));
    EXPECT_TRUE(appenderContains("SafeStore flag not set. Setting SafeStore to disabled."));
    ASSERT_FALSE(proc.isSafeStoreEnabled());
}

TEST_F(TestPolicyProcessor_SAV_policy, testProcessFlagSettingsDefault)
{
    UsingMemoryAppender memAppend(*this);

    EXPECT_CALL(*m_mockIFileSystemPtr, readFile(_)).WillOnce(Return(""));

    EXPECT_CALL(
        *m_mockIFileSystemPtr,
        writeFileAtomically(m_soapFlagConfigPath, R"sophos({"oa_enabled":false})sophos", _, 0640));

    Tests::ScopedReplaceFileSystem replacer(std::move(m_mockIFileSystemPtr));

    PolicyProcessorUnitTestClass proc;

    proc.processFlagSettings("{\"av.something_else\":  false}");

    EXPECT_TRUE(appenderContains("No on-access flag found, overriding on-access policy settings"));
    EXPECT_TRUE(appenderContains("SafeStore flag not set. Setting SafeStore to disabled."));
    ASSERT_FALSE(proc.isSafeStoreEnabled());
}

TEST_F(TestPolicyProcessor_SAV_policy, testWriteFlagConfigFailedOnAccess)
{
    UsingMemoryAppender memAppend(*this);

    EXPECT_CALL(*m_mockIFileSystemPtr, readFile(_)).WillOnce(Return(""));

    Common::FileSystem::IFileSystemException ex("error!");
    EXPECT_CALL(
        *m_mockIFileSystemPtr,
        writeFileAtomically(m_soapFlagConfigPath, R"sophos({"oa_enabled":false})sophos", _, 0640))
        .WillOnce(Throw(ex));

    Tests::ScopedReplaceFileSystem replacer(std::move(m_mockIFileSystemPtr));

    PolicyProcessorUnitTestClass proc;

    proc.processFlagSettings(R"sophos({"av.onaccess.enabled": false, "safestore.enabled": true})sophos");

    EXPECT_TRUE(appenderContains(
        "Failed to write Flag Config, Sophos On Access Process will use the default settings (on-access disabled)"));
    EXPECT_TRUE(appenderContains("SafeStore flag set. Setting SafeStore to enabled."));
    ASSERT_TRUE(proc.isSafeStoreEnabled());
}
