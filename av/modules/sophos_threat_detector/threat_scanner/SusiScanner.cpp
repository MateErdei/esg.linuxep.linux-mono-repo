/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SusiScanner.h"

#include "Logger.h"
#include "datatypes/sophos_filesystem.h"

#include "Common/UtilityImpl/StringUtils.h"
#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"

#include <thirdparty/nlohmann-json/json.hpp>

#include <iostream>
#include <string>

using namespace threat_scanner;
using json = nlohmann::json;
namespace fs = sophos_filesystem;

fs::path pluginInstall()
{
    auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
    return appConfig.getData("PLUGIN_INSTALL");
}

SusiScanner::SusiScanner(const std::shared_ptr<ISusiWrapperFactory>& susiWrapperFactory)
{
    fs::path libraryPath = pluginInstall() / "chroot/susi/distribution_version";

    static const std::string scannerInfo = R"("scanner": {
        "signatureBased": {
            "fileTypeCategories": {
                "archive": true,
                "selfExtractor": true,
                "executable": true,
                "office": true,
                "adobe": true,
                "android": true,
                "internet": true,
                "webArchive": true,
                "webEncoding": true,
                "media": true,
                "macintosh": true
            },
            "scanControl": {
                "trueFileTypeDetection": true,
                "puaDetection": true,
                "archiveRecursionDepth": 16,
                "stopOnArchiveBombs": true
            }
        }
    })";

    std::string runtimeConfig = Common::UtilityImpl::StringUtils::orderedStringReplace(R"sophos({
    "library": {
        "libraryPath": "@@LIBRARY_PATH@@",
        "tempPath": "/tmp",
        "product": {
            "name": "SSPL AV Plugin",
            "context": "File",
            "version": "1.0.0"
        },
        "customerID": "0123456789abcdef",
        "machineID": "fedcba9876543210"
    },
    @@SCANNER_CONFIG@@
})sophos", {{"@@LIBRARY_PATH@@", libraryPath},
                               {"@@SCANNER_CONFIG@@", scannerInfo}
                               });

    std::string scannerConfig = Common::UtilityImpl::StringUtils::orderedStringReplace(R"sophos({
        @@SCANNER_CONFIG@@
})sophos", {{"@@SCANNER_CONFIG@@", scannerInfo}
    });

    m_susi = susiWrapperFactory->createSusiWrapper(runtimeConfig, scannerConfig);
}

scan_messages::ScanResponse
SusiScanner::scan(datatypes::AutoFd& /*fd*/, const std::string& file_path)
{
    scan_messages::ScanResponse response;
    response.setClean(true);

    static const std::string metaDataJson = R"({
    "properties": {
        "url": "www.example.com"
    }
    })";

    SusiScanResult* scanResult = nullptr;
    SusiResult res = m_susi->scanFile(metaDataJson.c_str(), file_path.c_str(), &scanResult);

    if (res == SUSI_I_THREATPRESENT)
    {
        response.setClean(false);
    }
    LOGINFO("Scan result " << std::hex << res << std::dec);
    if (scanResult != nullptr)
    {
        LOGINFO("Details: " << scanResult->version << ", " << scanResult->scanResultJson);

        json parsedScanResult = json::parse(scanResult->scanResultJson);
        for (auto result : parsedScanResult["results"])
        {
            for (auto detection : result["detections"])
            {
                LOGERROR("Detected " << detection["threatName"] << " in " << detection["path"]);
            }
        }
    }

    m_susi->freeResult(scanResult);

    return response;
}