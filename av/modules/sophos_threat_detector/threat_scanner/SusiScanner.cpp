/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SusiScanner.h"

#include "Logger.h"
#include "datatypes/sophos_filesystem.h"

#include "Common/UtilityImpl/StringUtils.h"
#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"

#include <iostream>
#include <string>
#include <unistd.h>

#include <sys/stat.h>


using namespace threat_scanner;
namespace fs = sophos_filesystem;

fs::path pluginInstall()
{
    auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
    return appConfig.getData("PLUGIN_INSTALL");
}

SusiScanner::SusiScanner()
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
            "name": "DLCL_Experiment",
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

    m_susi = std::make_unique<SusiWrapper>(runtimeConfig, scannerConfig);
}

scan_messages::ScanResponse
SusiScanner::scan(datatypes::AutoFd& /*fd*/, const std::string& /*file_path*/)
{
    // TODO: Will be implemented by LINUXDAR-1565
    scan_messages::ScanResponse response;
    response.setClean(true);
    return response;
}