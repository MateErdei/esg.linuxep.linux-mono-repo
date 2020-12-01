/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ScannerInfo.h"

#include "Logger.h"

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
#include "Common/UtilityImpl/StringUtils.h"

std::string threat_scanner::create_scanner_info(bool scanArchives)
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
                "macintosh": true
            },
            "scanControl": {
                "trueFileTypeDetection": false,
                "puaDetection": false,
                "archiveRecursionDepth": 16,
                "stopOnArchiveBombs": true
            }
        }
    })sophos", {
                             {"@@SCAN_ARCHIVES@@", scanArchives?"true":"false"},
                             {"@@WEB_ARCHIVES@@",  scanArchives?"true":"false"},
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
