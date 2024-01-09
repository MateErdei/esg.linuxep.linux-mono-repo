// Copyright 2023-2024 Sophos Limited. All rights reserved.

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

TEST_F(TestJsonBuilder, proxyFalse)
{
    using namespace thininstaller::telemetry;
    JsonBuilder::ConfigFile config{{"TENANT_ID=ABC"}};
    JsonBuilder::ConfigFile::lines_t lines;
    lines.emplace_back("usedProxy = false");
    lines.emplace_back("usedUpdateCache = false");
    lines.emplace_back("usedMessageRelay = false");
    lines.emplace_back("proxyOrMessageRelayURL = ");
    JsonBuilder::map_t results;
    results.emplace("registration_comms_check.ini", JsonBuilder::ConfigFile{lines});

    JsonBuilder builder(config, results);
    auto json = builder.build(*platform_);
    auto actual = nlohmann::json::parse(json);
    EXPECT_FALSE(actual["linuxInstaller"]["usedProxy"]);
}

TEST_F(TestJsonBuilder, proxyTrue)
{
    using namespace thininstaller::telemetry;
    JsonBuilder::ConfigFile config{{"TENANT_ID=ABC"}};
    JsonBuilder::ConfigFile::lines_t lines;
    lines.emplace_back("usedProxy = true");
    lines.emplace_back("usedUpdateCache = false");
    lines.emplace_back("usedMessageRelay = false");
    lines.emplace_back("proxyOrMessageRelayURL = ");
    JsonBuilder::map_t results;
    results.emplace("registration_comms_check.ini", JsonBuilder::ConfigFile{lines});

    JsonBuilder builder(config, results);
    auto json = builder.build(*platform_);
    auto actual = nlohmann::json::parse(json);
    EXPECT_TRUE(actual["linuxInstaller"]["usedProxy"]);
}

TEST_F(TestJsonBuilder, updateCacheTrue)
{
    using namespace thininstaller::telemetry;
    JsonBuilder::ConfigFile config{{"TENANT_ID=ABC"}};
    JsonBuilder::ConfigFile::lines_t lines;
    lines.emplace_back("usedProxy = false");
    lines.emplace_back("usedUpdateCache = true");
    lines.emplace_back("usedMessageRelay = false");
    lines.emplace_back("proxyOrMessageRelayURL = ");
    JsonBuilder::map_t results;
    results.emplace("cdn_comms_check.ini", JsonBuilder::ConfigFile{lines});

    JsonBuilder builder(config, results);
    auto json = builder.build(*platform_);
    auto actual = nlohmann::json::parse(json);
    EXPECT_TRUE(actual["linuxInstaller"]["usedUpdateCache"]);
}

TEST_F(TestJsonBuilder, messageRelayTrue)
{
    using namespace thininstaller::telemetry;
    JsonBuilder::ConfigFile config{{"TENANT_ID=ABC"}};
    JsonBuilder::ConfigFile::lines_t lines;
    lines.emplace_back("usedProxy = false");
    lines.emplace_back("usedUpdateCache = false");
    lines.emplace_back("usedMessageRelay = true");
    lines.emplace_back("proxyOrMessageRelayURL = ");
    JsonBuilder::map_t results;
    results.emplace("registration_comms_check.ini", JsonBuilder::ConfigFile{lines});

    JsonBuilder builder(config, results);
    auto json = builder.build(*platform_);
    auto actual = nlohmann::json::parse(json);
    EXPECT_TRUE(actual["linuxInstaller"]["usedMessageRelay"]);
}

TEST_F(TestJsonBuilder, proxyUrl)
{
    using namespace thininstaller::telemetry;
    JsonBuilder::ConfigFile config{{"TENANT_ID=ABC"}};
    JsonBuilder::ConfigFile::lines_t lines;
    lines.emplace_back("usedProxy = false");
    lines.emplace_back("usedUpdateCache = false");
    lines.emplace_back("usedMessageRelay = false");
    lines.emplace_back("proxyOrMessageRelayURL = http://proxy:8000");
    JsonBuilder::map_t results;
    results.emplace("registration_comms_check.ini", JsonBuilder::ConfigFile{lines});

    JsonBuilder builder(config, results);
    auto json = builder.build(*platform_);
    EXPECT_EQ(builder.proxy(), "http://proxy:8000");
}

TEST_F(TestJsonBuilder, commandLineDefaultValues)
{
    using namespace thininstaller::telemetry;
    JsonBuilder::ConfigFile config{{"TENANT_ID=ABC"}};
    JsonBuilder::ConfigFile::lines_t lines;
    JsonBuilder::map_t results;
    results.emplace("thininstallerArgs.ini", JsonBuilder::ConfigFile{lines});

    JsonBuilder builder(config, results);
    auto json = builder.build(*platform_);
    auto actual = nlohmann::json::parse(json);
    EXPECT_FALSE(actual["commandLine"]["forceInstall"]);
    EXPECT_FALSE(actual["commandLine"]["products"]);
    EXPECT_FALSE(actual["commandLine"]["group"]);
    EXPECT_FALSE(actual["commandLine"]["messageRelays"]);
    EXPECT_FALSE(actual["commandLine"]["updateCaches"]);
    EXPECT_FALSE(actual["commandLine"]["installDir"]);
    EXPECT_FALSE(actual["commandLine"]["disableAuditd"]);
}

TEST_F(TestJsonBuilder, commandLineAllTrue)
{
    using namespace thininstaller::telemetry;
    JsonBuilder::ConfigFile config{{"TENANT_ID=ABC"}};
    JsonBuilder::ConfigFile::lines_t lines;
    lines.emplace_back("force = true");
    lines.emplace_back("products = true");
    lines.emplace_back("group = true");
    lines.emplace_back("message-relays = true");
    lines.emplace_back("update-caches = true");
    lines.emplace_back("install-dir = true");
    lines.emplace_back("disable-auditd = true");
    JsonBuilder::map_t results;
    results.emplace("thininstallerArgs.ini", JsonBuilder::ConfigFile{lines});

    JsonBuilder builder(config, results);
    auto json = builder.build(*platform_);
    auto actual = nlohmann::json::parse(json);

    EXPECT_TRUE(actual["commandLine"]["forceInstall"]);
    EXPECT_TRUE(actual["commandLine"]["products"]);
    EXPECT_TRUE(actual["commandLine"]["group"]);
    EXPECT_TRUE(actual["commandLine"]["messageRelays"]);
    EXPECT_TRUE(actual["commandLine"]["updateCaches"]);
    EXPECT_TRUE(actual["commandLine"]["installDir"]);
    EXPECT_TRUE(actual["commandLine"]["disableAuditd"]);
}
