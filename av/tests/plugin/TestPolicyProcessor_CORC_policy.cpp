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
        }
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
    <lookupUrl>{{intelix_lookup_url}}</lookupUrl>
  </intelix>
</policy>
)sophos";

TEST_F(TestPolicyProcessor_CORC_policy, processCorcPolicyWithAllowList)
{
    expectConstructorCalls();
    std::string baseJson;
    std::string chrootJson;
    saveSusiConfigFromBothWrites(_, baseJson, chrootJson);

    Tests::ScopedReplaceFileSystem replacer(std::move(m_mockIFileSystemPtr));
    auto attributeMap = Common::XmlUtilities::parseXml(GL_CORC_POLICY);
    PolicyProcessorUnitTestClass proc;
    ASSERT_NO_THROW(proc.processCorcPolicy(attributeMap));
    EXPECT_EQ(baseJson, chrootJson);
    auto actual = nlohmann::json::parse(chrootJson);
    auto pathAllowList = actual.at("pathAllowList");
    ASSERT_EQ(pathAllowList.size(), 1);
    EXPECT_EQ(pathAllowList[0].get<std::string>(), "/tmp/a/path");
    auto shaAllowList = actual.at("shaAllowList");
    ASSERT_EQ(shaAllowList.size(), 2);
    EXPECT_EQ(shaAllowList[0], "a651a4b1cda12a3bccde8e5c8fb83b3cff1f40977dfe562883808000ffe3f798");
    EXPECT_EQ(shaAllowList[1], "42268ef08462e645678ce738bd26518bc170a0404a186062e8b1bec2dc578673");
}

TEST_F(TestPolicyProcessor_CORC_policy, processCorcPolicyWithEmptyAllowList)
{
    std::string emptyAllowList = R"(<?xml version="1.0"?>
        <policy RevID="revisionid" policyType="37">
          <whitelist>
          </whitelist>
        </policy>)";

    expectConstructorCalls();
    std::string actualJson;
    auto expectedJsonFragment = HasSubstr(R"sophos("pathAllowList":[])sophos");
    saveSusiConfigFromWrite(expectedJsonFragment, actualJson);

    Tests::ScopedReplaceFileSystem replacer(std::move(m_mockIFileSystemPtr));
    PolicyProcessorUnitTestClass proc;
    ASSERT_NO_THROW(proc.processCORCpolicyFromString(emptyAllowList));
    auto actual = nlohmann::json::parse(actualJson);
    EXPECT_TRUE(actual.at("pathAllowList").empty());
    EXPECT_TRUE(actual.at("puaApprovedList").empty());
    EXPECT_TRUE(actual.at("shaAllowList").empty());
}

TEST_F(TestPolicyProcessor_CORC_policy, processCorcPolicyWithMissingAllowList)
{
    std::string missingAllowList = R"(<?xml version="1.0"?>
        <policy RevID="revisionid" policyType="37">
        </policy>)";

    expectConstructorCalls();
    std::string actualJson;
    auto expectedJsonFragment = HasSubstr(R"sophos("pathAllowList":[],"puaApprovedList":[],"shaAllowList":[])sophos");
    saveSusiConfigFromWrite(expectedJsonFragment, actualJson);

    Tests::ScopedReplaceFileSystem replacer(std::move(m_mockIFileSystemPtr));
    PolicyProcessorUnitTestClass proc;
    ASSERT_NO_THROW(proc.processCORCpolicyFromString(missingAllowList));
    auto actual = nlohmann::json::parse(actualJson);
    EXPECT_TRUE(actual.at("pathAllowList").empty());
    EXPECT_TRUE(actual.at("puaApprovedList").empty());
    EXPECT_TRUE(actual.at("shaAllowList").empty());
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

    expectConstructorCalls();
    std::string actualJson;
    auto expectedJsonFragment = HasSubstr(R"sophos("pathAllowList":["/tmp/a/path"],"puaApprovedList":[],"shaAllowList":[])sophos");
    saveSusiConfigFromWrite(expectedJsonFragment, actualJson);

    Tests::ScopedReplaceFileSystem replacer(std::move(m_mockIFileSystemPtr));
    PolicyProcessorUnitTestClass proc;
    ASSERT_NO_THROW(proc.processCORCpolicyFromString(policy));
    auto actual = nlohmann::json::parse(actualJson);
    EXPECT_TRUE(actual.at("shaAllowList").empty());
}

TEST_F(TestPolicyProcessor_CORC_policy, processCorcPolicyWithNoPathTypeAllowListItems)
{
    std::string policy = R"(<?xml version="1.0"?>
        <policy RevID="revisionid" policyType="37">
        <whitelist>
            <item type="sha256">42268ef08462e645678ce738bd26518bc170a0404a186062e8b1bec2dc578673</item>
            <item type="cert-signer">SignerName</item>
        </whitelist>
        </policy>)";

    expectConstructorCalls();
    std::string actualJson;
    auto expectedJsonFragment = HasSubstr(R"sophos("pathAllowList":[])sophos");
    saveSusiConfigFromWrite(expectedJsonFragment, actualJson);

    Tests::ScopedReplaceFileSystem replacer(std::move(m_mockIFileSystemPtr));
    PolicyProcessorUnitTestClass proc;
    ASSERT_NO_THROW(proc.processCORCpolicyFromString(policy));
    auto actual = nlohmann::json::parse(actualJson);
    EXPECT_TRUE(actual.at("pathAllowList").empty());
}

