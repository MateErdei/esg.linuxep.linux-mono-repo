/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "common/LogInitializedTests.h"
#include "datatypes/sophos_filesystem.h"
#include "sophos_on_access_process/OnAccessConfig/OnAccessConfigurationUtils.h"

#include "Common/FileSystem/IFileSystemException.h"
#include "Common/Helpers/FileSystemReplaceAndRestore.h"
#include "Common/Helpers/MockFileSystem.h"

#include <gtest/gtest.h>

using namespace sophos_on_access_process::OnAccessConfig;
namespace fs = sophos_filesystem;

class TestOnAccessConfigUtils : public LogInitializedTests
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

TEST_F(TestOnAccessConfigUtils, readConfigFile)
{
    EXPECT_CALL(*m_mockIFileSystemPtr, readFile(m_soapConfigPath)).WillOnce(Return("x"));

    Tests::ScopedReplaceFileSystem replacer(std::move(m_mockIFileSystemPtr));

    ASSERT_EQ(readConfigFile(), "x");
}

TEST_F(TestOnAccessConfigUtils, readConfigFileReadThrows)
{
    EXPECT_CALL(*m_mockIFileSystemPtr, readFile(m_soapConfigPath)).WillOnce(
        Throw(Common::FileSystem::IFileSystemException("bang")));

    Tests::ScopedReplaceFileSystem replacer(std::move(m_mockIFileSystemPtr));

    ASSERT_EQ(readConfigFile(), "");
}

TEST_F(TestOnAccessConfigUtils, parseOnAccessSettingsFromJson)
{
    std::string jsonString = R"({"enabled":"true","excludeRemoteFiles":"false","exclusions":["/mnt/","/uk-filer5/"]})";

    OnAccessConfiguration expectedResult;
    expectedResult.enabled = true;
    expectedResult.excludeRemoteFiles = false;
    expectedResult.exclusions = {"/mnt/", "/uk-filer5/"};

    ASSERT_EQ(parseOnAccessSettingsFromJson(jsonString), expectedResult);

    expectedResult.enabled = false;
    expectedResult.excludeRemoteFiles = true;
    expectedResult.exclusions = {"/uk-filer5/", "/mnt/"};

    ASSERT_NE(parseOnAccessSettingsFromJson(jsonString), expectedResult);
}

TEST_F(TestOnAccessConfigUtils, parseOnAccessSettingsFromJsonInvalidJson)
{
    std::string jsonString = R"({"enabled":"I think","excludeRemoteFiles":"therefore","exclusions":["I","am"]})";

    OnAccessConfiguration expectedResult;
    expectedResult.enabled = false;
    expectedResult.excludeRemoteFiles = false;
    expectedResult.exclusions = {"I", "am"};

    ASSERT_EQ(parseOnAccessSettingsFromJson(jsonString), expectedResult);
}

TEST_F(TestOnAccessConfigUtils, parseOnAccessSettingsFromJsonInvalidJsonSyntax)
{
    std::string jsonString = R"(this is going to break)";

    OnAccessConfiguration expectedResult {};

    ASSERT_EQ(parseOnAccessSettingsFromJson(jsonString), expectedResult);
}