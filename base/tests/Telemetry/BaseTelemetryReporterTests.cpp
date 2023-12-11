// Copyright 2019-2023 Sophos Limited. All rights reserved.

#define TEST_PUBLIC public

#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "Common/FileSystem/IFileSystemException.h"
#include "Common/FileSystem/IFileTooLargeException.h"
#include "Common/Logging/ConsoleLoggingSetup.h"
#include "Telemetry/TelemetryImpl/BaseTelemetryReporter.h"
#include "tests/Common/Helpers/FilePermissionsReplaceAndRestore.h"
#include "tests/Common/Helpers/FileSystemReplaceAndRestore.h"
#include "tests/Common/Helpers/MockFilePermissions.h"
#include "tests/Common/Helpers/MockFileSystem.h"

#include <gmock/gmock-matchers.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <nlohmann/json.hpp>
#include <map>
#include <regex>
#include <utility>
#include <vector>

using ::testing::Return;
using ::testing::StrictMock;

#define PRINT(x) std::cerr << x << '\n'

struct TelemetryTestCase
{
    TelemetryTestCase(  std::string iXml, std::string value):
            inputXml(std::move(iXml)),
            expectedValue(std::move(value))
    {

    }
    std::string inputXml;
    std::string expectedValue;
};

class BaseTelemetryReporterTests : public ::testing::Test
{
public:
    Common::Logging::ConsoleLoggingSetup m_loggingSetup;
    std::unique_ptr<IFileSystem> m_fileSystem;
};

class OutbreakTelemetryTests : public BaseTelemetryReporterTests
{
};

TEST_F(BaseTelemetryReporterTests, extractCustomerIdOK) // NOLINT
{
    auto testCase = TelemetryTestCase{ R"(<?xml version="1.0"?>
<AUConfigurations xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:csc="com.sophos\msys\csc" xmlns="http://www.sophos.com/EE/AUConfig">
  <csc:Comp RevID="b6a8fe2c0ce016c949016a5da2b7a089699271290ef7205d5bea0986768485d9" policyType="1"/>
  <customer id="4b4ca3ba-c144-4447-8050-6c96a7104c11"/>
</AUConfigurations>)", "4b4ca3ba-c144-4447-8050-6c96a7104c11"};

    auto customerId = Telemetry::extractCustomerId(testCase.inputXml);
    EXPECT_TRUE( customerId.has_value());
    EXPECT_EQ(testCase.expectedValue, customerId.value());
}

TEST_F(BaseTelemetryReporterTests, extractCustomerIdXmlAttributeNoAvailable) // NOLINT
{
    std::string policySnippet =  R"(<?xml version="1.0"?>
<AUConfigurations xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:csc="com.sophos\msys\csc" xmlns="http://www.sophos.com/EE/AUConfig">
  <csc:Comp RevID="b6a8fe2c0ce016c949016a5da2b7a089699271290ef7205d5bea0986768485d9" policyType="1"/>
</AUConfigurations>)";
    auto customerId = Telemetry::extractCustomerId(policySnippet);
    EXPECT_FALSE( customerId.has_value());
}

TEST_F(BaseTelemetryReporterTests, extractCustomerIdWrongIdEntry) // NOLINT
{
    std::string policySnippet = R"(<?xml version="1.0"?>
<AUConfigurations xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:csc="com.sophos\msys\csc" xmlns="http://www.sophos.com/EE/AUConfig">
  <csc:Comp RevID="b6a8fe2c0ce016c949016a5da2b7a089699271290ef7205d5bea0986768485d9" policyType="1"/>
  <customer >
    <entry id="dontpickkthis"/>
  </customer>
</AUConfigurations>)";

    auto customerId = Telemetry::extractCustomerId(policySnippet);
    EXPECT_FALSE( customerId.has_value());
}

TEST_F(BaseTelemetryReporterTests, extractCustomerIdInvalidXml) // NOLINT
{
    std::string policySnippet = R"(<?xml version="1.0"?>
<AUConfigurations xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:csc="com.sophos\msys\csc" xmlns="http://www.sophos.com/EE/AUConfig">
  <csc:Comp RevID="b6a8fe2c0ce016c949016a5da2b7a089699271290ef7205d5bea0986768485d9" policyType="1"/>
  <customer id="4b4ca3ba-c144-4447-8050-6c96a7104c11">
    <entry id="dontpickkthis"/>
</AUConfigurations>)";

    auto customerId = Telemetry::extractCustomerId(policySnippet);
    EXPECT_FALSE( customerId.has_value());
}

