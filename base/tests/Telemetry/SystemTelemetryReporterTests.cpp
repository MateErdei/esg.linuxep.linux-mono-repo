// Copyright 2019-2023 Sophos Limited. All rights reserved.

#include "tests/Telemetry/MockSystemTelemetryCollector.h"

#include "Common/Logging/ConsoleLoggingSetup.h"
#include "Telemetry/TelemetryImpl/SystemTelemetryReporter.h"
#include "Common/Process/IProcess.h"
#include "tests/Common/Helpers/FileSystemReplaceAndRestore.h"
#include "tests/Common/Helpers/MockFileSystem.h"
#include <gmock/gmock-matchers.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <map>
#include <regex>
#include <utility>
#include <vector>

using ::testing::Return;
class SystemTelemetryReporterTests : public ::testing::Test
{
public:
    void TearDown() override {}

    Common::Logging::ConsoleLoggingSetup m_loggingSetup;
};

TEST_F(SystemTelemetryReporterTests, getTelemetryEmptyOK)
{
    std::vector<std::string> osReleaseContents = {"NAME=\"Ubuntu\"",
                                            "VERSION=\"20.04.6 LTS (Focal Fossa)\"",
                                            "ID=ubuntu",
                                            "ID_LIKE=debian",
                                            "PRETTY_NAME=\"Ubuntu 20.04.6 LTS\"",
                                            "VERSION_ID=20.04",
                                            "HOME_URL=\"https://www.ubuntu.com/\"",
                                            "SUPPORT_URL=\"https://help.ubuntu.com/\"",
                                            "BUG_REPORT_URL=\"https://bugs.launchpad.net/ubuntu/\"",
                                            "PRIVACY_POLICY_URL=\"https://www.ubuntu.com/legal/terms-and-policies/privacy-policy\"",
                                            "VERSION_CODENAME=focal",
                                            "UBUNTU_CODENAME=focal"};
    auto filesystemMock = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*filesystemMock, isFile("/etc/os-release")).Times(4).WillRepeatedly(Return(true));
    EXPECT_CALL(*filesystemMock, isFile("/opt/sophos-spl/base/etc/sophosspl/instance-metadata.json")).WillRepeatedly(Return(false));
    EXPECT_CALL(*filesystemMock, isFile("/opt/sophos-spl/base/etc/sophosspl/current_proxy")).WillRepeatedly(Return(false));
    EXPECT_CALL(*filesystemMock, readLines("/etc/os-release")).Times(3).WillRepeatedly(Return(osReleaseContents));
    auto scopedReplaceFileSystem = std::make_unique<Tests::ScopedReplaceFileSystem>(std::move(filesystemMock));

    std::map<std::string, Telemetry::TelemetryItem> emptySimpleObjs;
    std::map<std::string, std::vector<Telemetry::TelemetryItem>> emptyArrayObjs;

    std::unique_ptr<MockSystemTelemetryCollector> mockCollector(new MockSystemTelemetryCollector());

    EXPECT_CALL(*mockCollector, collectObjects()).WillOnce(Return(emptySimpleObjs));
    EXPECT_CALL(*mockCollector, collectArraysOfObjects()).WillOnce(Return(emptyArrayObjs));

    Telemetry::SystemTelemetryReporter reporter(std::move(mockCollector));
    auto sysTelemetryJSON = reporter.getTelemetry();
    ASSERT_EQ(sysTelemetryJSON, "{\"mcs-connection\":\"Direct\",\"os-name\":\"ubuntu\",\"os-version\":\"20.04\"}");

    scopedReplaceFileSystem.reset();
}

TEST_F(SystemTelemetryReporterTests, getTelemetryOk)
{
    std::map<std::string, Telemetry::TelemetryItem> simpleObjs = {
        { "test-simple-string", { { "", { "test-version 10.5 string" } } } }, { "test-simple-int", { { "", { 101 } } } }
    };

    std::map<std::string, std::vector<Telemetry::TelemetryItem>> arrayObjs = {
        { "test-array",
          { { { "fstype", { "testvalue1" } }, { "free", { 201 } } },
            { { "fstype", { "testvalue2" } }, { "free", { 202 } } } } }
    };

    std::unique_ptr<MockSystemTelemetryCollector> mockCollector(new MockSystemTelemetryCollector());

    EXPECT_CALL(*mockCollector, collectObjects()).WillOnce(Return(simpleObjs));
    EXPECT_CALL(*mockCollector, collectArraysOfObjects()).WillOnce(Return(arrayObjs));

    Telemetry::SystemTelemetryReporter reporter(std::move(mockCollector));
    auto sysTelemetryJSON = reporter.getTelemetry();

    ASSERT_TRUE(sysTelemetryJSON.find("test-simple-string") != std::string::npos);
    ASSERT_TRUE(sysTelemetryJSON.find("test-simple-int") != std::string::npos);
    ASSERT_TRUE(sysTelemetryJSON.find("test-array") != std::string::npos);
}

TEST_F(SystemTelemetryReporterTests, getTelemetryThrowsButNoTopLevelExpectedException)
{
    std::map<std::string, Telemetry::TelemetryItem> simpleObjs = { { "test-unexpected-object",
                                                                     { { "unexpected-name", { 42 } } } } };

    std::unique_ptr<MockSystemTelemetryCollector> mockCollector(new MockSystemTelemetryCollector());

    EXPECT_CALL(*mockCollector, collectObjects()).WillOnce(Return(simpleObjs));

    Telemetry::SystemTelemetryReporter reporter(std::move(mockCollector));

    ASSERT_ANY_THROW(reporter.getTelemetry());
}
