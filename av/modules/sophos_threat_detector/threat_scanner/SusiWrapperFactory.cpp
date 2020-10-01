/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#include "SusiWrapperFactory.h"

#include "ScannerInfo.h"
#include "SusiWrapper.h"
#include "Logger.h"

#include "Common/UtilityImpl/StringUtils.h"
#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>
#include <Common/XmlUtilities/AttributesMap.h>

#include <fstream>


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

static std::string getCustomerId()
{
    auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
    fs::path customerIdPath = appConfig.getData("SOPHOS_INSTALL") + "/base/mcs/policy/ALC-1_policy.xml";

    std::ifstream fs(customerIdPath, std::ifstream::in);

    if (fs.good())
    {
        std::stringstream customerIdContents;
        customerIdContents << fs.rdbuf();
        LOGINFO("XML contents: " << customerIdContents.str());
        auto attributeMap = Common::XmlUtilities::parseXml(customerIdContents.str());
        try
        {
            auto attributes = attributeMap.lookupMultiple("AUConfigurations");

            LOGINFO("value is " << attributeMap.lookup("AUConfigurations/customer").value("id") << " textid is " << attributeMap.lookup("AUConfigurations/customer").TextId << " contents are " << attributeMap.lookup("AUConfigurations/customer").contents());

            return attributeMap.lookup("AUConfigurations/customer").contents();
        }
        catch (std::exception& e)
        {
            LOGERROR("Unexpected exception when reading customer id for global rep setup: " << e.what());
        }
    }

    return "unknown";
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