TEST_F(BaseTelemetryReporterTests, extractValueFromIniFileMissingIniFile) // NOLINT
{
    std::unique_ptr<MockFileSystem> mockfileSystemUniquePtr(new StrictMock<MockFileSystem>());
    MockFileSystem* mockFileSystemRawptr = mockfileSystemUniquePtr.get();
    Tests::replaceFileSystem(std::move(mockfileSystemUniquePtr));

    std::string testFilePath("testfilepath");
    EXPECT_CALL(*mockFileSystemRawptr, isFile(testFilePath)).WillOnce(Return(false));
    auto endPointId = Telemetry::extractValueFromFile(testFilePath, "endpointId");
    EXPECT_FALSE( endPointId.has_value());
}

TEST_F(BaseTelemetryReporterTests, extractOverallHealthInvalidXml) // NOLINT
{
    testing::internal::CaptureStderr();

    std::string shsStatusXml =
        R"(<?xml version="1.0" encoding="utf-8" ?>
            <health version="3.0.0" activeHeartbeat="false" activeHeartbeatUtmId="">
                <item name="health" value="1" />
                <item name="service" value="1" >
                    <detail name="Sophos MCS Client" value="0" />
                    <detail name="Sophos Linux AntiVirus" value="0" />
                    <detail name="Update Scheduler" value="0" />
                </item>
                <item name="threatService" value="1" >
                    <detail name="Sophos MCS Client" value="0" />
                    <detail name="Sophos Linux AntiVirus" value="0" />
                    <detail name="Update Scheduler" value="0" />
                </item>
                <item name="threat" value="1" />
        )";

    auto overallHealth = Telemetry::extractOverallHealth(shsStatusXml);

    std::string logMessage = testing::internal::GetCapturedStderr();
    EXPECT_THAT(logMessage, ::testing::HasSubstr("WARN Invalid status XML received. Error: Error parsing xml: "));
    ASSERT_FALSE( overallHealth.has_value());
}

TEST_F(BaseTelemetryReporterTests, extractOverallHealthNoItemsInXml) // NOLINT
{
    testing::internal::CaptureStderr();

    std::string shsStatusXml =
        R"(<?xml version="1.0" encoding="utf-8" ?>
            <health version="3.0.0" activeHeartbeat="false" activeHeartbeatUtmId="">
            </health>
        )";

    auto overallHealth = Telemetry::extractOverallHealth(shsStatusXml);

    std::string logMessage = testing::internal::GetCapturedStderr();
    EXPECT_THAT(logMessage, ::testing::HasSubstr("WARN No Health items present in the SHS status XML"));
    ASSERT_FALSE( overallHealth.has_value());
}

TEST_F(BaseTelemetryReporterTests, extractOverallHealthExpectedNameNotInXml) // NOLINT
{
    testing::internal::CaptureStderr();

    std::string shsStatusXml =
        R"(<?xml version="1.0" encoding="utf-8" ?>
            <health version="3.0.0" activeHeartbeat="false" activeHeartbeatUtmId="">
                <item name="service" value="1" >
                    <detail name="Sophos MCS Client" value="0" />
                    <detail name="Sophos Linux AntiVirus" value="0" />
                    <detail name="Update Scheduler" value="0" />
                </item>
                <item name="threatService" value="1" >
                    <detail name="Sophos MCS Client" value="0" />
                    <detail name="Sophos Linux AntiVirus" value="0" />
                    <detail name="Update Scheduler" value="0" />
                </item>
                <item name="threat" value="1" />
            </health>
        )";

    auto overallHealth = Telemetry::extractOverallHealth(shsStatusXml);

    std::string logMessage = testing::internal::GetCapturedStderr();
    EXPECT_THAT(logMessage, ::testing::HasSubstr("WARN Overall Health not present in the SHS status XML"));
    ASSERT_FALSE( overallHealth.has_value());
}

