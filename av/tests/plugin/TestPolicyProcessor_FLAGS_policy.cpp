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
    class TestPolicyProcessor_FLAGS_policy : public TestPolicyProcessorBase
    {
    protected:
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

            m_soapFlagConfigPath = m_testDir / "var/oa_flag.json";
            m_customerIdPath = m_testDir / "var/customer_id.txt";
            m_mockIFileSystemPtr = std::make_unique<StrictMock<MockFileSystem>>();
        }

        void TearDown() override
        {
            fs::remove_all(m_testDir);
        }

        void expectReadConfigFile()
        {
            EXPECT_CALL(*m_mockIFileSystemPtr, readFile(m_soapFlagConfigPath)).WillOnce(Return("{}"));
        }

        void expectReadCustomerId()
        {
            EXPECT_CALL(*m_mockIFileSystemPtr, readFile(m_customerIdPath)).WillOnce(Return(""));
        }

        std::string m_soapFlagConfigPath;
        std::string m_customerIdPath;
        std::unique_ptr<StrictMock<MockFileSystem>> m_mockIFileSystemPtr;
    };
}

TEST_F(TestPolicyProcessor_FLAGS_policy, testProcessFlagSettingCatchesBadJson)
{
    UsingMemoryAppender memAppend(*this);

    PolicyProcessorUnitTestClass proc;

    proc.processFlagSettings("{\"bad\" \"json\": true and false}");

    EXPECT_TRUE(appenderContains(
        "Failed to parse FLAGS policy due to parse error, reason: "));
}

TEST_F(TestPolicyProcessor_FLAGS_policy, testProcessFlagSettingsEnabled)
{
    UsingMemoryAppender memAppend(*this);
    expectReadCustomerId();
    expectReadConfigFile();

    EXPECT_CALL(
        *m_mockIFileSystemPtr,
        writeFileAtomically(m_soapFlagConfigPath, R"sophos({"oa_enabled":true})sophos", _, 0640));

    Tests::ScopedReplaceFileSystem replacer(std::move(m_mockIFileSystemPtr));

    PolicyProcessorUnitTestClass proc;

    proc.processFlagSettings(R"sophos({"av.onaccess.enabled": true, "safestore.enabled": true})sophos");

    EXPECT_TRUE(appenderContains(
        "On-access is enabled in the FLAGS policy, assuming on-access policy settings"));
    EXPECT_TRUE(appenderContains("SafeStore flag set. Setting SafeStore to enabled."));
    EXPECT_TRUE(proc.isSafeStoreEnabled());
}

TEST_F(TestPolicyProcessor_FLAGS_policy, testProcessFlagSettingsDisabled)
{
    UsingMemoryAppender memAppend(*this);

    expectReadCustomerId();
    expectReadConfigFile();

    EXPECT_CALL(
        *m_mockIFileSystemPtr,
        writeFileAtomically(m_soapFlagConfigPath, R"sophos({"oa_enabled":false})sophos", _, 0640));

    Tests::ScopedReplaceFileSystem replacer(std::move(m_mockIFileSystemPtr));

    PolicyProcessorUnitTestClass proc;

    proc.processFlagSettings(R"sophos({"av.onaccess.enabled": false, "safestore.enabled": false})sophos");

    EXPECT_TRUE(appenderContains(
        "On-access is disabled in the FLAGS policy, overriding on-access policy settings"));
    EXPECT_TRUE(appenderContains("SafeStore flag not set. Setting SafeStore to disabled."));
    EXPECT_FALSE(proc.isSafeStoreEnabled());
}

TEST_F(TestPolicyProcessor_FLAGS_policy, testProcessFlagSettingsDefault)
{
    expectReadCustomerId();
    expectReadConfigFile();

    UsingMemoryAppender memAppend(*this);

    EXPECT_CALL(
        *m_mockIFileSystemPtr,
        writeFileAtomically(m_soapFlagConfigPath, R"sophos({"oa_enabled":false})sophos", _, 0640));

    Tests::ScopedReplaceFileSystem replacer(std::move(m_mockIFileSystemPtr));

    PolicyProcessorUnitTestClass proc;

    proc.processFlagSettings("{\"av.something_else\":  false}");

    EXPECT_TRUE(appenderContains("No on-access flag found, overriding on-access policy settings"));
    EXPECT_TRUE(appenderContains("SafeStore flag not set. Setting SafeStore to disabled."));
    EXPECT_FALSE(proc.isSafeStoreEnabled());
}

TEST_F(TestPolicyProcessor_FLAGS_policy, testWriteFlagConfigFailedOnAccess)
{
    expectReadCustomerId();
    expectReadConfigFile();
    UsingMemoryAppender memAppend(*this);

    Common::FileSystem::IFileSystemException ex("error!");
    EXPECT_CALL(
        *m_mockIFileSystemPtr,
        writeFileAtomically(m_soapFlagConfigPath, R"sophos({"oa_enabled":false})sophos", _, 0640))
        .WillOnce(Throw(ex));

    Tests::ScopedReplaceFileSystem replacer(std::move(m_mockIFileSystemPtr));

    PolicyProcessorUnitTestClass proc;

    proc.processFlagSettings(R"sophos({"av.onaccess.enabled": false, "safestore.enabled": true})sophos");

    EXPECT_TRUE(appenderContains(
        "Failed to write Flag Config, Sophos On Access Process will use the default settings error!"));
}
