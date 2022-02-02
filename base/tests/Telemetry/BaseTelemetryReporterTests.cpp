/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/Logging/ConsoleLoggingSetup.h>
#include <Telemetry/TelemetryImpl/BaseTelemetryReporter.h>
#include <gmock/gmock-matchers.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <map>
#include <regex>
#include <utility>
#include <vector>

#include <tests/Common/Helpers/FilePermissionsReplaceAndRestore.h>
#include <tests/Common/Helpers/FileSystemReplaceAndRestore.h>
#include <tests/Common/Helpers/MockFilePermissions.h>
#include <tests/Common/Helpers/MockFileSystem.h>

using ::testing::Return;
using ::testing::StrictMock;

struct TelemetryTestCase
{
    TelemetryTestCase(  std::string iXml, std::string value):
            inputXml(std::move(iXml)),
            expectedValue(std::move(value))
    {

    }
    std::string inputXml;       //NOLINT
    std::string expectedValue;      //NOLINT
};

class BaseTelemetryReporterTests : public ::testing::Test
{
public:
    void TearDown() override {}

    Common::Logging::ConsoleLoggingSetup m_loggingSetup;
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