TEST_F(BaseTelemetryReporterTests, extractOverallHealthFindsOverallHealthXml) // NOLINT
{
    std::string shsStatusXml =
        R"(<?xml version="1.0" encoding="utf-8" ?>
            <health version="3.0.0" activeHeartbeat="false" activeHeartbeatUtmId="">
                <item name="health" value="1" />
                <item name="service" value="1" >
                    <detail name="Sophos MCS Client" value="0" />
                    <detail name="Sophos Linux AntiVirus" value="0" />
                    <detail name="Update Scheduler" value="0" />
                </item>
                <item name="threatService" value="1" >
                    <detail name="Sophos MCS Client" value="0" />
                    <detail name="Sophos Linux AntiVirus" value="0" />
                    <detail name="Update Scheduler" value="0" />
                </item>
                <item name="threat" value="1" />
            </health>
        )";

    auto overallHealth = Telemetry::extractOverallHealth(shsStatusXml);
    std::string expectedValue = "1";
    ASSERT_EQ(overallHealth, expectedValue);
}

TEST_F(BaseTelemetryReporterTests, parseOutbreakStatusWhenFileDoesNotExist)
{
    auto mockFileSystem =  std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(
        *mockFileSystem,
        isFile(Common::ApplicationConfiguration::applicationPathManager().getOutbreakModeStatusFilePath()))
        .WillOnce(Return(false));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockFileSystem));
    EXPECT_EQ(Telemetry::parseOutbreakStatusFile(), std::nullopt);
}

TEST_F(BaseTelemetryReporterTests, parseOutbreakStatusCatchesFileSystemException)
{
    testing::internal::CaptureStderr();
    auto mockFileSystem =  std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(
        *mockFileSystem,
        isFile(Common::ApplicationConfiguration::applicationPathManager().getOutbreakModeStatusFilePath()))
        .WillOnce(Return(true));
    EXPECT_CALL(
        *mockFileSystem,
        readFile(Common::ApplicationConfiguration::applicationPathManager().getOutbreakModeStatusFilePath()))
        .WillOnce(Throw(Common::FileSystem::IFileSystemException("TEST")));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockFileSystem));
    EXPECT_EQ(Telemetry::parseOutbreakStatusFile(), std::nullopt);

    std::string logMessage = testing::internal::GetCapturedStderr();
    EXPECT_THAT(logMessage, ::testing::HasSubstr("Unable to read file at: "));
}

TEST_F(BaseTelemetryReporterTests, parseOutbreakStatusCatchesParseError)
{
    testing::internal::CaptureStderr();
    auto mockFileSystem =  std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(
        *mockFileSystem,
        isFile(Common::ApplicationConfiguration::applicationPathManager().getOutbreakModeStatusFilePath()))
        .WillOnce(Return(true));
    EXPECT_CALL(
        *mockFileSystem,
        readFile(Common::ApplicationConfiguration::applicationPathManager().getOutbreakModeStatusFilePath()))
        .WillOnce(Return("not a json"));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockFileSystem));
    EXPECT_EQ(Telemetry::parseOutbreakStatusFile(), std::nullopt);

    std::string logMessage = testing::internal::GetCapturedStderr();
    EXPECT_THAT(logMessage, ::testing::HasSubstr("Unable to parse json at: "));
}

TEST_F(BaseTelemetryReporterTests, getOutbreakModeCurrentInOutbreakMode)
{
    auto mockFileSystem =  std::make_unique<StrictMock<MockFileSystem>>();


    EXPECT_CALL(
        *mockFileSystem,
        isFile(Common::ApplicationConfiguration::applicationPathManager().getOutbreakModeStatusFilePath()))
        .WillOnce(Return(true));
    EXPECT_CALL(
        *mockFileSystem,
        readFile(Common::ApplicationConfiguration::applicationPathManager().getOutbreakModeStatusFilePath()))
        .WillOnce(Return(R"({ "outbreak-mode" : true })"));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockFileSystem));
    EXPECT_EQ(Telemetry::BaseTelemetryReporter::getOutbreakModeCurrent(), "true");
}

