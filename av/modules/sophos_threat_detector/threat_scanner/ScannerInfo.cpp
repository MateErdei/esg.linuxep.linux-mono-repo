/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ScannerInfo.h"

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
    })sophos", {{"@@SCAN_ARCHIVES@@", scanArchives?"true":"false"}});

    return scannerInfo;
}
