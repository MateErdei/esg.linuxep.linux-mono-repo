// Copyright 2022, Sophos Limited.  All rights reserved.

#include "safestore/SafeStoreServiceCallback.h"

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
#include "Common/FileSystem/IFileSystemException.h"
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

TEST_F(TestSafeStoreServiceCallback, SafeStoreTelemetryReturnsExpectedData)
{
    safestore::SafeStoreServiceCallback safeStoreCallback{};

    auto mockFileSystem = new StrictMock<MockFileSystem>();
    std::unique_ptr<MockFileSystem> mockIFileSystemPtr(mockFileSystem);
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockIFileSystemPtr));

    std::vector<std::string> fileList{"safestore.db", "safestore.pw"};

    EXPECT_CALL(*mockFileSystem, isFile(Plugin::getSafeStoreDormantFlagPath())).WillOnce(Return(false));
    EXPECT_CALL(*mockFileSystem, listFiles(Plugin::getSafeStoreDbDirPath())).WillOnce(Return(fileList));
    EXPECT_CALL(*mockFileSystem, fileSize(_)).Times(fileList.size()).WillRepeatedly(Return(150));
    
    EXPECT_EQ(safeStoreCallback.getTelemetry(), R"({"database-size":300,"dormant-mode":false,"health":0})");
}

TEST_F(TestSafeStoreServiceCallback, SafeStoreTelemetryReturnsExpectedDataWhenSafeStoreIsInDormantMode)
{
    safestore::SafeStoreServiceCallback safeStoreCallback{};

    auto mockFileSystem = new StrictMock<MockFileSystem>();
    std::unique_ptr<MockFileSystem> mockIFileSystemPtr(mockFileSystem);
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockIFileSystemPtr));

    std::vector<std::string> fileList{"safestore.db", "safestore.pw"};

    EXPECT_CALL(*mockFileSystem, isFile(Plugin::getSafeStoreDormantFlagPath())).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, listFiles(Plugin::getSafeStoreDbDirPath())).WillOnce(Return(fileList));
    EXPECT_CALL(*mockFileSystem, fileSize(_)).Times(fileList.size()).WillRepeatedly(Return(150));
    
    EXPECT_EQ(safeStoreCallback.getTelemetry(), R"({"database-size":300,"dormant-mode":true,"health":1})");
}

TEST_F(TestSafeStoreServiceCallback, SafeStoreTelemetryReturnsExpectedDataWhenSafeStoreDatabaseIsEmpty)
{
    safestore::SafeStoreServiceCallback safeStoreCallback{};

    auto mockFileSystem = new StrictMock<MockFileSystem>();
    std::unique_ptr<MockFileSystem> mockIFileSystemPtr(mockFileSystem);
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockIFileSystemPtr));

    std::vector<std::string> fileList{};

    EXPECT_CALL(*mockFileSystem, isFile(Plugin::getSafeStoreDormantFlagPath())).WillOnce(Return(false));
    EXPECT_CALL(*mockFileSystem, listFiles(Plugin::getSafeStoreDbDirPath())).WillOnce(Return(fileList));
    EXPECT_CALL(*mockFileSystem, fileSize(_)).Times(0);

    EXPECT_EQ(safeStoreCallback.getTelemetry(), R"({"database-size":0,"dormant-mode":false,"health":0})");
}

TEST_F(TestSafeStoreServiceCallback, SafeStoreTelemetryReturnsExpectedDataWhenSafeStoreDatabaseSizeHasNoValue)
{
    safestore::SafeStoreServiceCallback safeStoreCallback{};

    auto mockFileSystem = new StrictMock<MockFileSystem>();
    std::unique_ptr<MockFileSystem> mockIFileSystemPtr(mockFileSystem);
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockIFileSystemPtr));

    EXPECT_CALL(*mockFileSystem, isFile(Plugin::getSafeStoreDormantFlagPath())).WillOnce(Return(false));
    EXPECT_CALL(*mockFileSystem, listFiles(Plugin::getSafeStoreDbDirPath())).WillOnce(Throw(Common::FileSystem::IFileSystemException("")));

    EXPECT_EQ(safeStoreCallback.getTelemetry(), R"({"dormant-mode":false,"health":0})");
}

TEST_F(TestSafeStoreServiceCallback, SafeStoreTelemetryReturnsExpectedDataWhenSafeStoreDatabaseFileDoesNotExist)
{
    safestore::SafeStoreServiceCallback safeStoreCallback{};

    auto mockFileSystem = new StrictMock<MockFileSystem>();
    std::unique_ptr<MockFileSystem> mockIFileSystemPtr(mockFileSystem);
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockIFileSystemPtr));

    std::vector<std::string> fileList{"safestore.db", "safestore.pw"};

    EXPECT_CALL(*mockFileSystem, isFile(Plugin::getSafeStoreDormantFlagPath())).WillOnce(Return(false));
    EXPECT_CALL(*mockFileSystem, listFiles(Plugin::getSafeStoreDbDirPath())).WillOnce(Return(fileList));
    EXPECT_CALL(*mockFileSystem, fileSize(_))
        .Times(fileList.size())
        .WillOnce(Return(150))
        .WillOnce(Return(-1));

    EXPECT_EQ(safeStoreCallback.getTelemetry(), R"({"database-size":150,"dormant-mode":false,"health":0})");
}