TEST_F(BaseTelemetryReporterTests, getOutbreakModeCurrentNotInOutbreakMode)
{
    auto mockFileSystem =  std::make_unique<StrictMock<MockFileSystem>>();

    EXPECT_CALL(
        *mockFileSystem,
        isFile(Common::ApplicationConfiguration::applicationPathManager().getOutbreakModeStatusFilePath()))
        .WillOnce(Return(true));
    EXPECT_CALL(
        *mockFileSystem,
        readFile(Common::ApplicationConfiguration::applicationPathManager().getOutbreakModeStatusFilePath()))
        .WillOnce(Return(R"({ "outbreak-mode" : false })"));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockFileSystem));
    EXPECT_EQ(Telemetry::BaseTelemetryReporter::getOutbreakModeCurrent(), "false");
}

TEST_F(BaseTelemetryReporterTests, getOutbreakModeCurrentWithInvalidJson)
{
    testing::internal::CaptureStderr();
    auto mockFileSystem =  std::make_unique<StrictMock<MockFileSystem>>();

    EXPECT_CALL(
        *mockFileSystem,
        isFile(Common::ApplicationConfiguration::applicationPathManager().getOutbreakModeStatusFilePath()))
        .WillOnce(Return(true));
    EXPECT_CALL(
        *mockFileSystem,
        readFile(Common::ApplicationConfiguration::applicationPathManager().getOutbreakModeStatusFilePath()))
        .WillOnce(Return(R"({ .,';" })"));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockFileSystem));
    EXPECT_EQ(Telemetry::BaseTelemetryReporter::getOutbreakModeCurrent(), std::nullopt);

    std::string logMessage = testing::internal::GetCapturedStderr();
    EXPECT_THAT(logMessage, ::testing::HasSubstr("Unable to parse json at: "));
}

TEST_F(BaseTelemetryReporterTests, getOutbreakModeCurrentWithTypeError)
{
    auto mockFileSystem =  std::make_unique<StrictMock<MockFileSystem>>();

    EXPECT_CALL(
        *mockFileSystem,
        isFile(Common::ApplicationConfiguration::applicationPathManager().getOutbreakModeStatusFilePath()))
        .WillOnce(Return(true));
    EXPECT_CALL(
        *mockFileSystem,
        readFile(Common::ApplicationConfiguration::applicationPathManager().getOutbreakModeStatusFilePath()))
        .WillOnce(Return(R"( { "outbreak-mode" : "not boolean" } )"));

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockFileSystem));
    EXPECT_EQ(Telemetry::BaseTelemetryReporter::getOutbreakModeCurrent(), std::nullopt);
}

TEST_F(BaseTelemetryReporterTests, getOutbreakModeCurrentWithEmptyJson)
{
    testing::internal::CaptureStderr();
    auto mockFileSystem =  std::make_unique<StrictMock<MockFileSystem>>();

    EXPECT_CALL(
        *mockFileSystem,
        isFile(Common::ApplicationConfiguration::applicationPathManager().getOutbreakModeStatusFilePath()))
        .WillOnce(Return(true));
    EXPECT_CALL(
        *mockFileSystem,
        readFile(Common::ApplicationConfiguration::applicationPathManager().getOutbreakModeStatusFilePath()))
        .WillOnce(Return(""));

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockFileSystem));
    EXPECT_EQ(Telemetry::BaseTelemetryReporter::getOutbreakModeCurrent(), std::nullopt);

    std::string logMessage = testing::internal::GetCapturedStderr();
    EXPECT_THAT(logMessage, ::testing::HasSubstr("Unable to parse json at: "));
}

TEST_F(BaseTelemetryReporterTests, getOutbreakModeCurrentWithMissingFile)
{
    testing::internal::CaptureStderr();
    auto mockFileSystem =  std::make_unique<StrictMock<MockFileSystem>>();

    EXPECT_CALL(
        *mockFileSystem,
        isFile(Common::ApplicationConfiguration::applicationPathManager().getOutbreakModeStatusFilePath()))
        .WillOnce(Return(false));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockFileSystem));
    EXPECT_EQ(Telemetry::BaseTelemetryReporter::getOutbreakModeCurrent(), std::nullopt);

    std::string logMessage = testing::internal::GetCapturedStderr();
    EXPECT_EQ(logMessage, ""); //no errors expected
}

