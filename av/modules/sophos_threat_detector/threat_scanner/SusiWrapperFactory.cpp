/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#include "SusiWrapperFactory.h"

#include "ScannerInfo.h"
#include "SusiWrapper.h"
#include "Logger.h"

#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>
#include "Common/UtilityImpl/StringUtils.h"
#include <Common/XmlUtilities/AttributesMap.h>

#include <fstream>
#include <thirdparty/nlohmann-json/json.hpp>

using namespace threat_scanner;
using json = nlohmann::json;
namespace fs = sophos_filesystem;

std::shared_ptr<ISusiWrapper> SusiWrapperFactory::createSusiWrapper(const std::string& scannerConfig)
{
    return std::make_shared<SusiWrapper>(m_globalHandler, scannerConfig);
}

static fs::path susi_library_path()
{
    return pluginInstall() / "chroot/susi/distribution_version";
}

static std::string getEndpointId()
{
    auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
    fs::path machineIdPath = appConfig.getData("SOPHOS_INSTALL") + "/base/etc/machine_id.txt";

    std::ifstream fs(machineIdPath);

    if (fs.good())
    {
        try
        {
            std::stringstream endpointIdContents;
            endpointIdContents << fs.rdbuf();

            return endpointIdContents.str();
        }
        catch (std::exception& e)
        {
            LOGERROR("Unexpected error when reading endpoint id for global rep setup: " << e.what());
        }
    }

    return "66b8fd8b39754951b87269afdfcb285c";
}

static std::string getCustomerId()
{
    auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
    fs::path customerIdPath = appConfig.getData("SOPHOS_INSTALL") + "/base/update/var/update_config.json";

    std::ifstream fs(customerIdPath, std::ifstream::in);

    if (fs.good())
    {
        try
        {
            std::stringstream updateConfigContents;
            updateConfigContents << fs.rdbuf();

            auto jsonContents = json::parse(updateConfigContents);
            LOGINFO("json: " << jsonContents);
            LOGINFO("creds: " << jsonContents["credential"]);
            return jsonContents["credential"]["password"];

        }
        catch (std::exception& e)
        {
            LOGERROR("Unexpected error when reading customer id for global rep setup: " << e.what());
        }
    }

    return "c1cfcf69a42311a6084bcefe8af02c8a";
}


static std::string create_runtime_config(const std::string& scannerInfo, const std::string& endpointId, const std::string& customerId)
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
            {"@@CUSTOMER_ID@@", customerId},
            {"@@MACHINE_ID@@", endpointId},
            {"@@SCANNER_CONFIG@@", scannerInfo}
    });
    return runtimeConfig;
}

//make a method that reads ALC-policy to get the Customer ID
SusiWrapperFactory::SusiWrapperFactory()
{
    std::string scannerInfo = create_scanner_info(false);
    std::string runtimeConfig = create_runtime_config(scannerInfo, getEndpointId(), getCustomerId());
    m_globalHandler = std::make_shared<SusiGlobalHandler>(runtimeConfig);
}
