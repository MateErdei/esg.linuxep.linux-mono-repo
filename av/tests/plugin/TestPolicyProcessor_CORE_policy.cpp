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
    class TestPolicyProcessor_CORE_policy : public TestPolicyProcessorBase
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

        void expectReadSoapdConfig()
        {
            EXPECT_CALL(*m_mockIFileSystemPtr, readFile(m_soapConfigPath)).WillRepeatedly(
                Throw(Common::FileSystem::IFileSystemException("Test exception"))
            );
        }

        void expectReadCustomerIdOnce()
        {
            EXPECT_CALL(*m_mockIFileSystemPtr, readFile(m_customerIdPath)).WillOnce(Return(""));
        }

        void expectReadCustomerIdRepeatedly()
        {
            EXPECT_CALL(*m_mockIFileSystemPtr, readFile(m_customerIdPath)).WillRepeatedly(Return(""));
        }


        std::string m_susiStartupConfigPath;
        std::string m_susiStartupConfigChrootPath;
        std::string m_soapConfigPath;
        std::string m_soapFlagConfigPath;
        std::string m_customerIdPath;
        std::unique_ptr<StrictMock<MockFileSystem>> m_mockIFileSystemPtr;
    };
}

TEST_F(TestPolicyProcessor_CORE_policy, sendExampleCOREpolicy)
{
    std::string CORE_policy{R"sophos(<?xml version="1.0"?>
<policy RevID="{{revisionId}}" policyType="36">

  <!-- Core Features -->
 <coreFeatures>
    <machineLearningEnabled>{{true|false}}</machineLearningEnabled>
    <malwareDetectionEnabled>{{true|false}}</malwareDetectionEnabled>
    <appControlEnabled>{{true|false}}</appControlEnabled>
    <mlDetectionLevel>{{default|conservative}}</mlDetectionLevel>
    <behaviouralEnabled>{{true|false}}</behaviouralEnabled>
    <backgroundScanningEnabled>{{true|false}}</backgroundScanningEnabled>
    <eventJournalsEnabled>{{true|false}}</eventJournalsEnabled>
    <backgroundMemoryScanningEnabled>{{true|false}}</backgroundMemoryScanningEnabled>
    <activeAdversaryMitigationEnabled>{{true|false}}</activeAdversaryMitigationEnabled>
  </coreFeatures>

  <!-- Linux (SSPL) Features -->
  <linuxFeatures>
    <linuxBehaviouralEnabled>{{true|false}}</linuxBehaviouralEnabled>
  </linuxFeatures>

  <!-- New EDR/RCA/FIM hashing exclusions -->
  <journalHashing>
    <exclusions>
      <excludeRemoteFiles>{{true|false}}</excludeRemoteFiles>
      <filePathSet>
        <filePath>{{filePath1}}</filePath>
        <filePath>{{filePath2}}</filePath>
      </filePathSet>
      <processPathSet>
        <processPath>{{processPath1}}</processPath>
        <processPath>{{processPath2}}</processPath>
      </processPathSet>
    </exclusions>
  </journalHashing>

  <!-- Block QUIC setting -->
  <webBlockQuic>
    <enabled>{{true|false}}</enabled>
  </webBlockQuic>

  <!-- Https decryption setting -->
  <webHttpsDecrypt>
    <enabled>{{true|false}}</enabled>
  </webHttpsDecrypt>

  <!-- From SAV policy -->
  <onAccessScan>
    <enabled>{{true|false}}</enabled>
    <!-- START New (for finer control over on-access scans) -->
    <onRead>{{true|false}}</onRead>
    <onWrite>{{true|false}}</onWrite>
    <onExec>{{true|false}}</onExec>
    <!-- END New (for finer control over on-access scans) -->
    <exclusions>
      <excludeRemoteFiles>{{true|false}}</excludeRemoteFiles>
      <filePathSet>
        <filePath>{{filePath1}}</filePath>
        <filePath>{{filePath2}}</filePath>
        ...
      </filePathSet>
      <processPathSet>
        <processPath>{{processPath1}}</processPath>
        <processPath>{{processPath2}}</processPath>
        ...
      </processPathSet>
    </exclusions>
  </onAccessScan>

  <amsiProtection>
    <enabled>{{true|false}}</enabled> <!--Whether Microsoft anti malware scanning interface capability is enabled-->
    <blockOnDetect>{{true|false}}</blockOnDetect> <!--True to block execution when an AMSI detection occurs, false to run in monitor mode and allow execution to continue.-->
    <exclusions>
        <filePathSet>
            <filePath>{{filePath1}}</filePath>
            <filePath>{{filePath2}}</filePath>
        </filePathSet>
    </exclusions>
  </amsiProtection>

  <!-- From SED policy -->
  <tamper-protection>
    <enabled>{{tamperProtectionEnabled}}</enabled>
    <ignore-sav>{{tamperProtectionIgnoreSav}}</ignore-sav>
    <password>{{tamperProtectionPassword}}</password>
  </tamper-protection>

  <!-- From STAC policy -->
  <rcaUploadEnabled>{{rcaUploadEnabled}}</rcaUploadEnabled>
  <rcaUploadUri>{{rcaUploadUri}}</rcaUploadUri>
  <rcaConfiguration>
    <nodeLimit>{{nodeLimit}}</nodeLimit>
    <artifactLimit>{{artifactLimit}}</artifactLimit>
    <fileReadLimit>{{fileReadLimit}}</fileReadLimit>
    <fileWriteLimit>{{fileWriteLimit}}</fileWriteLimit>
    <fileRenameLimit>{{fileRenameLimit}}</fileRenameLimit>
    <fileDeleteLimit>{{fileDeleteLimit}}</fileDeleteLimit>
    <regKeyCreateLimit>{{regKeyCreateLimit}}</regKeyCreateLimit>
    <regKeyDeleteLimit>{{regKeyDeleteLimit}}</regKeyDeleteLimit>
    <regKeySetValueLimit>{{regKeySetValueLimit}}</regKeySetValueLimit>
    <regKeyDeleteValueLimit>{{regKeyDeleteValueLimit}}</regKeyDeleteValueLimit>
    <urlAccessLimit>{{urlAccessLimit}}</urlAccessLimit>
    <dnsLookupLimit>{{dnsLookupLimit}}</dnsLookupLimit>
    <ipConnectorLimit>{{ipConnectorLimit}}</ipConnectorLimit>
  </rcaConfiguration>
  <rcaUploadHeaders>
    <header><name>{{headerName1}}</name><value>{{headerValue1}}</value></header>
    <header><name>{{headerName2}}</name><value>{{headerValue2}}</value></header>
    ...
  </rcaUploadHeaders>

  <snapshotUploadUri>{{snapshotUploadUri}}</snapshotUploadUri>
  <snapshotUploadHeaders>
    <header><name>{{headerName3}}</name><value>{{headerValue3}}</value></header>
    <header><name>{{headerName4}}</name><value>{{headerValue4}}</value></header>
    ...
  </snapshotUploadHeaders>

  <!-- Monitor Mode policy -->
  <monitorMode>
      <enabled>true</enabled>
  </monitorMode>

  <!-- Aggressive Activity Classification policy -->
  <aggressiveActivityClassification>
      <enabled>true</enabled>
  </aggressiveActivityClassification>

  <!-- Account Preferences Privacy options -->
  <privacySettings>
    <cixDataSharingEnabled>{{true|false}}</cixDataSharingEnabled> <!-- true when the toggle is ON -->
  </privacySettings>

</policy>)sophos"};

    EXPECT_CALL(*m_mockIFileSystemPtr, readFile(_)).WillOnce(Return(""));

    Tests::ScopedReplaceFileSystem replacer(std::move(m_mockIFileSystemPtr));

    auto attributeMap = Common::XmlUtilities::parseXml(CORE_policy);

    PolicyProcessorUnitTestClass proc;

    EXPECT_THROW(proc.processCOREpolicy(attributeMap), Plugin::InvalidPolicyException);
}