TEST_F(BaseTelemetryReporterTests, getOutbreakModeHistoricWithFile)
{
    auto mockFileSystem =  std::make_unique<StrictMock<MockFileSystem>>();

    EXPECT_CALL(
        *mockFileSystem,
        isFile(Common::ApplicationConfiguration::applicationPathManager().getOutbreakModeStatusFilePath()))
        .WillOnce(Return(true));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockFileSystem));
    EXPECT_EQ(Telemetry::BaseTelemetryReporter::getOutbreakModeHistoric(), "true");
}

TEST_F(BaseTelemetryReporterTests, getOutbreakModeHistoricWithMissingFile)
{
    auto mockFileSystem =  std::make_unique<StrictMock<MockFileSystem>>();

    EXPECT_CALL(
        *mockFileSystem,
        isFile(Common::ApplicationConfiguration::applicationPathManager().getOutbreakModeStatusFilePath()))
        .WillOnce(Return(false));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockFileSystem));
    EXPECT_EQ(Telemetry::BaseTelemetryReporter::getOutbreakModeHistoric(), "false");
}

namespace
{
    class TZReset
    {
        bool has_old_tz_ = false;
        std::string old_tz_;
    public:
        explicit TZReset(const std::string& TZ)
        {
            auto* tz = getenv("TZ");
            if (tz == nullptr)
            {
                has_old_tz_ = false;
            }
            else
            {
                has_old_tz_ = true;
                old_tz_ = tz;
            }
            setenv("TZ", TZ.c_str(), 1);
        }

        ~TZReset()
        {
            if (has_old_tz_)
            {
                setenv("TZ", old_tz_.c_str(), 1);
            }
        }
    };

    using clock_t = Telemetry::BaseTelemetryReporter::clock_t;

    std::string expectReadOutbreakStatusFile(MockFileSystem* mockFileSystem, std::chrono::time_point<clock_t> outbreakTime)
    {
        auto ts = Common::UtilityImpl::TimeUtils::MessageTimeStamp(outbreakTime);
        EXPECT_CALL(
            *mockFileSystem,
            isFile(Common::ApplicationConfiguration::applicationPathManager().getOutbreakModeStatusFilePath()))
            .WillOnce(Return(true));
        EXPECT_CALL(
            *mockFileSystem,
            readFile(Common::ApplicationConfiguration::applicationPathManager().getOutbreakModeStatusFilePath()))
            .WillOnce(Return(R"({ "timestamp" : ")" + ts + "\" }"));
        return ts;
    }
}

TEST_F(BaseTelemetryReporterTests, getOutbreakModeTodayWithLargeTimeDifference)
{
    auto mockFileSystem = new StrictMock<MockFileSystem>();
    std::unique_ptr<MockFileSystem> mockIFileSystemPtr = std::unique_ptr<MockFileSystem>(mockFileSystem);
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockIFileSystemPtr));

    auto now = Telemetry::BaseTelemetryReporter::clock_t::now();
    auto yesterday = now - std::chrono::hours{25};

    expectReadOutbreakStatusFile(mockFileSystem, yesterday);
    EXPECT_EQ(Telemetry::BaseTelemetryReporter::getOutbreakModeToday(now), "false");
}

TEST_F(BaseTelemetryReporterTests, getOutbreakModeTodayWithSmallTimeDifference)
{
    auto now = Telemetry::BaseTelemetryReporter::clock_t::now();
    auto earlier = now - std::chrono::hours{23};

    auto mockFileSystem = new StrictMock<MockFileSystem>();
    expectReadOutbreakStatusFile(mockFileSystem, earlier);
    std::unique_ptr<MockFileSystem> mockIFileSystemPtr = std::unique_ptr<MockFileSystem>(mockFileSystem);
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockIFileSystemPtr));

    EXPECT_EQ(Telemetry::BaseTelemetryReporter::getOutbreakModeToday(now), "true");
}

TEST_F(BaseTelemetryReporterTests, getOutbreakModeTodayWithFutureTimeDifference)
{
    auto now = Telemetry::BaseTelemetryReporter::clock_t::now();
    auto tomorrow = now + std::chrono::hours{25};

    auto mockFileSystem = new StrictMock<MockFileSystem>();
    expectReadOutbreakStatusFile(mockFileSystem, tomorrow);
    std::unique_ptr<MockFileSystem> mockIFileSystemPtr = std::unique_ptr<MockFileSystem>(mockFileSystem);
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockIFileSystemPtr));

    EXPECT_EQ(Telemetry::BaseTelemetryReporter::getOutbreakModeToday(now), "false");
}

