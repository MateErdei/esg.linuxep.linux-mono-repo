// Copyright 2020-2023 Sophos Limited. All rights reserved.

#include "TestPolicyProcessor.h"

#include "Common/Helpers/FileSystemReplaceAndRestore.h"
#include "Common/FileSystem/IFileSystemException.h"
#include "Common/UtilityImpl/StringUtils.h"

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
    <item type="posix-path">/tmp/a/path</item>
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
            <item type="posix-path">/tmp/a/path</item>
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
            <item type="posix-path">/tmp/a/path</item>
            <item type="posix-path"></item>
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
            <item type="posix-path">/tmp/a/path</item>
            <item type="posix-path">not/a/absolute/path</item>
            <item type="posix-path">*/not/a/absolute/path/but/am/glob</item>
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
            <item type="posix-path">/tmp/a/path</item>
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
            <item type="posix-path">/tmp/a/path</item>
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

TEST_F(TestPolicyProcessor_CORC_policy, processCorcPolicyWithWindowsAllowPath)
{
    std::string policy = R"(<?xml version="1.0"?>
        <policy RevID="revisionid" policyType="37">
        <whitelist>
            <item type="posix-path">/tmp/a/path</item>
            <item type="path">/im/for/windows</item>
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

// SXL 4 URL tests:

namespace
{
    std::string getPolicyForSXL4Url(const std::string& sxlUrl)
    {
        const std::string policyTemplate = R"(<?xml version="1.0"?>
        <policy RevID="revisionid" policyType="37">
            <detectionFeedback>
              <sendData>true</sendData>
            </detectionFeedback>
            <intelix>
              <lookupUrl>{{SXL4_URL}}</lookupUrl>
            </intelix>
        </policy>)";
        return Common::UtilityImpl::StringUtils::orderedStringReplace(policyTemplate, { { "{{SXL4_URL}}", sxlUrl } });
    }
}

TEST_F(TestPolicyProcessor_CORC_policy, valid_sxl4_url)
{
    const auto policy = getPolicyForSXL4Url("https://4.sophosxl.net");
    expectConstructorCalls();
    std::string actualJson;
    auto expectedJsonFragment = HasSubstr(R"sophos("sxlUrl":"https://4.sophosxl.net/lookup")sophos");
    saveSusiConfigFromWrite(expectedJsonFragment, actualJson);

    Tests::ScopedReplaceFileSystem replacer(std::move(m_mockIFileSystemPtr));
    PolicyProcessorUnitTestClass proc;
    ASSERT_NO_THROW(proc.processCORCpolicyFromString(policy));
    auto actual = nlohmann::json::parse(actualJson);
    EXPECT_EQ(actual.at("sxlUrl"), "https://4.sophosxl.net/lookup");
    EXPECT_TRUE(proc.restartThreatDetector());
}

TEST_F(TestPolicyProcessor_CORC_policy, sxl4_url_all_alpha_numeric)
{
    const auto policy = getPolicyForSXL4Url("https://ABCDEFGHIJKLMNOPQRSTUVWXYZ.abcdefghijklmnopqrstuvwxyz.0123456789.com");

    expectConstructorCalls();
    std::string actualJson;
    auto expectedJsonFragment = HasSubstr(R"sophos("sxlUrl":)sophos");
    saveSusiConfigFromWrite(expectedJsonFragment, actualJson);

    Tests::ScopedReplaceFileSystem replacer(std::move(m_mockIFileSystemPtr));
    PolicyProcessorUnitTestClass proc;
    ASSERT_NO_THROW(proc.processCORCpolicyFromString(policy));
    auto actual = nlohmann::json::parse(actualJson);
    EXPECT_EQ(actual.at("sxlUrl"), "https://ABCDEFGHIJKLMNOPQRSTUVWXYZ.abcdefghijklmnopqrstuvwxyz.0123456789.com/lookup");
}

TEST_F(TestPolicyProcessor_CORC_policy, missing_sxl4_url_goes_to_empty)
{
    const std::string policy = R"(<?xml version="1.0"?>
        <policy RevID="revisionid" policyType="37">
        </policy>)";

    expectConstructorCalls();
    std::string actualJson;
    auto expectedJsonFragment = HasSubstr(R"sophos("sxlUrl")sophos");
    saveSusiConfigFromWrite(expectedJsonFragment, actualJson);

    Tests::ScopedReplaceFileSystem replacer(std::move(m_mockIFileSystemPtr));
    PolicyProcessorUnitTestClass proc;
    ASSERT_NO_THROW(proc.processCORCpolicyFromString(policy));
    auto actual = nlohmann::json::parse(actualJson);
    EXPECT_EQ(actual.at("sxlUrl"), "");
}

