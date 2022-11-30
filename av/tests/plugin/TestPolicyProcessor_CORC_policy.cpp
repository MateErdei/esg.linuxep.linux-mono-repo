// Copyright 2020-2022, Sophos Limited.  All rights reserved.

#include "TestPolicyProcessor.h"

#include <Common/Helpers/FileSystemReplaceAndRestore.h>
#include <Common/FileSystem/IFileSystemException.h>

#include <gtest/gtest.h>

namespace fs = sophos_filesystem;

namespace
{
    class TestPolicyProcessor_CORC_policy : public TestPolicyProcessorBase
    {
    protected:
        void SetUp() override
        {
            setupBase();
            m_susiStartupConfigPath = m_testDir / "var/susi_startup_settings.json";
            m_susiStartupConfigChrootPath = std::string(m_testDir / "chroot") + m_susiStartupConfigPath;
        }

        void expectWriteSusiConfigFromString(const std::string& expected)
        {
            EXPECT_CALL(*m_mockIFileSystemPtr, writeFileAtomically(m_susiStartupConfigPath, expected,_ ,0640)).Times(1);
            EXPECT_CALL(*m_mockIFileSystemPtr, writeFileAtomically(m_susiStartupConfigChrootPath, expected,_ ,0640)).Times(1);
        }

        std::string m_susiStartupConfigPath;
        std::string m_susiStartupConfigChrootPath;
    };

}

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

TEST_F(TestPolicyProcessor_CORC_policy, processCorcPolicyWithAllowList)
{
    const std::string settingsJson = R"sophos({"enableSxlLookup":true,"shaAllowList":["a651a4b1cda12a3bccde8e5c8fb83b3cff1f40977dfe562883808000ffe3f798","42268ef08462e645678ce738bd26518bc170a0404a186062e8b1bec2dc578673"]})sophos";
    expectReadCustomerIdOnce();
    expectWriteSusiConfigFromString(settingsJson);

    Tests::ScopedReplaceFileSystem replacer(std::move(m_mockIFileSystemPtr));
    auto attributeMap = Common::XmlUtilities::parseXml(GL_CORC_POLICY);
    PolicyProcessorUnitTestClass proc;
    ASSERT_NO_THROW(proc.processCorcPolicy(attributeMap));
}

TEST_F(TestPolicyProcessor_CORC_policy, processCorcPolicyWithEmptyAllowList)
{
    std::string emptyAllowList = R"(<?xml version="1.0"?>
        <policy RevID="revisionid" policyType="37">
          <whitelist>
          </whitelist>
        </policy>)";

    std::string settingsJson = R"sophos({"enableSxlLookup":true,"shaAllowList":[]})sophos";
    expectReadCustomerIdOnce();
    expectWriteSusiConfigFromString(settingsJson);

    Tests::ScopedReplaceFileSystem replacer(std::move(m_mockIFileSystemPtr));
    auto attributeMap = Common::XmlUtilities::parseXml(emptyAllowList);
    PolicyProcessorUnitTestClass proc;
    ASSERT_NO_THROW(proc.processCorcPolicy(attributeMap));
}

TEST_F(TestPolicyProcessor_CORC_policy, processCorcPolicyWithMissingAllowList)
{
    std::string missingAllowList = R"(<?xml version="1.0"?>
        <policy RevID="revisionid" policyType="37">
        </policy>)";

    std::string settingsJson = R"sophos({"enableSxlLookup":true,"shaAllowList":[]})sophos";
    expectReadCustomerIdOnce();
    expectWriteSusiConfigFromString(settingsJson);

    Tests::ScopedReplaceFileSystem replacer(std::move(m_mockIFileSystemPtr));
    auto attributeMap = Common::XmlUtilities::parseXml(missingAllowList);
    PolicyProcessorUnitTestClass proc;
    ASSERT_NO_THROW(proc.processCorcPolicy(attributeMap));
}

TEST_F(TestPolicyProcessor_CORC_policy, processCorcPolicyWithNonShaTypeAllowListItems)
{
    std::string policy = R"(<?xml version="1.0"?>
        <policy RevID="revisionid" policyType="37">
        <whitelist>
            <item type="path">/tmp/a/path</item>
            <item type="cert-signer">SignerName</item>
        </whitelist>
        </policy>)";

    std::string settingsJson = R"sophos({"enableSxlLookup":true,"shaAllowList":[]})sophos";
    expectReadCustomerIdOnce();
    expectWriteSusiConfigFromString(settingsJson);

    Tests::ScopedReplaceFileSystem replacer(std::move(m_mockIFileSystemPtr));
    auto attributeMap = Common::XmlUtilities::parseXml(policy);
    PolicyProcessorUnitTestClass proc;
    ASSERT_NO_THROW(proc.processCorcPolicy(attributeMap));
}

TEST_F(TestPolicyProcessor_CORC_policy, processCorcPolicyWithOneEmptyShaTypeAllowListItem)
{
    std::string policy = R"(<?xml version="1.0"?>
        <policy RevID="revisionid" policyType="37">
        <whitelist>
            <item type="path">/tmp/a/path</item>
            <item type="sha256"></item>
            <item type="sha256">42268ef08462e645678ce738bd26518bc170a0404a186062e8b1bec2dc578673</item>
            <item type="cert-signer">SignerName</item>
        </whitelist>
        </policy>)";

    std::string settingsJson = R"sophos({"enableSxlLookup":true,"shaAllowList":["42268ef08462e645678ce738bd26518bc170a0404a186062e8b1bec2dc578673"]})sophos";
    expectReadCustomerIdOnce();
    expectWriteSusiConfigFromString(settingsJson);

    Tests::ScopedReplaceFileSystem replacer(std::move(m_mockIFileSystemPtr));
    auto attributeMap = Common::XmlUtilities::parseXml(policy);
    PolicyProcessorUnitTestClass proc;
    ASSERT_NO_THROW(proc.processCorcPolicy(attributeMap));
}

TEST_F(TestPolicyProcessor_CORC_policy, processCorcPolicyWithShaAndCommentInAllowList)
{
    std::string policy = R"(<?xml version="1.0"?>
        <policy RevID="revisionid" policyType="37">
        <whitelist>
            <item type="path">/tmp/a/path</item>
            <!-- SHA256 of some file -->
            <item type="sha256">42268ef08462e645678ce738bd26518bc170a0404a186062e8b1bec2dc578673</item>
            <item type="cert-signer">SignerName</item>
        </whitelist>
        </policy>)";

    std::string settingsJson = R"sophos({"enableSxlLookup":true,"shaAllowList":["42268ef08462e645678ce738bd26518bc170a0404a186062e8b1bec2dc578673"]})sophos";
    expectReadCustomerIdOnce();
    expectWriteSusiConfigFromString(settingsJson);

    Tests::ScopedReplaceFileSystem replacer(std::move(m_mockIFileSystemPtr));
    auto attributeMap = Common::XmlUtilities::parseXml(policy);
    PolicyProcessorUnitTestClass proc;
    ASSERT_NO_THROW(proc.processCorcPolicy(attributeMap));
}


