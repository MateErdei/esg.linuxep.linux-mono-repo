// Copyright 2020-2023 Sophos Limited. All rights reserved.

#include "TestPolicyProcessor.h"

#include "datatypes/sophos_filesystem.h"

#include "Common/Helpers/FileSystemReplaceAndRestore.h"
#include "Common/FileSystem/IFileSystemException.h"

#include <gtest/gtest.h>

namespace fs = sophos_filesystem;

namespace
{
    class TestPolicyProcessor_ALC_policy : public TestPolicyProcessorBase
    {
    protected:
        void SetUp() override
        {
            setupBase();
        }

        template<typename Matcher>
        void expectWriteCustomerID(const Matcher& id)
        {
            assert(m_mockIFileSystemPtr);
            const std::string customerIdFilePath1 = m_customerIdPath;
            const std::string customerIdFilePath2 = std::string(m_testDir / "chroot") + customerIdFilePath1;
            EXPECT_CALL(*m_mockIFileSystemPtr, writeFile(customerIdFilePath1, id)).Times(1);
            EXPECT_CALL(*m_mockIFileSystemPtr, writeFile(customerIdFilePath2, id)).Times(1);
        }
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

static Common::XmlUtilities::AttributesMap parseFullPolicy()
{
    static auto map = Common::XmlUtilities::parseXml(ALC_FULL_POLICY);
    return map;
}

TEST_F(TestPolicyProcessor_ALC_policy, getCustomerIdFromAttributeMap)
{
    auto attributeMap = parseFullPolicy();
    auto customerId = Plugin::PolicyProcessor::getCustomerId(attributeMap);
    EXPECT_EQ(customerId, "5e259db8da3ae4df8f18a2add2d3d47d");
}

TEST_F(TestPolicyProcessor_ALC_policy, customerIDFromPlaceholders)
{
    const auto policyXml = R"sophos(<?xml version="1.0"?>
<AUConfigurations xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:csc="com.sophos\msys\csc" xmlns="http://www.sophos.com/EE/AUConfig">
    <csc:Comp RevID="thisIsAnUpgradeTestRevID" policyType="1"/>
    <AUConfig platform="Linux">
        <sophos_address address="http://es-web.sophos.com/update"/>
        <primary_location>
            <server BandwidthLimit="256" AutoDial="false" Algorithm="Clear" UserPassword="@PASSWORD@" UserName="@USERNAME@" UseSophos="true" UseHttps="true" UseDelta="true" ConnectionAddress="@CONNECTIONADDRESS@" AllowLocalConfig="false"/>
            <proxy ProxyType="0" ProxyUserPassword="" ProxyUserName="" ProxyPortNumber="0" ProxyAddress="" AllowLocalConfig="false"/>
        </primary_location>
    </AUConfig>
</AUConfigurations>)sophos";

