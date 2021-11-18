/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include <Telemetry/TelemetryImpl/TelemetryUtils.h>

#include <Common/FileSystem/IFileSystemException.h>
#include <Common/ApplicationConfiguration/IApplicationPathManager.h>

#include <tests/Common/Helpers/LogInitializedTests.h>
#include <tests/Common/Helpers/FileSystemReplaceAndRestore.h>
#include <tests/Common/Helpers/MockFileSystem.h>

#include <gtest/gtest.h>

class TestTelemetryUtils: public LogOffInitializedTests{};

TEST_F(TestTelemetryUtils, returnEmptyWhenFileDoesNotExist)
{
    std::string cloudPlatform = Telemetry::TelemetryUtils::getCloudPlatform();
    ASSERT_EQ(cloudPlatform, "");
}

TEST_F(TestTelemetryUtils, returnAWSWhenFileContainsaws)
{
    auto mockFileSystem = new StrictMock<MockFileSystem>();
    std::unique_ptr<MockFileSystem> mockIFileSystemPtr(mockFileSystem);
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockIFileSystemPtr));

    EXPECT_CALL(*mockFileSystem, isFile(_)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, readFile(_)).WillOnce(Return(R"({ "platform": "aws" })"));
    std::string cloudPlatform = Telemetry::TelemetryUtils::getCloudPlatform();
    ASSERT_EQ(cloudPlatform, "AWS");
}

TEST_F(TestTelemetryUtils, returnAzureWhenFileContainsazure)
{
    auto mockFileSystem = new StrictMock<MockFileSystem>();
    std::unique_ptr<MockFileSystem> mockIFileSystemPtr(mockFileSystem);
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockIFileSystemPtr));

    EXPECT_CALL(*mockFileSystem, isFile(_)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, readFile(_)).WillOnce(Return(R"({ "platform": "azure" })"));
    std::string cloudPlatform = Telemetry::TelemetryUtils::getCloudPlatform();
    ASSERT_EQ(cloudPlatform, "Azure");
}

TEST_F(TestTelemetryUtils, returnOracleWhenFileContainsoracle)
{
    auto mockFileSystem = new StrictMock<MockFileSystem>();
    std::unique_ptr<MockFileSystem> mockIFileSystemPtr(mockFileSystem);
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockIFileSystemPtr));

    EXPECT_CALL(*mockFileSystem, isFile(_)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, readFile(_)).WillOnce(Return(R"({ "platform": "oracle" })"));
    std::string cloudPlatform = Telemetry::TelemetryUtils::getCloudPlatform();
    ASSERT_EQ(cloudPlatform, "Oracle");
}

TEST_F(TestTelemetryUtils, returnGoogleWhenFileContainsgoogle)
{
    auto mockFileSystem = new StrictMock<MockFileSystem>();
    std::unique_ptr<MockFileSystem> mockIFileSystemPtr(mockFileSystem);
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockIFileSystemPtr));

    EXPECT_CALL(*mockFileSystem, isFile(_)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, readFile(_)).WillOnce(Return(R"({ "platform": "googlecloud" })"));
    std::string cloudPlatform = Telemetry::TelemetryUtils::getCloudPlatform();
    ASSERT_EQ(cloudPlatform, "Google");
}

TEST_F(TestTelemetryUtils, returnEmptyWhenFileContainsUnexpectedValue)
{
    auto mockFileSystem = new StrictMock<MockFileSystem>();
    std::unique_ptr<MockFileSystem> mockIFileSystemPtr(mockFileSystem);
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockIFileSystemPtr));

    EXPECT_CALL(*mockFileSystem, isFile(_)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, readFile(_)).WillOnce(Return(R"({ "platform": "someothercloudplatform" })"));
    std::string cloudPlatform = Telemetry::TelemetryUtils::getCloudPlatform();
    ASSERT_EQ(cloudPlatform, "");
}

TEST_F(TestTelemetryUtils, returnEmptyAndLogWarnWhenOnFileSystemException)
{
    auto mockFileSystem = new StrictMock<MockFileSystem>();
    std::unique_ptr<MockFileSystem> mockIFileSystemPtr(mockFileSystem);
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockIFileSystemPtr));
    Common::Logging::ConsoleLoggingSetup consoleLogger;
    testing::internal::CaptureStderr();

    std::stringstream expectedWarnMessageStream;
    expectedWarnMessageStream << "Could not access " << Common::ApplicationConfiguration::applicationPathManager().getCloudMetadataJsonPath() << " due to error: Test exception";
    std::string expectedWarnMessage = expectedWarnMessageStream.str();

    EXPECT_CALL(*mockFileSystem, isFile(_)).WillOnce(Throw(Common::FileSystem::IFileSystemException("Test exception")));
    std::string cloudPlatform = Telemetry::TelemetryUtils::getCloudPlatform();
    ASSERT_EQ(cloudPlatform, "");

    std::string logMessage = testing::internal::GetCapturedStderr();
    ASSERT_THAT(logMessage, ::testing::HasSubstr(expectedWarnMessage));
}
