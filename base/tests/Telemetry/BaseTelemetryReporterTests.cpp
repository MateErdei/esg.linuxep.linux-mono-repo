/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/Logging/ConsoleLoggingSetup.h>
#include <Common/UtilityImpl/IniFileUtilities.h>
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

//TODO extend the tests for extractValueFromIniFile maybe in seperate file
TEST_F(BaseTelemetryReporterTests, extractValueFromIniFileMissingIniFile) // NOLINT
{
    MockFileSystem* mockFileSystem = nullptr;
    std::unique_ptr<MockFileSystem> mockfileSystem(new StrictMock<MockFileSystem>());
    mockFileSystem = mockfileSystem.get();
    Tests::replaceFileSystem(std::move(mockfileSystem));

    std::string testFilePath("testfilepath");
    EXPECT_CALL(*mockFileSystem, isFile(testFilePath)).WillOnce(Return(false));
    auto endPointId = Common::UtilityImpl::extractValueFromIniFile(testFilePath, "endpointId");
    EXPECT_FALSE( endPointId.has_value());
}