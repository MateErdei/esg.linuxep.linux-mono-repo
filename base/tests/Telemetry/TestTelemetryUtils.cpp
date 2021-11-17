/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include <Telemetry/TelemetryImpl/TelemetryUtils.h>


#include <tests/Common/Helpers/LogInitializedTests.h>
#include <tests/Common/Helpers/FileSystemReplaceAndRestore.h>
#include <tests/Common/Helpers/MockFileSystem.h>
#include <tests/Common/Helpers/MockFilePermissions.h>
#include <gtest/gtest.h>
#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>

class TestTelemetryUtils: public LogOffInitializedTests{};

TEST_F(TestTelemetryUtils, returnEmptyWhenFileDoesNotExist)
{
    std::string cloudPlatform = Telemetry::TelemetryUtils::getCloudPlatform();
    ASSERT_EQ(cloudPlatform, "");
}

TEST_F(TestTelemetryUtils, returnAWSWhenFilecontainsaws)
{

    auto mockFileSystem = new StrictMock<MockFileSystem>();
    std::unique_ptr<MockFileSystem> mockIFileSystemPtr(mockFileSystem);
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockIFileSystemPtr));

    EXPECT_CALL(*mockFileSystem, isFile(_)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, readFile(_)).WillOnce(Return(R"({ "platform": "aws" })"));
    std::string cloudPlatform = Telemetry::TelemetryUtils::getCloudPlatform();
    ASSERT_EQ(cloudPlatform, "AWS");
}

TEST_F(TestTelemetryUtils, returnAzureWhenFilecontainsazure)
{

    auto mockFileSystem = new StrictMock<MockFileSystem>();
    std::unique_ptr<MockFileSystem> mockIFileSystemPtr(mockFileSystem);
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockIFileSystemPtr));

    EXPECT_CALL(*mockFileSystem, isFile(_)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, readFile(_)).WillOnce(Return(R"({ "platform": "azure" })"));
    std::string cloudPlatform = Telemetry::TelemetryUtils::getCloudPlatform();
    ASSERT_EQ(cloudPlatform, "Azure");
}

TEST_F(TestTelemetryUtils, returnOracleWhenFilecontainsoracle)
{

    auto mockFileSystem = new StrictMock<MockFileSystem>();
    std::unique_ptr<MockFileSystem> mockIFileSystemPtr(mockFileSystem);
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockIFileSystemPtr));

    EXPECT_CALL(*mockFileSystem, isFile(_)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, readFile(_)).WillOnce(Return(R"({ "platform": "oracle" })"));
    std::string cloudPlatform = Telemetry::TelemetryUtils::getCloudPlatform();
    ASSERT_EQ(cloudPlatform, "Oracle");
}

TEST_F(TestTelemetryUtils, returnGoogleWhenFilecontainsgoogle)
{

    auto mockFileSystem = new StrictMock<MockFileSystem>();
    std::unique_ptr<MockFileSystem> mockIFileSystemPtr(mockFileSystem);
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockIFileSystemPtr));

    EXPECT_CALL(*mockFileSystem, isFile(_)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, readFile(_)).WillOnce(Return(R"({ "platform": "googlecloud" })"));
    std::string cloudPlatform = Telemetry::TelemetryUtils::getCloudPlatform();
    ASSERT_EQ(cloudPlatform, "Google");
}