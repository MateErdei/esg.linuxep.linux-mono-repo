// Copyright 2023 Sophos Limited. All rights reserved.

// File under test
#include "sophos_threat_detector/threat_scanner/SusiRuntimeConfig.h"
// Package
#include "sophos_threat_detector/threat_scanner/ScannerInfo.h"
// Product
#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
// Test helper
#include "Common/Helpers/LogInitializedTests.h"
// 3rd party
#include <nlohmann/json.hpp>
// Std C++
#include <thread>

using namespace threat_scanner;

namespace
{
    class TestSusiRuntimeConfig : public LogInitializedTests
    {
    public:
        void SetUp() override
        {
            setPluginInstall("/plugin");
        }

        static void setPluginInstall(const char* pluginInstall)
        {
            auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
            appConfig.setData("PLUGIN_INSTALL", pluginInstall);
        }
    };
}

#define PRINT(x) std::cerr << x << '\n'

TEST_F(TestSusiRuntimeConfig, can_parse_default_arguments)
{
    auto settings = std::make_shared<common::ThreatDetector::SusiSettings>();
    auto scannerInfo = createScannerInfo(false, false, false, false);
    auto result = createRuntimeConfig(scannerInfo, "", "", settings);
    EXPECT_NO_THROW(std::ignore = nlohmann::json::parse(result));
}

TEST_F(TestSusiRuntimeConfig, empty_url_leads_to_sxl_disabled)
{
    auto settings = std::make_shared<common::ThreatDetector::SusiSettings>();
    settings->setSxlUrl("");
    settings->setSxlLookupEnabled(true);
    auto scannerInfo = createScannerInfo(false, false, false, false);
    auto result = createRuntimeConfig(scannerInfo, "", "", settings);
    auto json = nlohmann::json::parse(result);
    auto sxlEnabled = json.at("library").at("SXL4").at("enableLookup");
    EXPECT_FALSE(sxlEnabled);
}

TEST_F(TestSusiRuntimeConfig, bad_url_leads_to_sxl_disabled)
{
    auto url = "https://" + std::string(1025, 'a');
    auto settings = std::make_shared<common::ThreatDetector::SusiSettings>();
    settings->setSxlUrl(url);
    settings->setSxlLookupEnabled(true);
    auto scannerInfo = createScannerInfo(false, false, false, false);
    auto result = createRuntimeConfig(scannerInfo, "", "", settings);
    auto json = nlohmann::json::parse(result);
    auto sxlEnabled = json.at("library").at("SXL4").at("enableLookup");
    EXPECT_FALSE(sxlEnabled);
}

TEST_F(TestSusiRuntimeConfig, sxl_enabled_with_url)
{
    auto settings = std::make_shared<common::ThreatDetector::SusiSettings>();
    std::string expectedSxlUrl = "https://foobar";
    settings->setSxlUrl(expectedSxlUrl);
    settings->setSxlLookupEnabled(true);
    auto scannerInfo = createScannerInfo(false, false, false, false);
    auto result = createRuntimeConfig(scannerInfo, "", "", settings);
    auto json = nlohmann::json::parse(result);
    auto sxlEnabled = json.at("library").at("SXL4").at("enableLookup");
    EXPECT_TRUE(sxlEnabled);
    auto sxlURL = json.at("library").at("SXL4").at("url");
    EXPECT_EQ(sxlURL, expectedSxlUrl);
}

TEST_F(TestSusiRuntimeConfig, DISABLED_weird_install_path)
{
    setPluginInstall("/opt/\" 12345/sophos-spl/plugins/av");
    auto settings = std::make_shared<common::ThreatDetector::SusiSettings>();
    auto scannerInfo = createScannerInfo(false, false, false, false);
    auto result = createRuntimeConfig(scannerInfo, "", "", settings);
    PRINT(result);
    std::this_thread::sleep_for(std::chrono::milliseconds{1});
    EXPECT_NO_THROW(std::ignore = nlohmann::json::parse(result));
}
