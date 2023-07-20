// Copyright 2023 Sophos Limited. All rights reserved.

# define TEST_PUBLIC public

#include "PluginMemoryAppenderUsingTests.h"

#include "common/ApplicationPaths.h"
#include "pluginimpl/HealthStatus.h"

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
#include "Common/FileSystem/IFileNotFoundException.h"
#include "Common/Helpers/FileSystemReplaceAndRestore.h"
#include "Common/Helpers/MockFileSystem.h"

namespace
{
    class TestHealthStatus : public PluginMemoryAppenderUsingTests
    {
    public:
        void SetUp() override
        {
            setPluginInstall();
            mockFileSystem_ = std::make_unique<StrictMock<MockFileSystem>>();
            statusFilePath_ = Plugin::getThreatDetectorSusiUpdateStatusPath();
        }

        void setPluginInstall(const char* pluginInstall="/opt/sophos-spl/plugins/av")
        {
            auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
            appConfig.setData("PLUGIN_INSTALL", pluginInstall);
        }

        std::unique_ptr<MockFileSystem> mockFileSystem_;
        std::string statusFilePath_;
    };
}

TEST_F(TestHealthStatus, susiUpdateFailed_absentFile)
{
    EXPECT_CALL(*mockFileSystem_, readFile(StrEq(statusFilePath_))).WillOnce(Throw(Common::FileSystem::IFileNotFoundException("FOOBAR")));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{std::move(mockFileSystem_)};
    EXPECT_FALSE(Plugin::susiUpdateFailed());
}

TEST_F(TestHealthStatus, susiUpdateFailed_emptyFile)
{
    EXPECT_CALL(*mockFileSystem_, readFile(StrEq(statusFilePath_))).WillOnce(Return(""));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{std::move(mockFileSystem_)};
    EXPECT_FALSE(Plugin::susiUpdateFailed());
}

TEST_F(TestHealthStatus, susiUpdateFailed_invalid_json)
{
    EXPECT_CALL(*mockFileSystem_, readFile(StrEq(statusFilePath_))).WillOnce(Return("FOOBAR"));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{std::move(mockFileSystem_)};
    EXPECT_FALSE(Plugin::susiUpdateFailed());
}

TEST_F(TestHealthStatus, susiUpdateFailed_emptyMap)
{
    EXPECT_CALL(*mockFileSystem_, readFile(StrEq(statusFilePath_))).WillOnce(Return("{}"));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{std::move(mockFileSystem_)};
    EXPECT_FALSE(Plugin::susiUpdateFailed());
}

TEST_F(TestHealthStatus, susiUpdateFailed_incorrectTypeForSuccessKey)
{
    EXPECT_CALL(*mockFileSystem_, readFile(StrEq(statusFilePath_))).WillOnce(Return("{\"success\":5}"));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{std::move(mockFileSystem_)};
    EXPECT_FALSE(Plugin::susiUpdateFailed());
}

TEST_F(TestHealthStatus, susiUpdateFailed_true)
{
    EXPECT_CALL(*mockFileSystem_, readFile(StrEq(statusFilePath_))).WillOnce(Return("{\"success\":false}"));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{std::move(mockFileSystem_)};
    EXPECT_TRUE(Plugin::susiUpdateFailed());
}

TEST_F(TestHealthStatus, susiUpdateFailed_false)
{
    EXPECT_CALL(*mockFileSystem_, readFile(StrEq(statusFilePath_))).WillOnce(Return("{\"success\":true}"));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{std::move(mockFileSystem_)};
    EXPECT_FALSE(Plugin::susiUpdateFailed());
}
