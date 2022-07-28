/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "sophos_on_access_process/OnAccessConfigMonitor/OnAccessConfigMonitor.h"

#include "common/LogInitializedTests.h"
#include "datatypes/sophos_filesystem.h"

#include "Common/Helpers/FileSystemReplaceAndRestore.h"
#include "Common/FileSystem/IFileSystemException.h"
#include "Common/Helpers/MockFileSystem.h"

#include <gtest/gtest.h>

using namespace sophos_on_access_process::ConfigMonitorThread;
namespace fs = sophos_filesystem;

class TestOnAccessConfigMonitor : public LogInitializedTests
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

        m_soapConfigPath = m_testDir / "var/soapd_config.json";
        m_mockIFileSystemPtr = std::make_unique<StrictMock<MockFileSystem>>();
    }

    void TearDown() override
    {
        fs::remove_all(m_testDir);
    }

    std::string m_soapConfigPath;
    std::unique_ptr<StrictMock<MockFileSystem>> m_mockIFileSystemPtr;
};

TEST_F(TestOnAccessConfigMonitor, readConfigFile) // NOLINT
{
    EXPECT_CALL(*m_mockIFileSystemPtr, readFile(m_soapConfigPath)).WillOnce(Return("x"));

    Tests::ScopedReplaceFileSystem replacer(std::move(m_mockIFileSystemPtr));

    ASSERT_EQ(OnAccessConfigMonitor::readConfigFile(), "x");
}

TEST_F(TestOnAccessConfigMonitor, readConfigFileReadThrows) // NOLINT
{
    EXPECT_CALL(*m_mockIFileSystemPtr, readFile(m_soapConfigPath)).WillOnce(
        Throw(Common::FileSystem::IFileSystemException("bang")));

    Tests::ScopedReplaceFileSystem replacer(std::move(m_mockIFileSystemPtr));

    ASSERT_EQ(OnAccessConfigMonitor::readConfigFile(), "");
}

TEST_F(TestOnAccessConfigMonitor, parseOnAccessSettingsFromJson) // NOLINT
{
    std::string jsonString = R"({"enabled":"true","excludeRemoteFiles":"false","exclusions":["/mnt/","/uk-filer5/"]})";

    ASSERT_EQ(OnAccessConfigMonitor::parseOnAccessSettingsFromJson(jsonString), true);
}