TEST_F(TestPolicyProcessor_CORC_policy, empty_sxl4_url_goes_to_empty)
{
    const auto policy = getPolicyForSXL4Url("");

    expectConstructorCalls();
    std::string actualJson;
    auto expectedJsonFragment = HasSubstr(R"sophos("sxlUrl")sophos");
    saveSusiConfigFromWrite(expectedJsonFragment, actualJson);

    Tests::ScopedReplaceFileSystem replacer(std::move(m_mockIFileSystemPtr));
    PolicyProcessorUnitTestClass proc;
    ASSERT_NO_THROW(proc.processCORCpolicyFromString(policy));
    auto actual = nlohmann::json::parse(actualJson);
    EXPECT_EQ(actual.at("sxlUrl"), "");
}

TEST_F(TestPolicyProcessor_CORC_policy, invalid_utf8_in_sxl4_url)
{
    // echo -n "ありったけの夢をかき集め" | iconv -f utf-8 -t sjis | hexdump -C
    std::vector<unsigned char> sjisBytes { 0x82, 0xa0, 0x82, 0xe8, 0x82, 0xc1, 0x82, 0xbd, 0x82, 0xaf, 0x82, 0xcc,
                                                0x96, 0xb2, 0x82, 0xf0, 0x82, 0xa9, 0x82, 0xab, 0x8f, 0x57, 0x82, 0xdf };
    std::string sjis(sjisBytes.begin(), sjisBytes.end());
    std::string url = "https://" + sjis;
    const auto policy = getPolicyForSXL4Url(url);

    expectConstructorCalls();
    std::string actualJson;
    // Don't actually expect any writes!
    ON_CALL(*m_mockIFileSystemPtr, writeFileAtomically(m_susiStartupConfigPath, _,_ ,_)).WillByDefault(SaveArg<1>(&actualJson));

    Tests::ScopedReplaceFileSystem replacer(std::move(m_mockIFileSystemPtr));
    PolicyProcessorUnitTestClass proc;
    try
    {
        proc.processCORCpolicyFromString(policy);
        // If the XML gets parsed then expect empty URL
        auto actual = nlohmann::json::parse(actualJson);
        EXPECT_EQ(actual.at("sxlUrl"), "");
    }
    catch (const Common::XmlUtilities::XmlUtilitiesException&)
    {
        // Expect the XML parsing to fail
    }
}

TEST_F(TestPolicyProcessor_CORC_policy, sxl4_url_must_be_https)
{
    const auto policy = getPolicyForSXL4Url("http://4.sophosxl.net");

    expectConstructorCalls();
    std::string actualJson;
    auto expectedJsonFragment = HasSubstr(R"sophos("sxlUrl":"")sophos");
    saveSusiConfigFromWrite(expectedJsonFragment, actualJson);

    Tests::ScopedReplaceFileSystem replacer(std::move(m_mockIFileSystemPtr));
    PolicyProcessorUnitTestClass proc;
    ASSERT_NO_THROW(proc.processCORCpolicyFromString(policy));
    auto actual = nlohmann::json::parse(actualJson);
    EXPECT_EQ(actual.at("sxlUrl"), "");
}

