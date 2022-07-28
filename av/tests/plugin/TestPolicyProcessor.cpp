/******************************************************************************************************

Copyright 2020-2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

# define SLEEP_TIME 0

#include <Common/Helpers/MockFileSystem.h>
#include <Common/Helpers/FileSystemReplaceAndRestore.h>
#include "PluginMemoryAppenderUsingTests.h"

#include "datatypes/sophos_filesystem.h"

#include <Common/FileSystem/IFileSystemException.h>
#include <gtest/gtest.h>
#include <pluginimpl/PolicyProcessor.h>

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
            m_soapConfigPath = m_testDir / "var/soapd_config.json";
            m_susiStartupConfigChrootPath = std::string(m_testDir / "chroot") + m_susiStartupConfigPath;
            m_mockIFileSystemPtr = std::make_unique<StrictMock<MockFileSystem>>();
        }

        void TearDown() override
        {
            fs::remove_all(m_testDir);
        }
        
        std::string m_susiStartupConfigPath;
        std::string m_susiStartupConfigChrootPath;
        std::string m_soapConfigPath;
        std::unique_ptr<StrictMock<MockFileSystem>> m_mockIFileSystemPtr;
    };

    class PolicyProcessorUnitTestClass : public Plugin::PolicyProcessor
    {
    protected:
        void notifyOnAccessProcess() override
        {
            PRINT("Notified soapd");
        }
    };
}

const std::string FULL_POLICY //NOLINT
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


std::string GL_POLICY_2 =  // NOLINT
    R"sophos(<?xml version="1.0"?>
<AUConfigurations>
  <AUConfig>
    <primary_location>
      <server Algorithm="Clear" UserPassword="A" UserName="B"/>
    </primary_location>
  </AUConfig>
</AUConfigurations>
)sophos";

TEST_F(TestPolicyProcessor, getCustomerIdFromAttributeMap) // NOLINT
{
    std::string policyXml = FULL_POLICY;

    auto attributeMap = Common::XmlUtilities::parseXml(policyXml);
    auto customerId = Plugin::PolicyProcessor::getCustomerId(attributeMap);
    EXPECT_EQ(customerId, "5e259db8da3ae4df8f18a2add2d3d47d");
}


TEST_F(TestPolicyProcessor, getCustomerIdFromMinimalAttributeMap) // NOLINT
{
    std::string policyXml = R"sophos(<?xml version="1.0"?>
<AUConfigurations>
  <AUConfig>
    <primary_location>
      <server Algorithm="AES256" UserPassword="CCD8CFFX8bdCDFtU0+hv6MvL3FoxA0YeSNjJyZJWxz1b3uTsBu5p8GJqsfp3SAByOZw=" UserName="CSP201127101443"/>
    </primary_location>
  </AUConfig>
</AUConfigurations>
)sophos";

    auto attributeMap = Common::XmlUtilities::parseXml(policyXml);
    auto customerId = Plugin::PolicyProcessor::getCustomerId(attributeMap);
    EXPECT_EQ(customerId, "5e259db8da3ae4df8f18a2add2d3d47d");
}


TEST_F(TestPolicyProcessor, getCustomerIdFromClearAttributeMap) // NOLINT
{
    std::string policyXml = GL_POLICY_2;

    auto attributeMap = Common::XmlUtilities::parseXml(policyXml);
    auto customerId = Plugin::PolicyProcessor::getCustomerId(attributeMap);
    EXPECT_EQ(customerId, "a1c0f318e58aad6bf90d07cabda54b7d");
}

TEST_F(TestPolicyProcessor, processAlcPolicyNoChangePolicy) // NOLINT
{
    const std::string expectedMd5 = "5e259db8da3ae4df8f18a2add2d3d47d";
    const std::string customerIdFilePath1 = m_testDir / "var/customer_id.txt";
    const std::string customerIdFilePath2 = std::string(m_testDir / "chroot") + customerIdFilePath1;
    EXPECT_CALL(*m_mockIFileSystemPtr, readFile(customerIdFilePath1)).WillOnce(Return(expectedMd5));

    Tests::ScopedReplaceFileSystem replacer(std::move(m_mockIFileSystemPtr));

    Plugin::PolicyProcessor proc;

    auto attributeMap = Common::XmlUtilities::parseXml(FULL_POLICY);
    bool changed = proc.processAlcPolicy(attributeMap);
    EXPECT_FALSE(changed);

    changed = proc.processAlcPolicy(attributeMap);
    EXPECT_FALSE(changed);
}


TEST_F(TestPolicyProcessor, processAlcPolicyChangedPolicy) // NOLINT
{
    const std::string expectedMd5_1 = "5e259db8da3ae4df8f18a2add2d3d47d";
    const std::string expectedMd5_2 = "a1c0f318e58aad6bf90d07cabda54b7d";
    const std::string customerIdFilePath1 = m_testDir / "var/customer_id.txt";
    const std::string customerIdFilePath2 = std::string(m_testDir / "chroot") + customerIdFilePath1;
    Common::FileSystem::IFileSystemException ex("Error, Failed to read file: '" + customerIdFilePath1 + "', file does not exist");
    EXPECT_CALL(*m_mockIFileSystemPtr, readFile(customerIdFilePath1)).WillOnce(Throw(ex));
    EXPECT_CALL(*m_mockIFileSystemPtr, writeFile(customerIdFilePath1, expectedMd5_1)).Times(1);
    EXPECT_CALL(*m_mockIFileSystemPtr, writeFile(customerIdFilePath2, expectedMd5_1)).Times(1);
    EXPECT_CALL(*m_mockIFileSystemPtr, writeFile(customerIdFilePath1, expectedMd5_2)).Times(1);
    EXPECT_CALL(*m_mockIFileSystemPtr, writeFile(customerIdFilePath2, expectedMd5_2)).Times(1);

    Tests::ScopedReplaceFileSystem replacer(std::move(m_mockIFileSystemPtr));

    Plugin::PolicyProcessor proc;

    auto attributeMap = Common::XmlUtilities::parseXml(FULL_POLICY);
    bool changed = proc.processAlcPolicy(attributeMap);
    EXPECT_TRUE(changed);

    attributeMap = Common::XmlUtilities::parseXml(GL_POLICY_2);
    changed = proc.processAlcPolicy(attributeMap);
    EXPECT_TRUE(changed);
}


TEST_F(TestPolicyProcessor, getCustomerIdFromEmptyAttributeMap) // NOLINT
{
    std::string policyXml = R"sophos(<?xml version="1.0"?>
<AUConfigurations>
  <AUConfig>
    <primary_location>
      <server />
    </primary_location>
  </AUConfig>
</AUConfigurations>
)sophos";

    auto attributeMap = Common::XmlUtilities::parseXml(policyXml);
    auto customerId = Plugin::PolicyProcessor::getCustomerId(attributeMap);
    EXPECT_EQ(customerId, "");
}

TEST_F(TestPolicyProcessor, getCustomerIdFromAttributeMapNoAlgorithm) // NOLINT
{
    std::string policyXml = R"sophos(<?xml version="1.0"?>
<AUConfigurations>
  <AUConfig>
    <primary_location>
      <server UserPassword="A" UserName="B"/>
    </primary_location>
  </AUConfig>
</AUConfigurations>
)sophos";

    auto attributeMap = Common::XmlUtilities::parseXml(policyXml);
    auto customerId = Plugin::PolicyProcessor::getCustomerId(attributeMap);
    EXPECT_EQ(customerId, "a1c0f318e58aad6bf90d07cabda54b7d");
}

TEST_F(TestPolicyProcessor, getCustomerIdFromAttributeMapHashed) // NOLINT
{
    std::string policyXml = R"sophos(<?xml version="1.0"?>
<AUConfigurations>
  <AUConfig>
    <primary_location>
      <server UserPassword="f882bf58515d6cf238ce0d38e16a35e7" UserName="f882bf58515d6cf238ce0d38e16a35e7"/>
    </primary_location>
  </AUConfig>
</AUConfigurations>
)sophos";

    auto attributeMap = Common::XmlUtilities::parseXml(policyXml);
    auto customerId = Plugin::PolicyProcessor::getCustomerId(attributeMap);
    EXPECT_EQ(customerId, "a1c0f318e58aad6bf90d07cabda54b7d");
}


TEST_F(TestPolicyProcessor, getCustomerIdFromAttributeMapNotHashed) // NOLINT
{
    std::string policyXml = R"sophos(<?xml version="1.0"?>
<AUConfigurations>
  <AUConfig>
    <primary_location>
      <server UserPassword="AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA" UserName="BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB"/>
    </primary_location>
  </AUConfig>
</AUConfigurations>
)sophos";

    auto attributeMap = Common::XmlUtilities::parseXml(policyXml);
    auto customerId = Plugin::PolicyProcessor::getCustomerId(attributeMap);
    EXPECT_EQ(customerId, "9f8a273ce2a45c1c20eae4f9163a3f75");
}


TEST_F(TestPolicyProcessor, getCustomerIdFromAttributeMapEmptyPassword) // NOLINT
{
    std::string policyXml = R"sophos(<?xml version="1.0"?>
<AUConfigurations>
  <AUConfig>
    <primary_location>
      <server UserPassword="" UserName="B"/>
    </primary_location>
  </AUConfig>
</AUConfigurations>
)sophos";

    auto attributeMap = Common::XmlUtilities::parseXml(policyXml);
    try
    {
        auto customerId = Plugin::PolicyProcessor::getCustomerId(attributeMap);
        FAIL() << "getCustomerId didn't throw";
    }
    catch (std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Invalid policy: Password is empty ");
    }
}


TEST_F(TestPolicyProcessor, getCustomerIdFromAttributeMapMissingPassword) // NOLINT
{
    std::string policyXml = R"sophos(<?xml version="1.0"?>
<AUConfigurations>
  <AUConfig>
    <primary_location>
      <server UserName="B"/>
    </primary_location>
  </AUConfig>
</AUConfigurations>
)sophos";

    auto attributeMap = Common::XmlUtilities::parseXml(policyXml);
    try
    {
        auto customerId = Plugin::PolicyProcessor::getCustomerId(attributeMap);
        FAIL() << "getCustomerId didn't throw";
    }
    catch (std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Invalid policy: Password is empty ");
    }
}


TEST_F(TestPolicyProcessor, getCustomerIdFromAttributeMapEmptyUserName) // NOLINT
{
    std::string policyXml = R"sophos(<?xml version="1.0"?>
<AUConfigurations>
  <AUConfig>
    <primary_location>
      <server UserPassword="A" UserName=""/>
    </primary_location>
  </AUConfig>
</AUConfigurations>
)sophos";

    auto attributeMap = Common::XmlUtilities::parseXml(policyXml);
    try
    {
        auto customerId = Plugin::PolicyProcessor::getCustomerId(attributeMap);
        FAIL() << "getCustomerId didn't throw";
    }
    catch (std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Invalid policy: Username is empty ");
    }
}

TEST_F(TestPolicyProcessor, getCustomerIdFromAttributeMapMissingUserName) // NOLINT
{
    std::string policyXml = R"sophos(<?xml version="1.0"?>
<AUConfigurations>
  <AUConfig>
    <primary_location>
      <server UserPassword="A"/>
    </primary_location>
  </AUConfig>
</AUConfigurations>
)sophos";

    auto attributeMap = Common::XmlUtilities::parseXml(policyXml);
    try
    {
        auto customerId = Plugin::PolicyProcessor::getCustomerId(attributeMap);
        FAIL() << "getCustomerId didn't throw";
    }
    catch (std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Invalid policy: Username is empty ");
    }
}


TEST_F(TestPolicyProcessor, getCustomerIdFromAttributeMapInvalidAlgorithm) // NOLINT
{
    std::string policyXml = R"sophos(<?xml version="1.0"?>
<AUConfigurations>
  <AUConfig>
    <primary_location>
      <server Algorithm="3DES" UserPassword="A" UserName="B"/>
    </primary_location>
  </AUConfig>
</AUConfigurations>
)sophos";

    auto attributeMap = Common::XmlUtilities::parseXml(policyXml);
    try
    {
        auto customerId = Plugin::PolicyProcessor::getCustomerId(attributeMap);
        FAIL() << "getCustomerId didn't throw";
    }
    catch (std::invalid_argument& e)
    {
        EXPECT_STREQ(e.what(), "Invalid policy: Unknown obfuscation algorithm");
    }
}


TEST_F(TestPolicyProcessor, getCustomerIdFromBlankAttributeMap) // NOLINT
{
    std::string policyXml {};

    try
    {
        auto attributeMap = Common::XmlUtilities::parseXml(policyXml);
        FAIL() << "parseXml didn't throw";
    }
    catch (std::exception& e)
    {
        EXPECT_THAT(e.what(), HasSubstr("Error parsing xml"));
    }
}


TEST_F(TestPolicyProcessor, processAlcPolicyInvalid) // NOLINT
{
    const std::string expectedMd5 = "5e259db8da3ae4df8f18a2add2d3d47d";
    EXPECT_CALL(*m_mockIFileSystemPtr, readFile(_)).WillOnce(Return(expectedMd5));

    Tests::ScopedReplaceFileSystem replacer(std::move(m_mockIFileSystemPtr));

    Plugin::PolicyProcessor proc;

    std::string policyXml = R"sophos(<?xml version="1.0"?>
<AUConfigurations>
  <AUConfig>
    <primary_location>
      <server />
    </primary_location>
  </AUConfig>
</AUConfigurations>
)sophos";

    auto attributeMap = Common::XmlUtilities::parseXml(policyXml);
    bool changed = proc.processAlcPolicy(attributeMap);
    EXPECT_FALSE(changed);
}

TEST_F(TestPolicyProcessor, processSavPolicy) // NOLINT
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
    EXPECT_TRUE(proc.processSavPolicy(attributeMap));
}

TEST_F(TestPolicyProcessor, defaultSXL4lookupValueIsTrue) // NOLINT
{
    Plugin::PolicyProcessor proc;
    EXPECT_TRUE(proc.getSXL4LookupsEnabled());
}

TEST_F(TestPolicyProcessor, processSavPolicyChanged) // NOLINT
{
    EXPECT_CALL(*m_mockIFileSystemPtr, writeFileAtomically(m_soapConfigPath,
                                                           R"sophos({"enabled":"false","excludeRemoteFiles":"false","exclusions":[]})sophos",
                                                           _,
                                                           0640)).Times(2);

    EXPECT_CALL(*m_mockIFileSystemPtr, readFile(_)).WillRepeatedly(Return(""));
    {
        InSequence seq;

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

    EXPECT_FALSE(proc.processSavPolicy(attributeMapTrue));
    EXPECT_TRUE(proc.getSXL4LookupsEnabled());

    EXPECT_TRUE(proc.processSavPolicy(attributeMapFalse));
    EXPECT_FALSE(proc.getSXL4LookupsEnabled());

    EXPECT_TRUE(proc.processSavPolicy(attributeMapTrue));
    EXPECT_TRUE(proc.getSXL4LookupsEnabled());
}

TEST_F(TestPolicyProcessor, processSavPolicyMissing) // NOLINT
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
    bool changed = proc.processSavPolicy(attributeMapInvalid);
    EXPECT_TRUE(changed);
}

TEST_F(TestPolicyProcessor, processSavPolicyInvalid) // NOLINT
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
    bool changed = proc.processSavPolicy(attributeMapInvalid);
    EXPECT_TRUE(changed);
}

TEST_F(TestPolicyProcessor, processOnAccessPolicy) // NOLINT
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

    proc.processOnAccessPolicy(attributeMap);
}

TEST_F(TestPolicyProcessor, processInvalidOnAccessPolicy) // NOLINT
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

    proc.processOnAccessPolicy(attributeMap);
}