/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <fstream>
#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>
#include "SusiWrapperFactory.h"

#include "ScannerInfo.h"
#include "SusiWrapper.h"
#include "Logger.h"

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

////make a method that reads /opt/sophos-spl/base/etc/machine_id.txt to get the Endpoint ID
static std::string getEndpointId()
{
    std::string endpointId = "unknown";

    auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
    fs::path machineIdPath = appConfig.getData("SOPHOS_INSTALL") + "/base/etc/machine_id.txt";

    std::ifstream fs(machineIdPath);
    LOGINFO("Entered getEndpointId");

    if (fs.good())
    {
        LOGINFO("Filestream is good");
        std::stringstream endpointIdContents;
        endpointIdContents << fs.rdbuf();
        endpointId = endpointIdContents.str();
        LOGINFO("Endpoint ID is: " << endpointId);
    }

    return endpointId;
}

static std::string create_runtime_config(const std::string& scannerInfo, const std::string endpointID)
{
    std::string customerID("0123456789abcdef");

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
        "customerID": "@@CUSTOMER_ID@@",
        "machineID": "@@MACHINE_ID@@"
    },
    @@SCANNER_CONFIG@@
})sophos", {{"@@LIBRARY_PATH@@", libraryPath},
            {"@@CUSTOMER_ID@@", customerID},
            {"@@MACHINE_ID@@", endpointID},
            {"@@SCANNER_CONFIG@@", scannerInfo}
    });
    return runtimeConfig;
}

//make a method that reads ALC-policy to get the Customer ID
SusiWrapperFactory::SusiWrapperFactory()
{
    std::string scannerInfo = create_scanner_info(false);
    std::string runtimeConfig = create_runtime_config(scannerInfo, getEndpointId());
    m_globalHandler = std::make_shared<SusiGlobalHandler>(runtimeConfig);
}
