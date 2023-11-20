// Copyright 2020-2023 Sophos Limited. All rights reserved.

#include "PluginMemoryAppenderUsingTests.h"
#include "TestPolicyProcessor.h"

#include "datatypes/sophos_filesystem.h"

#include "Common/FileSystem/IFileSystemException.h"
#include "Common/Helpers/FileSystemReplaceAndRestore.h"

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
    expectConstructorCalls();

    Tests::ScopedReplaceFileSystem replacer(std::move(m_mockIFileSystemPtr));

    PolicyProcessorUnitTestClass proc;

    proc.processFlagSettings(
        R"sophos({"safestore.quarantine-ml": true})sophos");


    EXPECT_TRUE(appenderContains("SafeStore Quarantine ML flag set. SafeStore will quarantine ML detections."));
    EXPECT_TRUE(proc.shouldSafeStoreQuarantineMl());
}

TEST_F(TestPolicyProcessor_FLAGS_policy, testProcessFlagSettingsDisabled)
{
    UsingMemoryAppender memAppend(*this);
    expectConstructorCalls();

    Tests::ScopedReplaceFileSystem replacer(std::move(m_mockIFileSystemPtr));

    PolicyProcessorUnitTestClass proc;

    proc.processFlagSettings(R"sophos({"safestore.quarantine-ml": false})sophos");

    EXPECT_TRUE(appenderContains("SafeStore Quarantine ML flag not set. SafeStore will not quarantine ML detections."));

    EXPECT_FALSE(proc.shouldSafeStoreQuarantineMl());
}

TEST_F(TestPolicyProcessor_FLAGS_policy, testProcessFlagSettingsDefault)
{
    expectConstructorCalls();
    UsingMemoryAppender memAppend(*this);

    Tests::ScopedReplaceFileSystem replacer(std::move(m_mockIFileSystemPtr));

    PolicyProcessorUnitTestClass proc;

    proc.processFlagSettings("{\"av.something_else\":  false}");

    EXPECT_TRUE(appenderContains("SafeStore Quarantine ML flag not set. SafeStore will not quarantine ML detections."));
    EXPECT_FALSE(proc.shouldSafeStoreQuarantineMl());
}
