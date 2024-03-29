// Copyright 2022-2023 Sophos Limited. All rights reserved.

#pragma once

#include "PluginMemoryAppenderUsingTests.h"

#include "pluginimpl/PolicyProcessor.h"

#include "datatypes/sophos_filesystem.h"
#include "datatypes/Print.h"

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
#include "Common/Helpers/MockFileSystem.h"

namespace fs = sophos_filesystem;

namespace
{
    class PolicyProcessorUnitTestClass : public Plugin::PolicyProcessor
    {
    public:
        PolicyProcessorUnitTestClass() : Plugin::PolicyProcessor(nullptr) {}

        void processCOREpolicyFromString(const std::string& policy)
        {
            auto attributeMap = Common::XmlUtilities::parseXml(policy);
            processCOREpolicy(attributeMap);
        }

        void processCORCpolicyFromString(const std::string& policy)
        {
            auto attributeMap = Common::XmlUtilities::parseXml(policy);
            processCorcPolicy(attributeMap);
        }

        void processSAVpolicyFromString(const std::string& policy)
        {
            auto attributeMap = Common::XmlUtilities::parseXml(policy);
            processSavPolicy(attributeMap);
        }

    protected:
        void notifyOnAccessProcessIfRequired() override { PRINT("Notified soapd"); }
    };

    class TestPolicyProcessorBase : public PluginMemoryAppenderUsingTests
    {
    protected:
        fs::path m_testDir;
        std::unique_ptr<StrictMock<MockFileSystem>> m_mockIFileSystemPtr;
        std::string m_customerIdPath;
        std::string m_susiStartupConfigPath;
        std::string m_susiStartupConfigChrootPath;

        void createTestDir()
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

            auto tmpdir = m_testDir / "tmp";
            fs::create_directories(tmpdir);

            auto var = m_testDir / "var";
            fs::create_directories(var);
        }

        void setupBase()
        {
            createTestDir();
            m_mockIFileSystemPtr = std::make_unique<StrictMock<MockFileSystem>>();
            m_customerIdPath = m_testDir / "var/customer_id.txt";
            m_susiStartupConfigPath = m_testDir / "var/susi_startup_settings.json";
            m_susiStartupConfigChrootPath = std::string(m_testDir / "chroot") + m_susiStartupConfigPath;
        }

        void TearDown() override
        {
            fs::remove_all(m_testDir);
        }

        void expectReadCustomerIdOnce()
        {
            EXPECT_CALL(*m_mockIFileSystemPtr, readFile(m_customerIdPath)).WillOnce(Return(""));
        }

        void expectReadCustomerIdRepeatedly()
        {
            EXPECT_CALL(*m_mockIFileSystemPtr, readFile(m_customerIdPath)).WillRepeatedly(Return(""));
        }

        void expectWriteSusiConfigFromString(const std::string& expected)
        {
            EXPECT_CALL(*m_mockIFileSystemPtr, writeFileAtomically(m_susiStartupConfigPath, expected,_ ,_)).Times(1);
            EXPECT_CALL(*m_mockIFileSystemPtr, writeFileAtomically(m_susiStartupConfigChrootPath, expected,_ ,_)).Times(1);
        }

        template<typename Matcher>
        void expectWriteSusiConfig(const Matcher& expected)
        {
            EXPECT_CALL(*m_mockIFileSystemPtr, writeFileAtomically(m_susiStartupConfigPath, expected,_ ,_)).Times(1);
            EXPECT_CALL(*m_mockIFileSystemPtr, writeFileAtomically(m_susiStartupConfigChrootPath, expected,_ ,_)).Times(1);
        }

        /**
         * Saves the text written to m_susiStartupConfigChrootPath
         * and matches expected against both m_susiStartupConfigPath and m_susiStartupConfigChrootPath
         * @tparam Matcher
         * @param expected
         * @param destination
         */
        template<typename Matcher>
        void saveSusiConfigFromWrite(const Matcher& expected, std::string& destination)
        {
            EXPECT_CALL(*m_mockIFileSystemPtr, writeFileAtomically(m_susiStartupConfigPath, expected,_ ,_)).Times(1);
            EXPECT_CALL(*m_mockIFileSystemPtr, writeFileAtomically(m_susiStartupConfigChrootPath, expected,_ ,_)).WillOnce(
                SaveArg<1>(&destination)
                );
        }

        /**
         * Saves the text written to each file, to verify the same text is written to both files.
         * @tparam Matcher
         * @param expected
         * @param base
         * @param chroot
         */
        template<typename Matcher>
        void saveSusiConfigFromBothWrites(const Matcher& expected, std::string& base, std::string& chroot)
        {
            EXPECT_CALL(*m_mockIFileSystemPtr, writeFileAtomically(m_susiStartupConfigPath, expected,_ ,_)).WillOnce(
                SaveArg<1>(&base)
            );
            EXPECT_CALL(*m_mockIFileSystemPtr, writeFileAtomically(m_susiStartupConfigChrootPath, expected,_ ,_)).WillOnce(
                SaveArg<1>(&chroot)
            );
        }

        void expectAbsentSusiConfig()
        {
            EXPECT_CALL(*m_mockIFileSystemPtr, isFile(m_susiStartupConfigPath)).WillOnce(Return(false));
        }

        void expectConstructorCalls()
        {
            expectReadCustomerIdOnce();
            expectAbsentSusiConfig();
        }
    };
}
