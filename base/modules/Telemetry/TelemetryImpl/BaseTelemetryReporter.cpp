// Copyright 2019-2023 Sophos Limited. All rights reserved.

#include "BaseTelemetryReporter.h"

#include "TelemetryProcessor.h"

#include "EventReceiverImpl/OutbreakModeController.h"
#include "Common/UtilityImpl/TimeUtils.h"

#include <Common/ApplicationConfigurationImpl/ApplicationPathManager.h>
#include <Common/TelemetryHelperImpl/TelemetrySerialiser.h>
#include <Common/UtilityImpl/FileUtils.h>
#include <Common/UtilityImpl/StringUtils.h>
#include <Common/XmlUtilities/AttributesMap.h>
#include <Telemetry/LoggerImpl/Logger.h>

#include <iomanip>

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
        updateTelemetryRoot(root, "tenantId", getTenantId);
        updateTelemetryRoot(root, "deviceId", getDeviceId);
        updateTelemetryRoot(root, "machineId", getMachineId);
        updateTelemetryRoot(root, "version", getVersion);
        updateTelemetryRoot(root, "overall-health", getOverallHealth);
        updateTelemetryRoot(root, "outbreak-now", getOutbreakModeCurrent);
        updateTelemetryRoot(root, "outbreak-ever", getOutbreakModeHistoric);
        updateTelemetryRoot(root, "outbreak-today", getOutbreakModeTodayWrapper);

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

    std::optional<std::string> BaseTelemetryReporter::getTenantId()
    {
        Path configFilePath = Common::ApplicationConfiguration::applicationPathManager().getMcsConfigFilePath();
        return extractValueFromFile(configFilePath, "tenant_id");
    }

    std::optional<std::string> BaseTelemetryReporter::getDeviceId()
    {
        Path configFilePath = Common::ApplicationConfiguration::applicationPathManager().getMcsConfigFilePath();
        return extractValueFromFile(configFilePath, "device_id");
    }

    std::optional<std::string> BaseTelemetryReporter::getOverallHealth()
    {
        Path shsStatusFilepath = Common::ApplicationConfiguration::applicationPathManager().getShsStatusFilePath();
        auto fs = Common::FileSystem::fileSystem();
        if (fs->isFile(shsStatusFilepath))
        {
            return extractOverallHealth(fs->readFile(shsStatusFilepath));
        }
        LOGWARN("Could not find the SHS status file at: " << shsStatusFilepath);
        return std::nullopt;
    }
    std::optional<std::string> BaseTelemetryReporter::getOutbreakModeCurrent()
    {
        Path outbreakModeStatusFilepath =
            Common::ApplicationConfiguration::applicationPathManager().getOutbreakModeStatusFilePath();
        auto fs = Common::FileSystem::fileSystem();
        if (fs->isFile(outbreakModeStatusFilepath))
        {
            return extractValueFromFile(outbreakModeStatusFilepath, "outbreak-mode");
        }
        return std::nullopt;
    }

    std::optional<std::string> BaseTelemetryReporter::getOutbreakModeHistoric() //just check if file exists
    {
        Path outbreakModeStatusFilepath =
            Common::ApplicationConfiguration::applicationPathManager().getOutbreakModeStatusFilePath();
        auto fs = Common::FileSystem::fileSystem();
        std::string everReachedOutbreak;
        if (fs->isFile(outbreakModeStatusFilepath))
        {
            return "true";
        }
        return "false";
    }

    std::optional<std::string> BaseTelemetryReporter::getOutbreakModeTodayWrapper()
    {
        Common::UtilityImpl::FormattedTime formattedTime;
        return getOutbreakModeToday(clock_t::now());
    }

    std::optional<std::string> BaseTelemetryReporter::getOutbreakModeToday(time_point_t now)
    {
        if (getOutbreakModeCurrent() == "true")
        {
            return "true";
        }

        Path outbreakModeStatusFilepath =
            Common::ApplicationConfiguration::applicationPathManager().getOutbreakModeStatusFilePath();
        auto fs = Common::FileSystem::fileSystem();
        if (fs->isFile(outbreakModeStatusFilepath))
        {
            std::optional<std::string> recordedTime = extractValueFromFile(outbreakModeStatusFilepath, "timestamp");
            if (recordedTime)
            {
                auto nowTime = clock_t::to_time_t(now);
                auto recordedTimeAsTime = Common::UtilityImpl::TimeUtils::toTime(recordedTime.value());
                if (difftime(nowTime, recordedTimeAsTime) > 86400) // 1 day in seconds
                {
                    return "true";
                }
                return "false";
            }
            LOGWARN("Could not parse timestamp of last outbreak event at: " << outbreakModeStatusFilepath);
            return std::nullopt;
        }
        return std::nullopt;
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

    std::optional<std::string> extractOverallHealth(const std::string& shsStatusXml)
    {
        try
        {
            Common::XmlUtilities::AttributesMap attributesMap = Common::XmlUtilities::parseXml(shsStatusXml);

            std::string items = "health/item";
            auto matchingPaths = attributesMap.entitiesThatContainPath(items);
            if (matchingPaths.empty())
            {
                LOGWARN("No Health items present in the SHS status XML");
                return std::nullopt;
            }

            std::string expectedName = "health";
            for (auto& matchingPath : matchingPaths)
            {
                Common::XmlUtilities::Attributes attributes = attributesMap.lookup(matchingPath);
                std::string name = attributes.value("name");
                if (name == expectedName)
                {
                    return attributes.value("value");
                }
            }
            LOGWARN("Overall Health not present in the SHS status XML");
        }
        catch (Common::XmlUtilities::XmlUtilitiesException& ex)
        {
            LOGWARN("Invalid status XML received. Error: " << ex.what());
        }
        return std::nullopt;
    }
} // namespace Telemetry