TEST_F(TestPolicyProcessor_CORC_policy, sxl4_url_not_alpha_numeric)
{
    const auto policy = getPolicyForSXL4Url("https://4.sxl@@@.net");

    expectConstructorCalls();
    std::string actualJson;
    auto expectedJsonFragment = HasSubstr(R"sophos("sxlUrl":"")sophos");
    saveSusiConfigFromWrite(expectedJsonFragment, actualJson);

    Tests::ScopedReplaceFileSystem replacer(std::move(m_mockIFileSystemPtr));
    PolicyProcessorUnitTestClass proc;
    ASSERT_NO_THROW(proc.processCORCpolicyFromString(policy));
    auto actual = nlohmann::json::parse(actualJson);
    EXPECT_EQ(actual.at("sxlUrl"), "");
}

TEST_F(TestPolicyProcessor_CORC_policy, sxl4_url_can_not_be_too_long)
{
    auto url = "https://" + std::string(1025, 'a');
    const auto policy = getPolicyForSXL4Url(url);

    expectConstructorCalls();
    std::string actualJson;
    auto expectedJsonFragment = HasSubstr(R"sophos("sxlUrl":"")sophos");
    saveSusiConfigFromWrite(expectedJsonFragment, actualJson);

    Tests::ScopedReplaceFileSystem replacer(std::move(m_mockIFileSystemPtr));
    PolicyProcessorUnitTestClass proc;
    ASSERT_NO_THROW(proc.processCORCpolicyFromString(policy));
    auto actual = nlohmann::json::parse(actualJson);
    EXPECT_EQ(actual.at("sxlUrl"), "");
}

// SXL Enable
#ifdef USE_SXL_ENABLE_FROM_CORC_POLICY

namespace
{
    std::string getPolicyForSXL4Enable(bool sxlEnabled)
    {
        const std::string policyTemplate = R"(<?xml version="1.0"?>
        <policy RevID="revisionid" policyType="37">
            <detectionFeedback>
              <sendData>{{SXL4}}</sendData>
              <sendFiles>false</sendFiles>
              <onDemandEnable>true</onDemandEnable>
            </detectionFeedback>
            <intelix>
                <lookupUrl>https://4.sxlsophos.net</lookupUrl>
            </intelix>
        </policy>)";
        return Common::UtilityImpl::StringUtils::orderedStringReplace(policyTemplate, { { "{{SXL4}}", sxlEnabled ? "true" : "false" } });
    }
}

TEST_F(TestPolicyProcessor_CORC_policy, sxl4_enabled)
{
    const auto policy = getPolicyForSXL4Enable(true);

    expectConstructorCalls();
    std::string actualJson;
    saveSusiConfigFromWrite(_, actualJson);

    Tests::ScopedReplaceFileSystem replacer(std::move(m_mockIFileSystemPtr));
    PolicyProcessorUnitTestClass proc;
    ASSERT_NO_THROW(proc.processCORCpolicyFromString(policy));
    auto actual = nlohmann::json::parse(actualJson);
    EXPECT_TRUE(actual.at("enableSxlLookup"));
    EXPECT_EQ(proc.restartThreatDetector(), !common::ThreatDetector::SXL_DEFAULT);
}

TEST_F(TestPolicyProcessor_CORC_policy, sxl4_disabled)
{
    const auto policy = getPolicyForSXL4Enable(false);

    expectConstructorCalls();
    std::string actualJson;
    saveSusiConfigFromWrite(_, actualJson);

    Tests::ScopedReplaceFileSystem replacer(std::move(m_mockIFileSystemPtr));
    PolicyProcessorUnitTestClass proc;
    ASSERT_NO_THROW(proc.processCORCpolicyFromString(policy));
    auto actual = nlohmann::json::parse(actualJson);
    EXPECT_FALSE(actual.at("enableSxlLookup"));
    EXPECT_EQ(proc.restartThreatDetector(), common::ThreatDetector::SXL_DEFAULT);
}

#endif /* USE_SXL_ENABLE_FROM_CORC_POLICY */

