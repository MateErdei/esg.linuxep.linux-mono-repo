// Copyright 2022, Sophos Limited.  All rights reserved.

#include "safestore/SafeStoreServiceCallback.h"

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
#include "Common/Helpers/FileSystemReplaceAndRestore.h"
#include "Common/Helpers/LogInitializedTests.h"
#include "Common/Helpers/MockFileSystem.h"
#include "common/ApplicationPaths.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;

class TestSafeStoreServiceCallback : public LogInitializedTests
{
protected:
    void SetUp() override
    {
        auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
        appConfig.setData("SOPHOS_INSTALL", "/tmp");
        appConfig.setData("PLUGIN_INSTALL", "/tmp/av");
    }
};

TEST_F(TestSafeStoreServiceCallback, SafeStoreTelemetryReturnsExpectedData) // NOLINT
{
    safestore::SafeStoreServiceCallback safeStoreCallback{};

    auto mockFileSystem = new StrictMock<MockFileSystem>();
    std::unique_ptr<MockFileSystem> mockIFileSystemPtr(mockFileSystem);
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockIFileSystemPtr));
    EXPECT_CALL(*mockFileSystem, isFile(Plugin::getSafeStoreDormantFlagPath())).WillOnce(Return(false));


    EXPECT_EQ(safeStoreCallback.getTelemetry(), "{\"dormant-mode\":false,\"health\":0}");
}

TEST_F(TestSafeStoreServiceCallback, SafeStoreTelemetryReturnsExpectedDataWhenSafeStoreIsInDormantMode) // NOLINT
{
    safestore::SafeStoreServiceCallback safeStoreCallback{};

    auto mockFileSystem = new StrictMock<MockFileSystem>();
    std::unique_ptr<MockFileSystem> mockIFileSystemPtr(mockFileSystem);
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockIFileSystemPtr));
    EXPECT_CALL(*mockFileSystem, isFile(Plugin::getSafeStoreDormantFlagPath())).WillOnce(Return(true));


    EXPECT_EQ(safeStoreCallback.getTelemetry(), "{\"dormant-mode\":true,\"health\":1}");
}