/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "TelemetryProcessor.h"
#include "BaseTelemetryReporter.h"

#include <Common/TelemetryHelperImpl/TelemetryHelper.h>
#include <Common/TelemetryHelperImpl/TelemetrySerialiser.h>
#include <Telemetry/LoggerImpl/Logger.h>
#include <Common/XmlUtilities/AttributesMap.h>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>

namespace
{
    using telemetryGetFuctor = std::function< std::optional<std::string>(void)>;
    void updateTelemetryRoot(TelemetryObject& root, const std::string& telemetryKey, const telemetryGetFuctor& functor)
    {
        try
        {
            auto telemetry = functor();
            if (telemetry)
            {
                TelemetryValue telemetryValue;
                telemetryValue.set(telemetry.value());
                root.set(telemetryKey, telemetryValue);
            }
        }
        catch (std::exception& ex)
        {
            LOGWARN("Could not get telemetry for " << telemetryKey << ". Exception: " << ex.what());
        }
    }
}

namespace Telemetry
{

    std::string BaseTelemetryReporter::getTelemetry()
    {
        TelemetryObject root;
        updateTelemetryRoot(root, "customerId", getCustomerId);
        updateTelemetryRoot(root, "endpointId", getEndpointId);
        updateTelemetryRoot(root, "machineId", getMachineId);
        updateTelemetryRoot(root, "version", getVersion);

        return TelemetrySerialiser::serialise(root);
    }

    std::optional<std::string> BaseTelemetryReporter::getMachineId()
    {
        auto fs = Common::FileSystem::fileSystem();
        Path machineIdFilePath = Common::ApplicationConfiguration::applicationPathManager().getMachineIdFilePath();
        if (fs->isFile(machineIdFilePath))
        {
            return fs->readFile(machineIdFilePath);
        }
        return std::nullopt;
    }

    std::optional<std::string> BaseTelemetryReporter::getCustomerId()
    {
        Path alcPolicyFilepath = Common::ApplicationConfiguration::applicationPathManager().getAlcPolicyFilePath();
        auto fs = Common::FileSystem::fileSystem();
        if (fs->isFile(alcPolicyFilepath))
        {
            return extractCustomerId(fs->readFile(alcPolicyFilepath));
        }
        LOGWARN("Could not find the ALC policy file at: " << alcPolicyFilepath);
        return std::nullopt;
    }

    std::optional<std::string> BaseTelemetryReporter::getVersion()
    {
        Path versionIniFilepath = Common::ApplicationConfiguration::applicationPathManager().getVersionFilePath();
        return extractValueFromIniFile(versionIniFilepath, "PRODUCT_VERSION");
    }

    std::optional<std::string> BaseTelemetryReporter::getEndpointId()
    {
        Path configFilePath = Common::ApplicationConfiguration::applicationPathManager().getMcsConfigFilePath();
        return extractValueFromIniFile(configFilePath, "MCSID");
    }

    std::optional<std::string> extractValueFromIniFile(const Path& filePath, const std::string& key)
    {
        auto fs = Common::FileSystem::fileSystem();
        if (fs->isFile(filePath))
        {
            try
            {
                boost::property_tree::ptree ptree;
                boost::property_tree::read_ini(filePath, ptree);
                return ptree.get<std::string>(key);
            }
            catch (boost::property_tree::ptree_error& ex)
            {
                LOGWARN("Failed to find key: " << key << " in ini file: " << filePath <<". Error: " << ex.what());
                return std::nullopt;
            }
        }

        LOGWARN("Could not find ini file to extract data from, file path: " << filePath);
        return std::nullopt;
    }

    std::optional<std::string> extractCustomerId(const std::string& policyXml)
    {
        try
        {
            //TODO - LINUXDAR-735
            // Current implementation has to get around the limitations of AttributesMap::entitiesThatContainPath implementation -
            Common::XmlUtilities::AttributesMap attributesMap = Common::XmlUtilities::parseXml(policyXml);

            // Search for paths that include the # so that we don't match sub-elements of customer or elements that start with customer
            std::string customerElementPath = "AUConfigurations/customer#";
            auto matchingPaths = attributesMap.entitiesThatContainPath(customerElementPath);
            if (matchingPaths.empty())
            {
                return std::nullopt;
            }

            for( auto& matchingPath: matchingPaths)
            {
                Common::XmlUtilities::Attributes attributes = attributesMap.lookup(matchingPath);
                std::string customerId = attributes.value("id");
                std::string expectedCustomerIdPath;
                expectedCustomerIdPath += customerElementPath;
                expectedCustomerIdPath += customerId;
                if( matchingPath == (expectedCustomerIdPath) )
                {
                    return customerId;
                }
            }
            LOGWARN("CustomerID not present in the policy XML");
        }
        catch (Common::XmlUtilities::XmlUtilitiesException& ex)
        {
            LOGWARN("Invalid policy received. Error: " << ex.what());
        }
        return std::nullopt;
    }
}