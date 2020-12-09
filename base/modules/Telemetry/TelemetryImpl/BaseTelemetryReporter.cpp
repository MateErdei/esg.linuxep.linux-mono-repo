/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "BaseTelemetryReporter.h"

#include "TelemetryProcessor.h"

#include <Common/TelemetryHelperImpl/TelemetryHelper.h>
#include <Common/TelemetryHelperImpl/TelemetrySerialiser.h>
#include <Common/UtilityImpl/FileUtils.h>
#include <Common/XmlUtilities/AttributesMap.h>
#include <Telemetry/LoggerImpl/Logger.h>

namespace
{
    using telemetryGetFuctor = std::function<std::optional<std::string>(void)>;
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
} // namespace

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
        return extractValueFromFile(versionIniFilepath, "PRODUCT_VERSION");
    }

    std::optional<std::string> BaseTelemetryReporter::getEndpointId()
    {
        Path configFilePath = Common::ApplicationConfiguration::applicationPathManager().getMcsConfigFilePath();
        return extractValueFromFile(configFilePath, "MCSID");
    }

    std::optional<std::string> extractValueFromFile(const Path& filePath, const std::string& key)
    {
        auto fs = Common::FileSystem::fileSystem();
        if (fs->isFile(filePath))
        {
            std::pair<std::string, std::string> value =
                Common::UtilityImpl::FileUtils::extractValueFromFile(filePath, key);
            if (value.first.empty() && !value.second.empty())
            {
                LOGWARN("Failed to find key: " << key << " in file: " << filePath << " due to error: " << value.second);
                return std::nullopt;
            }
            return value.first;
        }

        LOGWARN("Could not find file to extract data from, file path: " << filePath);
        return std::nullopt;
    }

    std::optional<std::string> extractCustomerId(const std::string& policyXml)
    {
        try
        {
            // TODO - LINUXDAR-735
            // Current implementation has to get around the limitations of AttributesMap::entitiesThatContainPath
            // implementation -
            Common::XmlUtilities::AttributesMap attributesMap = Common::XmlUtilities::parseXml(policyXml);

            // Search for paths that include the # so that we don't match sub-elements of customer or elements that
            // start with customer
            std::string customerElementPath = "AUConfigurations/customer#";
            auto matchingPaths = attributesMap.entitiesThatContainPath(customerElementPath);
            if (matchingPaths.empty())
            {
                return std::nullopt;
            }

            for (auto& matchingPath : matchingPaths)
            {
                Common::XmlUtilities::Attributes attributes = attributesMap.lookup(matchingPath);
                std::string customerId = attributes.value("id");
                std::string expectedCustomerIdPath;
                expectedCustomerIdPath += customerElementPath;
                expectedCustomerIdPath += customerId;
                if (matchingPath == (expectedCustomerIdPath))
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
} // namespace Telemetry