TEST_F(TestPolicyProcessor_CORC_policy, processCorcPolicyWithOneEmptyPathTypeAllowListItem)
{
    std::string policy = R"(<?xml version="1.0"?>
        <policy RevID="revisionid" policyType="37">
        <whitelist>
            <item type="path">/tmp/a/path</item>
            <item type="path"></item>
            <item type="sha256">42268ef08462e645678ce738bd26518bc170a0404a186062e8b1bec2dc578673</item>
            <item type="cert-signer">SignerName</item>
        </whitelist>
        </policy>)";

    expectConstructorCalls();
    std::string actualJson;
    auto expectedJsonFragment = HasSubstr(R"sophos("pathAllowList":["/tmp/a/path"])sophos");
    saveSusiConfigFromWrite(expectedJsonFragment, actualJson);

    Tests::ScopedReplaceFileSystem replacer(std::move(m_mockIFileSystemPtr));
    PolicyProcessorUnitTestClass proc;
    ASSERT_NO_THROW(proc.processCORCpolicyFromString(policy));
    auto actual = nlohmann::json::parse(actualJson);
    ASSERT_EQ(actual.at("pathAllowList").size(), 1);
    EXPECT_EQ(actual.at("pathAllowList")[0], "/tmp/a/path");
}

TEST_F(TestPolicyProcessor_CORC_policy, processCorcPolicyWithOneInvalidPathTypeAllowListItem)
{
    std::string policy = R"(<?xml version="1.0"?>
        <policy RevID="revisionid" policyType="37">
        <whitelist>
            <item type="path">/tmp/a/path</item>
            <item type="path">not/a/absolute/path</item>
            <item type="path">*/not/a/absolute/path/but/am/glob</item>
            <item type="sha256">42268ef08462e645678ce738bd26518bc170a0404a186062e8b1bec2dc578673</item>
            <item type="cert-signer">SignerName</item>
        </whitelist>
        </policy>)";

    expectConstructorCalls();
    std::string actualJson;
    auto expectedJsonFragment = HasSubstr(R"sophos("pathAllowList":["/tmp/a/path","*/not/a/absolute/path/but/am/glob"])sophos");
    saveSusiConfigFromWrite(expectedJsonFragment, actualJson);

    Tests::ScopedReplaceFileSystem replacer(std::move(m_mockIFileSystemPtr));
    PolicyProcessorUnitTestClass proc;
    ASSERT_NO_THROW(proc.processCORCpolicyFromString(policy));
    auto actual = nlohmann::json::parse(actualJson);
    ASSERT_EQ(actual.at("pathAllowList").size(), 2);
    EXPECT_EQ(actual.at("pathAllowList")[0], "/tmp/a/path");
    EXPECT_EQ(actual.at("pathAllowList")[1], "*/not/a/absolute/path/but/am/glob");
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

    expectConstructorCalls();
    std::string actualJson;
    auto expectedJsonFragment = HasSubstr(R"sophos("shaAllowList":["42268ef08462e645678ce738bd26518bc170a0404a186062e8b1bec2dc578673"])sophos");
    saveSusiConfigFromWrite(expectedJsonFragment, actualJson);

    Tests::ScopedReplaceFileSystem replacer(std::move(m_mockIFileSystemPtr));
    PolicyProcessorUnitTestClass proc;
    ASSERT_NO_THROW(proc.processCORCpolicyFromString(policy));
    auto actual = nlohmann::json::parse(actualJson);
    ASSERT_EQ(actual.at("shaAllowList").size(), 1);
    EXPECT_EQ(actual.at("shaAllowList")[0], "42268ef08462e645678ce738bd26518bc170a0404a186062e8b1bec2dc578673");
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

    expectConstructorCalls();
    std::string actualJson;
    auto expectedJsonFragment = HasSubstr(R"sophos("shaAllowList":["42268ef08462e645678ce738bd26518bc170a0404a186062e8b1bec2dc578673"])sophos");
    saveSusiConfigFromWrite(expectedJsonFragment, actualJson);

    Tests::ScopedReplaceFileSystem replacer(std::move(m_mockIFileSystemPtr));
    PolicyProcessorUnitTestClass proc;
    ASSERT_NO_THROW(proc.processCORCpolicyFromString(policy));
    auto actual = nlohmann::json::parse(actualJson);
    ASSERT_EQ(actual.at("shaAllowList").size(), 1);
    EXPECT_EQ(actual.at("shaAllowList")[0], "42268ef08462e645678ce738bd26518bc170a0404a186062e8b1bec2dc578673");
    ASSERT_EQ(actual.at("pathAllowList").size(), 1);
    EXPECT_EQ(actual.at("pathAllowList")[0], "/tmp/a/path");
}


