// Copyright 2020-2023 Sophos Limited. All rights reserved.

#include "TestPolicyProcessor.h"

#include "datatypes/sophos_filesystem.h"

#include "Common/Helpers/FileSystemReplaceAndRestore.h"
#include "Common/FileSystem/IFileSystemException.h"

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
            expectWriteSusiConfig(sxlEnabled, {});
        }

        void expectWriteSusiConfig(bool sxlEnabled, const std::vector<std::string>& puaApprovedList)
        {
            nlohmann::json settings;
            settings["enableSxlLookup"] = sxlEnabled;
            settings["machineLearning"] = true;
            settings["puaApprovedList"] = puaApprovedList;
            settings["pathAllowList"] = nlohmann::json::array();
            settings["shaAllowList"] = nlohmann::json::array();
            settings["sxlUrl"] = "";
            expectWriteSusiConfigFromString(settings.dump());
        }

        std::string m_soapConfigPath;
    };
}

#ifdef USE_SXL_ENABLE_FROM_CORC_POLICY
constexpr bool SXL_FROM_CORC_POLICY = true;
#else
constexpr bool SXL_FROM_CORC_POLICY = false;
#endif


TEST_F(TestPolicyProcessor_SAV_policy, processSavPolicy)
{
    expectWriteSoapdConfig();
    expectReadSoapdConfig();
    expectConstructorCalls();
    expectWriteSusiConfigFromBool(!SXL_FROM_CORC_POLICY);

    Tests::ScopedReplaceFileSystem replacer(std::move(m_mockIFileSystemPtr));

    PolicyProcessorUnitTestClass proc;

    std::string policyXml = R"sophos(<?xml version="1.0"?>
<config>
    <detectionFeedback>
        <sendData>true</sendData>
    </detectionFeedback>
</config>
)sophos";

    EXPECT_NO_THROW(proc.processSAVpolicyFromString(policyXml));
    EXPECT_EQ(proc.restartThreatDetector(), !SXL_FROM_CORC_POLICY);
}

TEST_F(TestPolicyProcessor_SAV_policy, defaultSXL4lookupValueIsFalse)
{
    Plugin::PolicyProcessor proc{nullptr};
    EXPECT_EQ(proc.getSXL4LookupsEnabled(), common::ThreatDetector::SXL_DEFAULT);
}

#ifndef USE_SXL_ENABLE_FROM_CORC_POLICY
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
    // Only restart if default is false
    EXPECT_EQ(proc.restartThreatDetector(), !common::ThreatDetector::SXL_DEFAULT);
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
    // Only restart if default is false
    EXPECT_EQ(proc.restartThreatDetector(), !common::ThreatDetector::SXL_DEFAULT);
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
#endif

#ifdef USE_ON_ACCESS_EXCLUSIONS_FROM_SAV_POLICY

// Tests for getting on-access exclusions from SAV policy

TEST_F(TestPolicyProcessor_SAV_policy, getOnAccessExclusions)
{
    expectReadSoapdConfig();
    expectConstructorCalls();
    expectWriteSusiConfigFromBool(!SXL_FROM_CORC_POLICY);
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
    EXPECT_TRUE(proc.reloadThreatDetectorConfiguration());
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