namespace
{
    void testOnSecondInPast()
    {
        auto now = Telemetry::BaseTelemetryReporter::clock_t::now();
        auto outbreakTime = now - std::chrono::seconds{1};

        auto mockFileSystem = new StrictMock<MockFileSystem>();
        auto ts = expectReadOutbreakStatusFile(mockFileSystem, outbreakTime);
        std::unique_ptr<MockFileSystem> mockIFileSystemPtr = std::unique_ptr<MockFileSystem>(mockFileSystem);
        Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockIFileSystemPtr));

        EXPECT_EQ(Telemetry::BaseTelemetryReporter::getOutbreakModeToday(now), "true");
    }

    void testOnSecondInFuture()
    {
        auto now = Telemetry::BaseTelemetryReporter::clock_t::now();
        auto outbreakTime = now + std::chrono::seconds{1};

        auto mockFileSystem = new StrictMock<MockFileSystem>();
        auto ts = expectReadOutbreakStatusFile(mockFileSystem, outbreakTime);
        std::unique_ptr<MockFileSystem> mockIFileSystemPtr = std::unique_ptr<MockFileSystem>(mockFileSystem);
        Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockIFileSystemPtr));

        EXPECT_EQ(Telemetry::BaseTelemetryReporter::getOutbreakModeToday(now), "false");
    }
}

TEST_F(OutbreakTelemetryTests, OutbreakOnSecondInPastInLondon)
{
    TZReset timezoneResetter("Europe/London");
    testOnSecondInPast();
}

TEST_F(OutbreakTelemetryTests, OutbreakOneSecondInFutureInLondon)
{
    TZReset timezoneResetter("Europe/London");
    testOnSecondInFuture();
}

TEST_F(OutbreakTelemetryTests, OutbreakOnSecondInPastInTokyo)
{
    TZReset timezoneResetter("Asia/Tokyo");
    testOnSecondInPast();
}

TEST_F(OutbreakTelemetryTests, OutbreakOneSecondInFutureInJapan)
{
    TZReset timezoneResetter("Asia/Tokyo");
    testOnSecondInFuture();
}

TEST_F(OutbreakTelemetryTests, OutbreakOneSecondInPastInNewYork)
{
    TZReset timezoneResetter("America/New_York");
    testOnSecondInPast();
}

TEST_F(OutbreakTelemetryTests, OutbreakOneSecondInFutureInNewYork)
{
    TZReset timezoneResetter("America/New_York");
    testOnSecondInFuture();
}

TEST_F(BaseTelemetryReporterTests, getOutbreakModeTodayWithInvalidTime)
{
    testing::internal::CaptureStderr();

    auto mockFileSystem =  std::make_unique<StrictMock<MockFileSystem>>();

    auto now = Telemetry::BaseTelemetryReporter::clock_t::now();

    EXPECT_CALL(
        *mockFileSystem,
        isFile(Common::ApplicationConfiguration::applicationPathManager().getOutbreakModeStatusFilePath()))
        .WillOnce(Return(true));
    EXPECT_CALL(
        *mockFileSystem,
        readFile(Common::ApplicationConfiguration::applicationPathManager().getOutbreakModeStatusFilePath()))
        .WillOnce(Return(R"( { "timestamp" : "tomorrow, at some point" } )"));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockFileSystem));
    EXPECT_EQ(Telemetry::BaseTelemetryReporter::getOutbreakModeToday(now), std::nullopt);

    std::string logMessage = testing::internal::GetCapturedStderr();
    EXPECT_THAT(logMessage, ::testing::HasSubstr("Unable to parse outbreak mode timestamp to time: "));
}

