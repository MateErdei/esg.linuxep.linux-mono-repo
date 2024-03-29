// Copyright 2020-2022 Sophos Limited. All rights reserved.

#include "ScannerInfo.h"

#include "Logger.h"

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
#include "Common/UtilityImpl/StringUtils.h"

std::string threat_scanner::createScannerInfo(bool scanArchives, bool scanImages, bool detectPUAs, bool machineLearning)
{
    std::string scannerInfo = Common::UtilityImpl::StringUtils::orderedStringReplace(R"sophos("scanner": {
        "signatureBased": {
            "fileTypeCategories": {
                "archive": @@SCAN_ARCHIVES@@,
                "selfExtractor": true,
                "executable": true,
                "office": true,
                "adobe": true,
                "android": true,
                "internet": true,
                "webArchive": @@WEB_ARCHIVES@@,
                "webEncoding": true,
                "media": true,
                "macintosh": true,
                "discImage": @@DISC_IMAGE@@
            },
            "scanControl": {
                "trueFileTypeDetection": false,
                "puaDetection": @@DETECT_PUAS@@,
                "archiveRecursionDepth": 16,
                "stopOnArchiveBombs": true,
                "submitToAnalysis": false
            }
        },
        "nextGen": {
            "scanControl": {
                "machineLearning": @@MACHINE_LEARNING@@
            }
        }
    })sophos", {
                             {"@@SCAN_ARCHIVES@@", scanArchives?"true":"false"},
                             {"@@WEB_ARCHIVES@@",  scanArchives?"true":"false"},
                             {"@@DISC_IMAGE@@",  scanImages?"true":"false"},
                             {"@@DETECT_PUAS@@",  detectPUAs?"true":"false"},
                             {"@@MACHINE_LEARNING@@",  machineLearning?"true":"false"},
                         });

    return scannerInfo;
}

static sophos_filesystem::path sophosInstall()
{
    auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
    return appConfig.getData("SOPHOS_INSTALL");
}

sophos_filesystem::path threat_scanner::pluginInstall()
{
    auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
    try
    {
        return appConfig.getData("PLUGIN_INSTALL");
    }
    catch (const std::out_of_range&)
    {
        LOGWARN("PLUGIN_INSTALL not set");
    }
    return sophosInstall() / "plugins/av";
}
