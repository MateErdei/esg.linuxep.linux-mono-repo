/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "PluginMemoryAppenderUsingTests.h"
#include "MockFileSystem.h"

#include "datatypes/sophos_filesystem.h"

#include <pluginimpl/AlcPolicyProcessor.h>

#include <gtest/gtest.h>


namespace fs = sophos_filesystem;

namespace
{
    class TestAlcPolicyProcessor : public PluginMemoryAppenderUsingTests
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
        }

        void TearDown() override
        {
            fs::remove_all(m_testDir);
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

TEST_F(TestAlcPolicyProcessor, getCustomerIdFromAttributeMap) // NOLINT
{
    std::string policyXml = FULL_POLICY;

    auto attributeMap = Common::XmlUtilities::parseXml(policyXml);
    auto customerId = Plugin::AlcPolicyProcessor::getCustomerId(attributeMap);
    EXPECT_EQ(customerId, "5e259db8da3ae4df8f18a2add2d3d47d");
}


TEST_F(TestAlcPolicyProcessor, getCustomerIdFromMinimalAttributeMap) // NOLINT
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
    auto customerId = Plugin::AlcPolicyProcessor::getCustomerId(attributeMap);
    EXPECT_EQ(customerId, "5e259db8da3ae4df8f18a2add2d3d47d");
}


TEST_F(TestAlcPolicyProcessor, getCustomerIdFromClearAttributeMap) // NOLINT
{
    std::string policyXml = GL_POLICY_2;

    auto attributeMap = Common::XmlUtilities::parseXml(policyXml);
    auto customerId = Plugin::AlcPolicyProcessor::getCustomerId(attributeMap);
    EXPECT_EQ(customerId, "a1c0f318e58aad6bf90d07cabda54b7d");
}

TEST_F(TestAlcPolicyProcessor, processAlcPolicy) // NOLINT
{
    std::string policyXml = FULL_POLICY;
    Plugin::AlcPolicyProcessor proc;

    // Setup Mock filesystem
    auto mockIFileSystemPtr = std::make_unique<StrictMock<MockFileSystem>>();

    const std::string expectedMd5 = "5e259db8da3ae4df8f18a2add2d3d47d";
    const std::string customerIdFilePath1 = m_testDir / "var/customer_id.txt";
    const std::string customerIdFilePath2 = std::string(m_testDir / "chroot") + customerIdFilePath1;
    EXPECT_CALL(*mockIFileSystemPtr, writeFile(customerIdFilePath1, expectedMd5)).Times(1);
    EXPECT_CALL(*mockIFileSystemPtr, writeFile(customerIdFilePath2, expectedMd5)).Times(1);

    Tests::ScopedReplaceFileSystem replacer(std::move(mockIFileSystemPtr));

    auto attributeMap = Common::XmlUtilities::parseXml(policyXml);
    bool changed = proc.processAlcPolicy(attributeMap);
    EXPECT_FALSE(changed); // First time doesn't count as a change

    changed = proc.processAlcPolicy(attributeMap);
    EXPECT_FALSE(changed);
}


TEST_F(TestAlcPolicyProcessor, processAlcPolicyReturnsUpdate) // NOLINT
{
    Plugin::AlcPolicyProcessor proc;

    // Setup Mock filesystem
    auto mockIFileSystemPtr = std::make_unique<StrictMock<MockFileSystem>>();

    const std::string expectedMd5_1 = "5e259db8da3ae4df8f18a2add2d3d47d";
    const std::string expectedMd5_2 = "a1c0f318e58aad6bf90d07cabda54b7d";
    const std::string customerIdFilePath1 = m_testDir / "var/customer_id.txt";
    const std::string customerIdFilePath2 = std::string(m_testDir / "chroot") + customerIdFilePath1;
    EXPECT_CALL(*mockIFileSystemPtr, writeFile(customerIdFilePath1, expectedMd5_1)).Times(1);
    EXPECT_CALL(*mockIFileSystemPtr, writeFile(customerIdFilePath2, expectedMd5_1)).Times(1);
    EXPECT_CALL(*mockIFileSystemPtr, writeFile(customerIdFilePath1, expectedMd5_2)).Times(1);
    EXPECT_CALL(*mockIFileSystemPtr, writeFile(customerIdFilePath2, expectedMd5_2)).Times(1);

    Tests::ScopedReplaceFileSystem replacer(std::move(mockIFileSystemPtr));

    auto attributeMap = Common::XmlUtilities::parseXml(FULL_POLICY);
    bool changed = proc.processAlcPolicy(attributeMap);
    EXPECT_FALSE(changed); // First time doesn't count as a change

    attributeMap = Common::XmlUtilities::parseXml(GL_POLICY_2);
    changed = proc.processAlcPolicy(attributeMap);
    EXPECT_TRUE(changed);
}

