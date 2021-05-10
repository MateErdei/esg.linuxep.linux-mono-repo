/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SusiWrapperFactory.h"

#include "Logger.h"
#include "ScannerInfo.h"
#include "SusiWrapper.h"

#include "Common/UtilityImpl/StringUtils.h"
#include "common/PluginUtils.h"
#include "common/StringUtils.h"

#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>
#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <thirdparty/nlohmann-json/json.hpp>

#include <fstream>

namespace threat_scanner
{
    using json = nlohmann::json;
    namespace fs = sophos_filesystem;

    namespace
    {
        fs::path susi_library_path()
        {
            return pluginInstall() / "chroot/susi/distribution_version";
        }

        std::string getEndpointId()
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
                    auto endpointIdString = endpointIdContents.str();

                    if (endpointIdString.empty())
                    {
                        LOGERROR("EndpointID cannot be empty");
                    }
                    else if (common::contains(endpointIdString,"\n"))
                    {
                        LOGERROR("EndpointID cannot contain a new line");
                    }
                    else if (common::contains(endpointIdString," "))
                    {
                        LOGERROR("EndpointID cannot contain a empty space");
                    }
                    else if (endpointIdString.length() != 32)
                    {
                        LOGERROR("EndpointID should be 32 hex characters (read " << endpointIdString.length() << " characters)");
                    }
                        //also covers the case where characters are non-utf8
                    else if (!common::isStringHex(endpointIdString))
                    {
                        LOGERROR("EndpointID must be in hex format");
                    }
                    else
                    {
                        return endpointIdString;
                    }
                }
                catch (const std::exception& e)
                {
                    LOGERROR("Unexpected error when reading endpoint id for global rep setup: " << e.what());
                }
            }

            LOGERROR("Failed to read machine ID - using default value");
            return "66b8fd8b39754951b87269afdfcb285c";
        }

        std::string getCustomerId()
        {
            auto customerIdPath = pluginInstall() / "var/customer_id.txt";

            std::ifstream fs(customerIdPath, std::ifstream::in);

            if (fs.good())
            {
                try
                {
                    std::stringstream customerId;
                    customerId << fs.rdbuf();
                    auto custIdString = customerId.str();

                    if (custIdString.empty())
                    {
                        LOGERROR("CustomerID cannot be empty");
                    }
                    else if (common::contains(custIdString,"\n"))
                    {
                        LOGERROR("CustomerID cannot contain a new line");
                    }
                    else if (common::contains(custIdString," "))
                    {
                        LOGERROR("CustomerID cannot contain a empty space");
                    }
                    else if (custIdString.length() != 32)
                    {
                        LOGERROR("CustomerID should be 32 hex characters (read " << custIdString.length() << " characters)");
                    }
                    //also covers the case where characters are non-utf8
                    else if (!common::isStringHex(custIdString))
                    {
                        LOGERROR("CustomerID must be in hex format");
                    }
                    else
                    {
                        return custIdString;
                    }
                }
                catch (const std::exception& e)
                {
                    LOGERROR("Unexpected error when reading customer id for global rep setup: " << e.what());
                }
            }

            LOGERROR("Failed to read customerID - using default value");
            return "c1cfcf69a42311a6084bcefe8af02c8a";
        }

        bool isSxlLookupEnabled()
        {
            auto susiStartupSettingsPath = pluginInstall() / "var/susi_startup_settings.json";

            std::ifstream fs(susiStartupSettingsPath, std::ifstream::in);

            if (fs.good())
            {
                try
                {
                    std::stringstream settingsString;
                    settingsString << fs.rdbuf();

                    auto settingsJson = json::parse(settingsString.str());
                    if (settingsJson["enableSxlLookup"])
                    {
                        LOGDEBUG("SXL Lookups will be enabled");
                    }
                    else
                    {
                        LOGDEBUG("SXL Lookups will be disabled");
                    }
                    return settingsJson["enableSxlLookup"];
                }
                catch (const std::exception& e)
                {
                    LOGERROR("Unexpected error when reading susi startup settings for global rep setup: " << e.what());
                }
            }

            LOGINFO("Turning Live Protection on as default - no susi startup settings found");
            return true;
        }

        std::string create_runtime_config(
            const std::string& scannerInfo,
            const std::string& endpointId,
            const std::string& customerId,
            bool enableSxlLookup)
        {
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
            "machineID":  "@@MACHINE_ID@@",
            "url": "https://4.sophosxl.net/lookup",
            "timeout": 10
        }
    },
    @@SCANNER_CONFIG@@
})sophos",
                { { "@@LIBRARY_PATH@@", libraryPath },
                  { "@@VERSION_NUMBER@@", versionNumber },
                  { "@@ENABLE_SXL_LOOKUP@@", enableSxlLookup?"true":"false" },
                  { "@@CUSTOMER_ID@@", customerId },
                  { "@@MACHINE_ID@@", endpointId },
                  { "@@SCANNER_CONFIG@@", scannerInfo } });
            return runtimeConfig;
        }
    }

    std::shared_ptr<ISusiWrapper> SusiWrapperFactory::createSusiWrapper(const std::string& scannerConfig)
    {
        std::string scannerInfo = create_scanner_info(false);
        std::string runtimeConfig = create_runtime_config(scannerInfo, getEndpointId(), getCustomerId(), isSxlLookupEnabled());
        m_globalHandler->initializeSusi(runtimeConfig);
        return std::make_shared<SusiWrapper>(m_globalHandler, scannerConfig);
    }

    // make a method that reads ALC-policy to get the Customer ID
    SusiWrapperFactory::SusiWrapperFactory()
        : m_globalHandler(std::make_shared<SusiGlobalHandler>())
    {
    }

    bool SusiWrapperFactory::update()
    {
        return m_globalHandler->update(pluginInstall() / "chroot/susi/update_source");
    }
}