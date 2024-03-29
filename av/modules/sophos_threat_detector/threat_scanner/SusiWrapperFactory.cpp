// Copyright 2020-2023 Sophos Limited. All rights reserved.

#include "SusiWrapperFactory.h"

#include "Logger.h"
#include "ScannerInfo.h"
#include "SusiRuntimeConfig.h"
#include "SusiWrapper.h"

#include "common/ApplicationPaths.h"
#include "common/PluginUtils.h"
#include "common/StringUtils.h"

#include "Common/UtilityImpl/StringUtils.h"
#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"

#include <nlohmann/json.hpp>

#include <fstream>

namespace threat_scanner
{
    using json = nlohmann::json;
    namespace fs = sophos_filesystem;

    namespace
    {
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
                    else if (common::contains(endpointIdString, "\n"))
                    {
                        LOGERROR("EndpointID cannot contain a new line");
                    }
                    else if (common::contains(endpointIdString, " "))
                    {
                        LOGERROR("EndpointID cannot contain a empty space");
                    }
                    else if (endpointIdString.length() != 32)
                    {
                        LOGERROR(
                            "EndpointID should be 32 hex characters (read " << endpointIdString.length()
                                                                            << " characters)");
                    }
                    // also covers the case where characters are non-utf8
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
                    else if (common::contains(custIdString, "\n"))
                    {
                        LOGERROR("CustomerID cannot contain a new line");
                    }
                    else if (common::contains(custIdString, " "))
                    {
                        LOGERROR("CustomerID cannot contain a empty space");
                    }
                    else if (custIdString.length() != 32)
                    {
                        LOGERROR(
                            "CustomerID should be 32 hex characters (read " << custIdString.length() << " characters)");
                    }
                    // also covers the case where characters are non-utf8
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
    } // namespace

    std::shared_ptr<ISusiWrapper> SusiWrapperFactory::createSusiWrapper(const std::string& scannerConfig)
    {
        auto susiSettings = m_globalHandler->accessSusiSettings();
        std::string scannerInfo = createScannerInfo(false, false, true, isMachineLearningEnabled());

        std::string runtimeConfig = createRuntimeConfig(
            scannerInfo, getEndpointId(), getCustomerId(), susiSettings);
        m_globalHandler->initializeSusi(runtimeConfig);
        return std::make_shared<SusiWrapper>(m_globalHandler, scannerConfig);
    }

    // make a method that reads ALC-policy to get the Customer ID
    SusiWrapperFactory::SusiWrapperFactory() : m_globalHandler(std::make_shared<SusiGlobalHandler>()) {}

    bool SusiWrapperFactory::update()
    {
        return m_globalHandler->update(
            pluginInstall() / "chroot/susi/update_source", pluginInstall() / "chroot/var/susi_update.lock");
    }

    bool SusiWrapperFactory::reload()
    {
        auto susiSettings = m_globalHandler->accessSusiSettings();
        std::string scannerInfo = createScannerInfo(false, false, true, isMachineLearningEnabled());
        std::string runtimeConfig = createRuntimeConfig(
            scannerInfo, getEndpointId(), getCustomerId(), susiSettings);
        return m_globalHandler->reload(runtimeConfig);
    }

    void SusiWrapperFactory::shutdown()
    {
        m_globalHandler->shutdown();
    }

    bool SusiWrapperFactory::susiIsInitialized()
    {
        return m_globalHandler->susiIsInitialized();
    }

    bool SusiWrapperFactory::updateSusiConfig()
    {
        // Read potentially new SUSI settings from disk (saved to disk by av plugin process)
        auto newSettings = std::make_shared<common::ThreatDetector::SusiSettings>(Plugin::getSusiStartupSettingsPath());
        auto oldSettings = m_globalHandler->accessSusiSettings();
        bool changed = !oldSettings || (*oldSettings != *newSettings);
        if (changed)
        {
            LOGDEBUG("SUSI settings changed");
            LOGDEBUG(
                "SUSI SXL lookups enabled: " << std::boolalpha << newSettings->isSxlLookupEnabled()
                                             << std::noboolalpha);
            LOGDEBUG("SUSI allow-listed items by Sha256: " << newSettings->getAllowListSizeSha256());
            LOGDEBUG("SUSI allow-listed items by Path: " << newSettings->getAllowListSizePath());
            m_globalHandler->setSusiSettings(std::move(newSettings));
        }
        return changed;
    }
} // namespace threat_scanner