    auto attributeMap = Common::XmlUtilities::parseXml(policyXml);
    auto customerId = Plugin::PolicyProcessor::getCustomerId(attributeMap);
    EXPECT_EQ(customerId, "391dd69f3899e6f978d466e7512f7ab3");
}


TEST_F(TestPolicyProcessor_ALC_policy, getCustomerIdFromMinimalAttributeMap)
{
    const std::string policyXml = R"sophos(<?xml version="1.0"?>
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


TEST_F(TestPolicyProcessor_ALC_policy, getCustomerIdFromClearAttributeMap)
{
    const std::string policyXml = GL_POLICY_2;

    auto attributeMap = Common::XmlUtilities::parseXml(policyXml);
    auto customerId = Plugin::PolicyProcessor::getCustomerId(attributeMap);
    EXPECT_EQ(customerId, "a1c0f318e58aad6bf90d07cabda54b7d");
}

TEST_F(TestPolicyProcessor_ALC_policy, processAlcPolicyNoChangePolicy)
{
    const std::string expectedMd5 = "5e259db8da3ae4df8f18a2add2d3d47d";
    const std::string customerIdFilePath1 = m_customerIdPath;
    EXPECT_CALL(*m_mockIFileSystemPtr, readFile(customerIdFilePath1)).WillOnce(Return(expectedMd5));
    expectAbsentSusiConfig();

    Tests::ScopedReplaceFileSystem replacer(std::move(m_mockIFileSystemPtr));

    Plugin::PolicyProcessor proc{nullptr};

    auto attributeMap = parseFullPolicy();
    proc.processAlcPolicy(attributeMap);
    EXPECT_FALSE(proc.restartThreatDetector());
    EXPECT_FALSE(proc.reloadThreatDetectorConfiguration());

    proc.processAlcPolicy(attributeMap);
    EXPECT_FALSE(proc.restartThreatDetector());
    EXPECT_FALSE(proc.reloadThreatDetectorConfiguration());
}


TEST_F(TestPolicyProcessor_ALC_policy, processAlcPolicyChangedPolicy)
{
    const std::string expectedMd5_1 = "5e259db8da3ae4df8f18a2add2d3d47d";
    const std::string expectedMd5_2 = "a1c0f318e58aad6bf90d07cabda54b7d";
    const std::string customerIdFilePath1 = m_customerIdPath;
    const std::string customerIdFilePath2 = std::string(m_testDir / "chroot") + customerIdFilePath1;
    Common::FileSystem::IFileSystemException ex("Error, Failed to read file: '" + customerIdFilePath1 + "', file does not exist");
    EXPECT_CALL(*m_mockIFileSystemPtr, readFile(customerIdFilePath1)).WillOnce(Throw(ex));
    expectWriteCustomerID(expectedMd5_1);
    expectWriteCustomerID(expectedMd5_2);
    expectAbsentSusiConfig();

    Tests::ScopedReplaceFileSystem replacer(std::move(m_mockIFileSystemPtr));

    Plugin::PolicyProcessor proc{nullptr};

    auto attributeMap = parseFullPolicy();
    proc.processAlcPolicy(attributeMap);
    EXPECT_FALSE(proc.restartThreatDetector());
    EXPECT_TRUE(proc.reloadThreatDetectorConfiguration());

    attributeMap = Common::XmlUtilities::parseXml(GL_POLICY_2);
    proc.processAlcPolicy(attributeMap);
    EXPECT_FALSE(proc.restartThreatDetector());
    EXPECT_TRUE(proc.reloadThreatDetectorConfiguration());
}


TEST_F(TestPolicyProcessor_ALC_policy, getCustomerIdFromEmptyAttributeMap)
{
    const std::string policyXml = R"sophos(<?xml version="1.0"?>
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

TEST_F(TestPolicyProcessor_ALC_policy, getCustomerIdFromAttributeMapNoAlgorithm)
{
    const std::string policyXml = R"sophos(<?xml version="1.0"?>
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

TEST_F(TestPolicyProcessor_ALC_policy, getCustomerIdFromAttributeMapHashed)
{
    const std::string policyXml = R"sophos(<?xml version="1.0"?>
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


TEST_F(TestPolicyProcessor_ALC_policy, getCustomerIdFromAttributeMapNotHashed)
{
    const std::string policyXml = R"sophos(<?xml version="1.0"?>
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


TEST_F(TestPolicyProcessor_ALC_policy, getCustomerIdFromAttributeMapEmptyPassword)
{
    const std::string policyXml = R"sophos(<?xml version="1.0"?>
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


TEST_F(TestPolicyProcessor_ALC_policy, getCustomerIdFromAttributeMapMissingPassword)
{
    const std::string policyXml = R"sophos(<?xml version="1.0"?>
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


TEST_F(TestPolicyProcessor_ALC_policy, getCustomerIdFromAttributeMapEmptyUserName)
{
    const std::string policyXml = R"sophos(<?xml version="1.0"?>
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

TEST_F(TestPolicyProcessor_ALC_policy, getCustomerIdFromAttributeMapMissingUserName)
{
    const std::string policyXml = R"sophos(<?xml version="1.0"?>
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


TEST_F(TestPolicyProcessor_ALC_policy, getCustomerIdFromAttributeMapInvalidAlgorithm)
{
    const std::string policyXml = R"sophos(<?xml version="1.0"?>
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


TEST_F(TestPolicyProcessor_ALC_policy, getCustomerIdFromBlankAttributeMap)
{
    const std::string policyXml {};

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


TEST_F(TestPolicyProcessor_ALC_policy, processAlcPolicyInvalid)
{
    const std::string expectedMd5 = "5e259db8da3ae4df8f18a2add2d3d47d";
    EXPECT_CALL(*m_mockIFileSystemPtr, readFile(_)).WillOnce(Return(expectedMd5));
    expectAbsentSusiConfig();

    Tests::ScopedReplaceFileSystem replacer(std::move(m_mockIFileSystemPtr));

    Plugin::PolicyProcessor proc{nullptr};

    const std::string policyXml = R"sophos(<?xml version="1.0"?>
<AUConfigurations>
  <AUConfig>
    <primary_location>
      <server />
    </primary_location>
  </AUConfig>
</AUConfigurations>
)sophos";

    auto attributeMap = Common::XmlUtilities::parseXml(policyXml);
    proc.processAlcPolicy(attributeMap);
    EXPECT_FALSE(proc.restartThreatDetector());
    EXPECT_FALSE(proc.reloadThreatDetectorConfiguration());
}

namespace
{
    class ExposedPolicyProcessor : public Plugin::PolicyProcessor
    {
    public:
        using Plugin::PolicyProcessor::PolicyProcessor;
        void setTemporaryMarkerBooleansTrue()
        {
            m_restartThreatDetector = true;
            reloadThreatDetectorConfiguration_ = true;
        }
    };
}

TEST_F(TestPolicyProcessor_ALC_policy, test_markers_reset_by_alc_policy)
{
    std::string policyXml = GL_POLICY_2;
    auto attributeMap = Common::XmlUtilities::parseXml(policyXml);

    expectReadCustomerIdRepeatedly();
    expectAbsentSusiConfig();
    expectWriteCustomerID(_);
    Tests::ScopedReplaceFileSystem replacer(std::move(m_mockIFileSystemPtr));

    ExposedPolicyProcessor policyProcessor{nullptr};
    policyProcessor.processAlcPolicy(attributeMap);
    policyProcessor.setTemporaryMarkerBooleansTrue();
    policyProcessor.processAlcPolicy(attributeMap);
    EXPECT_FALSE(policyProcessor.restartThreatDetector());
    EXPECT_FALSE(policyProcessor.reloadThreatDetectorConfiguration());
}