TEST_F(BaseTelemetryReporterTests, getOutbreakModeTodayWithInvalidJson)
{
    testing::internal::CaptureStderr();

    auto mockFileSystem =  std::make_unique<StrictMock<MockFileSystem>>();

    auto now = Telemetry::BaseTelemetryReporter::clock_t::now();

    EXPECT_CALL(
        *mockFileSystem,
        isFile(Common::ApplicationConfiguration::applicationPathManager().getOutbreakModeStatusFilePath()))
        .WillOnce(Return(true));
    EXPECT_CALL(
        *mockFileSystem,
        readFile(Common::ApplicationConfiguration::applicationPathManager().getOutbreakModeStatusFilePath()))
        .WillOnce(Return(R"( { :D "timestamp" : "tomorrow, at some point" } )"));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockFileSystem));
    EXPECT_EQ(Telemetry::BaseTelemetryReporter::getOutbreakModeToday(now), std::nullopt);

    std::string logMessage = testing::internal::GetCapturedStderr();
    EXPECT_THAT(logMessage, ::testing::HasSubstr("Unable to parse json at: "));
}

TEST_F(BaseTelemetryReporterTests, getOutbreakModeTodayWithEmptyJson)
{
    testing::internal::CaptureStderr();

    auto mockFileSystem =  std::make_unique<StrictMock<MockFileSystem>>();

    auto now = Telemetry::BaseTelemetryReporter::clock_t::now();

    EXPECT_CALL(
        *mockFileSystem,
        isFile(Common::ApplicationConfiguration::applicationPathManager().getOutbreakModeStatusFilePath()))
        .WillOnce(Return(true));
    EXPECT_CALL(
        *mockFileSystem,
        readFile(Common::ApplicationConfiguration::applicationPathManager().getOutbreakModeStatusFilePath()))
        .WillOnce(Return(""));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockFileSystem));
    EXPECT_EQ(Telemetry::BaseTelemetryReporter::getOutbreakModeToday(now), std::nullopt);

    std::string logMessage = testing::internal::GetCapturedStderr();
    EXPECT_THAT(logMessage, ::testing::HasSubstr("Unable to parse json at: "));
}

TEST_F(BaseTelemetryReporterTests, OutbreakStatusFileTooLarge)
{
    testing::internal::CaptureStderr();

    auto mockFileSystem =  std::make_unique<StrictMock<MockFileSystem>>();

    auto now = Telemetry::BaseTelemetryReporter::clock_t::now();

    EXPECT_CALL(
        *mockFileSystem,
        isFile(Common::ApplicationConfiguration::applicationPathManager().getOutbreakModeStatusFilePath()))
        .WillOnce(Return(true));
    EXPECT_CALL(
        *mockFileSystem,
        readFile(Common::ApplicationConfiguration::applicationPathManager().getOutbreakModeStatusFilePath()))
        .WillOnce(Throw(Common::FileSystem::IFileTooLargeException("TEST")));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockFileSystem));
    EXPECT_EQ(Telemetry::BaseTelemetryReporter::getOutbreakModeToday(now), std::nullopt);

    std::string logMessage = testing::internal::GetCapturedStderr();
    EXPECT_THAT(logMessage, ::testing::HasSubstr("Unable to read file at: "));
}

TEST_F(BaseTelemetryReporterTests, getOutbreakModeTodayWithTypeError)
{
    auto mockFileSystem =  std::make_unique<StrictMock<MockFileSystem>>();

    auto now = Telemetry::BaseTelemetryReporter::clock_t::now();

    EXPECT_CALL(
        *mockFileSystem,
        isFile(Common::ApplicationConfiguration::applicationPathManager().getOutbreakModeStatusFilePath()))
        .WillOnce(Return(true));
    EXPECT_CALL(
        *mockFileSystem,
        readFile(Common::ApplicationConfiguration::applicationPathManager().getOutbreakModeStatusFilePath()))
        .WillOnce(Return(R"( {"timestamp" : false } )"));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockFileSystem));
    EXPECT_EQ(Telemetry::BaseTelemetryReporter::getOutbreakModeToday(now), std::nullopt);
}

TEST_F(BaseTelemetryReporterTests, getOutbreakModeTodayWithFileMissing)
{
    auto mockFileSystem =  std::make_unique<StrictMock<MockFileSystem>>();

    auto now = Telemetry::BaseTelemetryReporter::clock_t::now();

    EXPECT_CALL(
        *mockFileSystem,
        isFile(Common::ApplicationConfiguration::applicationPathManager().getOutbreakModeStatusFilePath()))
        .WillOnce(Return(false));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem(std::move(mockFileSystem));
    EXPECT_EQ(Telemetry::BaseTelemetryReporter::getOutbreakModeToday(now), std::nullopt);
}