TEST_F(TestPolicyProcessor_CORE_policy, processOnAccessPolicyEnabled)
{
    std::string CORE_policy{R"sophos(<?xml version="1.0"?>
<policy RevID="{{revisionId}}" policyType="36">
  <!-- From SAV policy -->
  <onAccessScan>
    <enabled>true</enabled>
    <exclusions>
      <excludeRemoteFiles>false</excludeRemoteFiles>
    </exclusions>
  </onAccessScan>
</policy>)sophos"};

    expectReadSoapdConfig();
    expectReadCustomerIdOnce();
    EXPECT_CALL(*m_mockIFileSystemPtr, writeFileAtomically(m_soapConfigPath,
#ifdef USE_ON_ACCESS_EXCLUSIONS_FROM_SAV_POLICY
                                                           R"sophos({"enabled":true,"excludeRemoteFiles":false})sophos",
#else
                                                           R"sophos({"enabled":true,"excludeRemoteFiles":false,"exclusions":[]})sophos",
#endif
                                                           _,
                                                           0640));

    Tests::ScopedReplaceFileSystem replacer(std::move(m_mockIFileSystemPtr));

    auto attributeMap = Common::XmlUtilities::parseXml(CORE_policy);

    PolicyProcessorUnitTestClass proc;

    proc.processCOREpolicy(attributeMap);
}

