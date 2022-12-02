// Copyright 2020-2022, Sophos Limited.  All rights reserved.

#include "PluginMemoryAppenderUsingTests.h"
#include "TestPolicyProcessor.h"

#include "datatypes/sophos_filesystem.h"

#include <Common/FileSystem/IFileSystemException.h>
#include <Common/Helpers/FileSystemReplaceAndRestore.h>

#include <gtest/gtest.h>

namespace fs = sophos_filesystem;

namespace
{
    class TestPolicyProcessor_FLAGS_policy : public TestPolicyProcessorBase
    {
    protected:
        void SetUp() override
        {
            setupBase();
            m_soapFlagConfigPath = m_testDir / "var/oa_flag.json";
        }

        void expectReadConfigFile()
        {
            EXPECT_CALL(*m_mockIFileSystemPtr, readFile(m_soapFlagConfigPath)).WillOnce(Return("{}"));
        }

        void expectReadCustomerId()
        {
            expectReadCustomerIdOnce();
        }

        std::string m_soapFlagConfigPath;
    };
} // namespace

TEST_F(TestPolicyProcessor_FLAGS_policy, testProcessFlagSettingCatchesBadJson)
{
    UsingMemoryAppender memAppend(*this);

    PolicyProcessorUnitTestClass proc;

    proc.processFlagSettings("{\"bad\" \"json\": true and false}");

    EXPECT_TRUE(appenderContains("Failed to parse FLAGS policy due to parse error, reason: "));
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

    proc.processFlagSettings(
        R"sophos({"av.onaccess.enabled": true, "safestore.enabled": true, "safestore.quarantine-ml": true})sophos");

    EXPECT_TRUE(appenderContains("On-access is enabled in the FLAGS policy, assuming on-access policy settings"));
    EXPECT_TRUE(appenderContains("SafeStore flag set. Setting SafeStore to enabled."));
    EXPECT_TRUE(appenderContains("SafeStore Quarantine ML flag set. SafeStore will quarantine ML detections."));
    EXPECT_TRUE(proc.isSafeStoreEnabled());
    EXPECT_TRUE(proc.shouldSafeStoreQuarantineMl());
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

    EXPECT_TRUE(appenderContains("On-access is disabled in the FLAGS policy, overriding on-access policy settings"));
    EXPECT_TRUE(appenderContains("SafeStore flag not set. Setting SafeStore to disabled."));
    EXPECT_FALSE(appenderContains("SafeStore Quarantine ML flag"));
    EXPECT_FALSE(proc.isSafeStoreEnabled());
    EXPECT_FALSE(proc.shouldSafeStoreQuarantineMl());
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
    EXPECT_FALSE(appenderContains("SafeStore Quarantine ML flag"));
    EXPECT_FALSE(proc.isSafeStoreEnabled());
    EXPECT_FALSE(proc.shouldSafeStoreQuarantineMl());
}

TEST_F(TestPolicyProcessor_FLAGS_policy, testProcessFlagSettingsSafeStoreEnabledQuarantineMlDefaultsToDisabled)
{
    expectReadCustomerId();
    expectReadConfigFile();

    UsingMemoryAppender memAppend(*this);

    EXPECT_CALL(
        *m_mockIFileSystemPtr,
        writeFileAtomically(m_soapFlagConfigPath, R"sophos({"oa_enabled":false})sophos", _, 0640));

    Tests::ScopedReplaceFileSystem replacer(std::move(m_mockIFileSystemPtr));

    PolicyProcessorUnitTestClass proc;

    proc.processFlagSettings(R"sophos({"safestore.enabled": true})sophos");

    EXPECT_TRUE(appenderContains("SafeStore flag set. Setting SafeStore to enabled."));
    EXPECT_TRUE(appenderContains("SafeStore Quarantine ML flag not set. SafeStore will not quarantine ML detections."));
    EXPECT_TRUE(proc.isSafeStoreEnabled());
    EXPECT_FALSE(proc.shouldSafeStoreQuarantineMl());
}

TEST_F(TestPolicyProcessor_FLAGS_policy, testProcessFlagSettingsQuarantineMlEnabledWithoutSafeStore)
{
    expectReadCustomerId();
    expectReadConfigFile();

    UsingMemoryAppender memAppend(*this);

    EXPECT_CALL(
        *m_mockIFileSystemPtr,
        writeFileAtomically(m_soapFlagConfigPath, R"sophos({"oa_enabled":false})sophos", _, 0640));

    Tests::ScopedReplaceFileSystem replacer(std::move(m_mockIFileSystemPtr));

    PolicyProcessorUnitTestClass proc;

    proc.processFlagSettings(R"sophos({"safestore.quarantine-ml": true})sophos");

    EXPECT_TRUE(appenderContains("SafeStore flag not set. Setting SafeStore to disabled."));
    EXPECT_FALSE(appenderContains("SafeStore Quarantine ML flag"));
    EXPECT_FALSE(proc.isSafeStoreEnabled());
    EXPECT_FALSE(proc.shouldSafeStoreQuarantineMl());
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

    proc.processFlagSettings(R"sophos({"av.onaccess.enabled": false})sophos");

    EXPECT_TRUE(appenderContains(
        "Failed to write Flag Config, Sophos On Access Process will use the default settings error!"));
}

TEST_F(TestPolicyProcessor_FLAGS_policy, testWriteFlagConfigFailedOnAccessDoesntPreventEnablingSafeStoreFlags)
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
    EXPECT_TRUE(appenderContains("SafeStore flag set. Setting SafeStore to enabled."));
    EXPECT_TRUE(proc.isSafeStoreEnabled());
}
