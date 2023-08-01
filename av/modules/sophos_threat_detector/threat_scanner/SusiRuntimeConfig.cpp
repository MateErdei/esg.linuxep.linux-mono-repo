// Copyright 2023 Sophos All rights reserved.
#include "SusiRuntimeConfig.h"

#include "Logger.h"
#include "ScannerInfo.h"

#include "common/PluginUtils.h"

#include "Common/UtilityImpl/StringUtils.h"

namespace
{
    fs::path susi_library_path()
    {
        return threat_scanner::pluginInstall() / "chroot/susi/distribution_version";
    }
}


std::string threat_scanner::createRuntimeConfig(
    const std::string& scannerInfo,
    const std::string& endpointId,
    const std::string& customerId,
    const std::shared_ptr<common::ThreatDetector::SusiSettings>& settings)
{
    auto enableSxlLookup = settings->isSxlLookupEnabled();
    auto sxlUrl = settings->getSxlUrl();
    if (sxlUrl.empty() && enableSxlLookup)
    {
        LOGINFO("Disabling Global Reputation as SXL URL is empty");
        enableSxlLookup = false;
    }

    fs::path libraryPath = susi_library_path();
    auto versionNumber = common::getPluginVersion();

    std::string runtimeConfig = Common::UtilityImpl::StringUtils::orderedStringReplace(
        R"sophos({
    "library": {
        "libraryPath": "@@LIBRARY_PATH@@",
        "tempPath": "/tmp",
        "product": {
            "name": "SUSI_SPLAV",
            "context": "File",
            "version": "@@VERSION_NUMBER@@"
        },
        "SXL4": {
            "enableLookup": @@ENABLE_SXL_LOOKUP@@,
            "sendTelemetry": true,
            "customerID": "@@CUSTOMER_ID@@",
            "machineID": "@@MACHINE_ID@@",
            "url": "@@SXL_URL@@",
            "timeout": 10
        }
    },
    @@SCANNER_CONFIG@@
})sophos",
        {
            { "@@LIBRARY_PATH@@", libraryPath },
            { "@@VERSION_NUMBER@@", versionNumber },
            { "@@ENABLE_SXL_LOOKUP@@", enableSxlLookup ? "true" : "false" },
            { "@@CUSTOMER_ID@@", customerId },
            { "@@MACHINE_ID@@", endpointId },
            { "@@SXL_URL@@", sxlUrl },
            { "@@SCANNER_CONFIG@@", scannerInfo }
        });
    return runtimeConfig;
}