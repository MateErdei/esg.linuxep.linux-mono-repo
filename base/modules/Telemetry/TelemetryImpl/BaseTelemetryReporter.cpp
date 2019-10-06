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

namespace Telemetry
{

    std::string BaseTelemetryReporter::getTelemetry() {
        TelemetryObject root;

        TelemetryValue customerIdValue;
        auto customerId = getCustomerId();
        if (customerId)
        {
            customerIdValue.set(customerId.value());
            root.set("customerId", customerIdValue);
        }

        TelemetryValue endpointIdValue;
        auto endpointId = getEndpointId();
        if (endpointId)
        {
            endpointIdValue.set(endpointId.value());
            root.set("endpointId", endpointIdValue);
        }

        TelemetryValue machineIdValue;
        auto machineId = getMachineId();
        if (machineId)
        {
            machineIdValue.set(machineId.value());
            root.set("machineId", machineIdValue);
        }

        TelemetryValue versionValue;
        auto version = getVersion();
        if (version)
        {
            versionValue.set(version.value());
            root.set("version", versionValue);
        }

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
        LOGDEBUG("Could not find the ALC policy file at: " << alcPolicyFilepath);
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

    std::optional<std::string> BaseTelemetryReporter::extractValueFromIniFile(const Path& filePath, const std::string& key)
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
                LOGDEBUG("Failed to get " << key <<". " << ex.what());
                return std::nullopt;
            }
        }

        LOGDEBUG("Could not find ini file to extract data from: " << filePath);
        return std::nullopt;
    }

    std::optional<std::string> BaseTelemetryReporter::extractCustomerId(const std::string& policyXml)
    {
        try
        {
            Common::XmlUtilities::AttributesMap attributesMap = Common::XmlUtilities::parseXml(policyXml);
            auto matchingPaths = attributesMap.entitiesThatContainPath("AUConfigurations/customer");
            if (matchingPaths.empty())
            {
                return std::nullopt;
            }

            Common::XmlUtilities::Attributes attributes;
            for (auto& path : matchingPaths)
            {
                attributes = attributesMap.lookup(path);
                if (!attributes.empty())
                {
                    break;
                }
            }
            if (!attributes.empty())
            {
                std::string customerId = attributes.value("id");
                if (!customerId.empty())
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