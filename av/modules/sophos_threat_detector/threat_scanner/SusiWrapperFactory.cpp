/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SusiWrapperFactory.h"

#include "ScannerInfo.h"
#include "SusiWrapper.h"

#include "Common/UtilityImpl/StringUtils.h"

using namespace threat_scanner;
namespace fs = sophos_filesystem;

std::shared_ptr<ISusiWrapper> SusiWrapperFactory::createSusiWrapper(const std::string& scannerConfig)
{
    return std::make_shared<SusiWrapper>(m_globalHandler, scannerConfig);
}

static fs::path susi_library_path()
{
    return pluginInstall() / "chroot/susi/distribution_version";
}

static std::string create_runtime_config(const std::string& scannerInfo)
{
    fs::path libraryPath = susi_library_path();
    std::string runtimeConfig = Common::UtilityImpl::StringUtils::orderedStringReplace(R"sophos({
    "library": {
        "libraryPath": "@@LIBRARY_PATH@@",
        "tempPath": "/tmp",
        "product": {
            "name": "SSPL AV Plugin",
            "context": "File",
            "version": "1.0.0"
        },
        "globalReputation": {
            "customerID": "c1cfcf69a42311a6084bcefe8af02c8a",
            "machineID":  "66b8fd8b39754951b87269afdfcb285c",
            "timeout": 5,
            "enableLookup": true
        }
    },
    @@SCANNER_CONFIG@@
})sophos", {{"@@LIBRARY_PATH@@", libraryPath},
            {"@@SCANNER_CONFIG@@", scannerInfo}
    });
    return runtimeConfig;
}

SusiWrapperFactory::SusiWrapperFactory()
{
    std::string scannerInfo = create_scanner_info(false);
    std::string runtimeConfig = create_runtime_config(scannerInfo);
    m_globalHandler = std::make_shared<SusiGlobalHandler>(runtimeConfig);
}