TEST_F(TestPolicyProcessor_CORE_policy, processOnAccessPolicyDisabled)
{
    std::string CORE_policy{R"sophos(<?xml version="1.0"?>
<policy RevID="{{revisionId}}" policyType="36">
  <!-- From SAV policy -->
  <onAccessScan>
    <enabled>false</enabled>
    <exclusions>
      <excludeRemoteFiles>false</excludeRemoteFiles>
    </exclusions>
  </onAccessScan>
</policy>)sophos"};

    expectReadSoapdConfig();
    expectReadCustomerIdOnce();
    EXPECT_CALL(*m_mockIFileSystemPtr, writeFileAtomically(m_soapConfigPath,
#ifdef USE_ON_ACCESS_EXCLUSIONS_FROM_SAV_POLICY
                                                           R"sophos({"enabled":false,"excludeRemoteFiles":false})sophos",
#else
                                                           R"sophos({"enabled":false,"excludeRemoteFiles":false,"exclusions":[]})sophos",
#endif
                                                           _,
                                                           0640));

    Tests::ScopedReplaceFileSystem replacer(std::move(m_mockIFileSystemPtr));

    auto attributeMap = Common::XmlUtilities::parseXml(CORE_policy);

    PolicyProcessorUnitTestClass proc;

    proc.processCOREpolicy(attributeMap);
}

TEST_F(TestPolicyProcessor_CORE_policy, processOnAccessPolicyExcludeRemoteEnabled)
{
    std::string CORE_policy{R"sophos(<?xml version="1.0"?>
<policy RevID="{{revisionId}}" policyType="36">
  <!-- From SAV policy -->
  <onAccessScan>
    <enabled>false</enabled>
    <exclusions>
      <excludeRemoteFiles>true</excludeRemoteFiles>
    </exclusions>
  </onAccessScan>
</policy>)sophos"};

    expectReadSoapdConfig();
    expectReadCustomerIdOnce();
    EXPECT_CALL(*m_mockIFileSystemPtr, writeFileAtomically(m_soapConfigPath,
#ifdef USE_ON_ACCESS_EXCLUSIONS_FROM_SAV_POLICY
                                                           R"sophos({"enabled":false,"excludeRemoteFiles":true})sophos",
#else
                                                           R"sophos({"enabled":false,"excludeRemoteFiles":true,"exclusions":[]})sophos",
#endif
                                                           _,
                                                           0640));

    Tests::ScopedReplaceFileSystem replacer(std::move(m_mockIFileSystemPtr));

    auto attributeMap = Common::XmlUtilities::parseXml(CORE_policy);

    PolicyProcessorUnitTestClass proc;

    proc.processCOREpolicy(attributeMap);
}


#ifndef USE_ON_ACCESS_EXCLUSIONS_FROM_SAV_POLICY
TEST_F(TestPolicyProcessor_CORE_policy, processOnAccessPolicyPathExclusions)
{
    std::string CORE_policy{R"sophos(<?xml version="1.0"?>
<policy RevID="{{revisionId}}" policyType="36">
  <!-- From SAV policy -->
  <onAccessScan>
    <enabled>false</enabled>
    <exclusions>
      <excludeRemoteFiles>true</excludeRemoteFiles>
        <filePathSet>
          <filePath>a</filePath>
          <filePath>b</filePath>
          ...
        </filePathSet>
        <processPathSet>
          <processPath>{{processPath1}}</processPath>
          <processPath>{{processPath2}}</processPath>
          ...
        </processPathSet>
    </exclusions>
  </onAccessScan>
</policy>)sophos"};

    expectReadSoapdConfig();
    expectReadCustomerIdOnce();
    EXPECT_CALL(*m_mockIFileSystemPtr, writeFileAtomically(m_soapConfigPath,
                                                           R"sophos({"enabled":false,"excludeRemoteFiles":true,"exclusions":["a","b"]})sophos",
                                                           _,
                                                           0640));

    Tests::ScopedReplaceFileSystem replacer(std::move(m_mockIFileSystemPtr));

    auto attributeMap = Common::XmlUtilities::parseXml(CORE_policy);

    PolicyProcessorUnitTestClass proc;

    proc.processCOREpolicy(attributeMap);
}
#endif
