// Copyright 2023 Sophos Limited. All rights reserved.

#include "telemetry/JsonBuilder.h"

#include "Common/Datatypes/Print.h"

#include "Common/Helpers/LogInitializedTests.h"
#include "Common/Helpers/MockPlatformUtils.h"

#include <nlohmann/json.hpp>

#include <gtest/gtest.h>

namespace
{
    class TestJsonBuilder : public LogInitializedTests
    {
    public:
        TestJsonBuilder()
        {
            platform_ = std::make_shared<NiceMock<MockPlatformUtils>>();
        }

        std::shared_ptr<MockPlatformUtils> platform_;
    };
}

TEST_F(TestJsonBuilder, timestampRightLength)
{
    auto timestamp = thininstaller::telemetry::JsonBuilder::generateTimeStamp();
    auto expected = std::string{"2023-09-15T10:32:18Z"};
    EXPECT_EQ(timestamp.size(), expected.size());
}

TEST_F(TestJsonBuilder, emptyConfig)
{
    using namespace thininstaller::telemetry;
    JsonBuilder::ConfigFile config{{}};
    JsonBuilder builder(config, {});

    auto json = builder.build(*platform_);
    EXPECT_EQ(json, "");

    auto tenant_id = builder.tenantId();
    EXPECT_EQ(tenant_id, "");
}

TEST_F(TestJsonBuilder, tenantId)
{
    using namespace thininstaller::telemetry;
    JsonBuilder::ConfigFile config{{"TENANT_ID=ABC"}};
    JsonBuilder builder(config, {});

    const std::string expectedTenantId = "ABC";
    const std::string expectedMachineIdSize = "3c233cfadef60213fd374b7971a89687";

    auto json = builder.build(*platform_);
    PRINT(json);
    auto actual = nlohmann::json::parse(json);
    EXPECT_EQ(actual["tenantId"], expectedTenantId);
    EXPECT_EQ(actual["machineId"].get<std::string>().size(), expectedMachineIdSize.size());
    EXPECT_FALSE(actual["linuxInstaller"]["installSuccess"].get<bool>());

    auto tenant_id = builder.tenantId();
    EXPECT_EQ(tenant_id, expectedTenantId);
}

TEST_F(TestJsonBuilder, commandSuccess)
{
    using namespace thininstaller::telemetry;
    JsonBuilder::ConfigFile config{{"TENANT_ID=ABC"}};
    JsonBuilder::ConfigFile::lines_t lines;
    lines.emplace_back("registrationWithCentral=true");
    lines.emplace_back("productInstalled=true");
    JsonBuilder::ConfigFile command{lines};
    JsonBuilder::map_t results;
    results.emplace("thininstaller_report.ini", command);

    JsonBuilder builder(config, results);
    auto json = builder.build(*platform_);
    auto actual = nlohmann::json::parse(json);
    EXPECT_TRUE(actual["command"]["installCommand"]["success"]);
    EXPECT_TRUE(actual["command"]["downloadCommand"]["success"]);
    EXPECT_TRUE(actual["command"]["registerCommand"]["success"]);
    EXPECT_TRUE(actual["command"]["compatabilityChecksCommand"]["success"]);
}

TEST_F(TestJsonBuilder, installFailure)
{
    const std::string expectedMessage="Failed to install at least one of the packages, see logs for details";

    using namespace thininstaller::telemetry;
    JsonBuilder::ConfigFile config{{"TENANT_ID=ABC"}};
    JsonBuilder::ConfigFile::lines_t lines;
    lines.emplace_back("registrationWithCentral=true");
    lines.emplace_back("productInstalled=false");
    lines.emplace_back(std::string{"summary="}+expectedMessage);
    JsonBuilder::ConfigFile command{lines};
    JsonBuilder::map_t results;
    results.emplace("thininstaller_report.ini", command);

    JsonBuilder builder(config, results);
    auto json = builder.build(*platform_);
    auto actual = nlohmann::json::parse(json);
    EXPECT_FALSE(actual["command"]["installCommand"]["success"]);
    EXPECT_EQ(actual["command"]["installCommand"]["error_message"], expectedMessage);
    EXPECT_TRUE(actual["command"]["downloadCommand"]["success"]);
    EXPECT_TRUE(actual["command"]["registerCommand"]["success"]);
    EXPECT_TRUE(actual["command"]["compatabilityChecksCommand"]["success"]);
}


TEST_F(TestJsonBuilder, downloadFailed)
{
    const std::string expectedMessage="Failed to install: Download failure";

    using namespace thininstaller::telemetry;
    JsonBuilder::ConfigFile config{{"TENANT_ID=ABC"}};
    JsonBuilder::ConfigFile::lines_t lines;
    lines.emplace_back("registrationWithCentral=true");
    lines.emplace_back("productInstalled=false");
    lines.emplace_back(std::string{"summary="}+expectedMessage);
    JsonBuilder::ConfigFile command{lines};
    JsonBuilder::map_t results;
    results.emplace("thininstaller_report.ini", command);

    JsonBuilder builder(config, results);
    auto json = builder.build(*platform_);
    auto actual = nlohmann::json::parse(json);
    EXPECT_FALSE(actual["command"]["downloadCommand"]["success"]);
    EXPECT_EQ(actual["command"]["downloadCommand"]["error_message"], expectedMessage);
    EXPECT_TRUE(actual["command"]["registerCommand"]["success"]);
    EXPECT_TRUE(actual["command"]["compatabilityChecksCommand"]["success"]);
}


TEST_F(TestJsonBuilder, registerFailed)
{
    const std::string expectedMessage="Cannot connect to Sophos Central - please check your network connections";

    using namespace thininstaller::telemetry;
    JsonBuilder::ConfigFile config{{"TENANT_ID=ABC"}};
    JsonBuilder::ConfigFile::lines_t lines;
    lines.emplace_back("registrationWithCentral=false");
    lines.emplace_back(std::string{"summary="}+expectedMessage);
    JsonBuilder::ConfigFile command{lines};
    JsonBuilder::map_t results;
    results.emplace("thininstaller_report.ini", command);

    JsonBuilder builder(config, results);
    auto json = builder.build(*platform_);
    auto actual = nlohmann::json::parse(json);
    EXPECT_FALSE(actual["command"]["registerCommand"]["success"]);
    EXPECT_EQ(actual["command"]["registerCommand"]["error_message"], expectedMessage);
    EXPECT_TRUE(actual["command"]["compatabilityChecksCommand"]["success"]);
}

TEST_F(TestJsonBuilder, compatibilityFailed)
{
    const std::string expectedMessage="ERROR: SPL installation will fail, can not install to '/a b' because it contains spaces.";

    using namespace thininstaller::telemetry;
    JsonBuilder::ConfigFile config{{"TENANT_ID=ABC"}};
    JsonBuilder::ConfigFile::lines_t lines;
    lines.emplace_back("installPathVerified=false");
    lines.emplace_back(std::string{"summary="}+expectedMessage);
    JsonBuilder::ConfigFile command{lines};
    JsonBuilder::map_t results;
    results.emplace("thininstaller_report.ini", command);

    JsonBuilder builder(config, results);
    auto json = builder.build(*platform_);
    auto actual = nlohmann::json::parse(json);
    EXPECT_FALSE(actual["command"]["compatabilityChecksCommand"]["success"]);
    EXPECT_EQ(actual["command"]["compatabilityChecksCommand"]["error_message"], expectedMessage);
}
