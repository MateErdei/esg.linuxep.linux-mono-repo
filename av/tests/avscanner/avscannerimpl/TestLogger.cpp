#include <gtest/gtest.h>

#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>
#include <tests/common/LogInitializedTests.h>

#include "avscanner/avscannerimpl/Logger.h"
#include "datatypes/sophos_filesystem.h"

#include <log4cplus/logger.h>

namespace fs = sophos_filesystem;

namespace
{
    class TestLogger : public LogInitializedTests
    {
    protected:
        void SetUp() override
        {
            const ::testing::TestInfo* const test_info = ::testing::UnitTest::GetInstance()->current_test_info();
            m_basePath = fs::temp_directory_path() / test_info->test_case_name() / test_info->name();
            fs::remove_all(m_basePath);
            fs::create_directories(m_basePath);
        }

        void TearDown() override
        {
            fs::remove_all(m_basePath);
        }

        fs::path m_basePath;
    };
}

TEST_F(TestLogger, TestConstructor) // NOLINT
{
    // set up ApplicationConfiguration so that Logger knows where to put its files
    auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
    appConfig.setData(Common::ApplicationConfiguration::SOPHOS_INSTALL, m_basePath);

    EXPECT_NO_THROW(Logger("test_logger"));
    EXPECT_TRUE(fs::is_regular_file(m_basePath / "plugins/av/log/test_logger.log"));
}

TEST_F(TestLogger, TestConstructorForCommandLine) // NOLINT
{
    EXPECT_NO_THROW(Logger(m_basePath / "test_logger.txt", log4cplus::INFO_LOG_LEVEL, true));
    EXPECT_TRUE(fs::is_regular_file(m_basePath / "test_logger.txt"));
}