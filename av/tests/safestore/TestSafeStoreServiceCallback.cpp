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

    std::vector<std::string> fileList{"safestore.db", "safestore.pw"};

    EXPECT_CALL(*mockFileSystem, isFile(Plugin::getSafeStoreDormantFlagPath())).WillOnce(Return(false));
    EXPECT_CALL(*mockFileSystem, listFiles(Plugin::getSafeStoreDbDirPath())).WillOnce(Return(fileList));
    EXPECT_CALL(*mockFileSystem, fileSize(_)).WillRepeatedly(Return(150));

    EXPECT_EQ(safeStoreCallback.getTelemetry(), "{\"database-size\":300,\"dormant-mode\":false,\"health\":0,\"quarantine-failures\":0,\"quarantine-successes\":0,\"unlink-failures\":0}");
}

TEST_F(TestSafeStoreServiceCallback, SafeStoreTelemetryReturnsExpectedDataWhenSafeStoreIsInDormantMode) // NOLINT
{
    safestore::SafeStoreServiceCallback safeStoreCallback{};

    auto mockFileSystem = new StrictMock<MockFileSystem>();
    std::unique_ptr<MockFileSystem> mockIFileSystemPtr(mockFileSystem);
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockIFileSystemPtr));

    std::vector<std::string> fileList{"safestore.db", "safestore.pw"};

    EXPECT_CALL(*mockFileSystem, isFile(Plugin::getSafeStoreDormantFlagPath())).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, listFiles(Plugin::getSafeStoreDbDirPath())).WillOnce(Return(fileList));
    EXPECT_CALL(*mockFileSystem, fileSize(_)).WillRepeatedly(Return(150));


    EXPECT_EQ(safeStoreCallback.getTelemetry(), "{\"database-size\":300,\"dormant-mode\":true,\"health\":1,\"quarantine-failures\":0,\"quarantine-successes\":0,\"unlink-failures\":0}");
}

TEST_F(TestSafeStoreServiceCallback, SafeStoreTelemetryReturnsExpectedDataWhenSafeStoreDatabaseIsEmpty) // NOLINT
{
    safestore::SafeStoreServiceCallback safeStoreCallback{};

    auto mockFileSystem = new StrictMock<MockFileSystem>();
    std::unique_ptr<MockFileSystem> mockIFileSystemPtr(mockFileSystem);
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockIFileSystemPtr));

    std::vector<std::string> fileList{};

    EXPECT_CALL(*mockFileSystem, isFile(Plugin::getSafeStoreDormantFlagPath())).WillOnce(Return(false));
    EXPECT_CALL(*mockFileSystem, listFiles(Plugin::getSafeStoreDbDirPath())).WillOnce(Return(fileList));
    EXPECT_CALL(*mockFileSystem, fileSize(_)).Times(0);

    EXPECT_EQ(safeStoreCallback.getTelemetry(), "{\"database-size\":0,\"dormant-mode\":false,\"health\":0,\"quarantine-failures\":0,\"quarantine-successes\":0,\"unlink-failures\":0}");
}

TEST_F(TestSafeStoreServiceCallback, SafeStoreTelemetryReturnsExpectedDataWhenSafeStoreDatabaseSizeHasNoValue) // NOLINT
{
    safestore::SafeStoreServiceCallback safeStoreCallback{};

    auto mockFileSystem = new StrictMock<MockFileSystem>();
    std::unique_ptr<MockFileSystem> mockIFileSystemPtr(mockFileSystem);
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockIFileSystemPtr));

    EXPECT_CALL(*mockFileSystem, isFile(Plugin::getSafeStoreDormantFlagPath())).WillOnce(Return(false));
    EXPECT_CALL(*mockFileSystem, listFiles(Plugin::getSafeStoreDbDirPath())).WillOnce(Throw(std::exception{}));

    EXPECT_EQ(safeStoreCallback.getTelemetry(), "{\"dormant-mode\":false,\"health\":0,\"quarantine-failures\":0,\"quarantine-successes\":0,\"unlink-failures\":0}");
}