// Copyright 2019-2023 Sophos Limited. All rights reserved.

#include "BaseTelemetryReporter.h"

#include "TelemetryProcessor.h"

#include "Common/UtilityImpl/TimeUtils.h"

#include <Common/ApplicationConfigurationImpl/ApplicationPathManager.h>
#include <Common/FileSystem/IFileSystemException.h>
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
        std::string outbreak;
        std::optional<nlohmann::json> outbreakStatusContents = parseOutbreakStatusFile();
        if (outbreakStatusContents.has_value() && outbreakStatusContents->contains("outbreak-mode") &&
            outbreakStatusContents->at("outbreak-mode").is_boolean())
        {
            return outbreakStatusContents->at("outbreak-mode") ? "true" : "false";
        }
        return std::nullopt;
    }

    std::optional<std::string> BaseTelemetryReporter::getOutbreakModeHistoric() //just check if file exists
    {
        Path outbreakModeStatusFilepath =
            Common::ApplicationConfiguration::applicationPathManager().getOutbreakModeStatusFilePath();
        auto fs = Common::FileSystem::fileSystem();
        if (fs->isFile(outbreakModeStatusFilepath))
        {
            return "true";
        }
        return "false";
    }

    std::optional<std::string> BaseTelemetryReporter::getOutbreakModeTodayWrapper()
    {
        return getOutbreakModeToday(clock_t::now());
    }

    std::optional<std::string> BaseTelemetryReporter::getOutbreakModeToday(time_point_t now)
    {
        std::optional<nlohmann::json> outbreakStatusContents = parseOutbreakStatusFile();
        std::string recordedTime;
        if (outbreakStatusContents.has_value() && outbreakStatusContents->contains("timestamp") &&
            outbreakStatusContents->at("timestamp").is_string())
        {
            recordedTime = outbreakStatusContents.value().value("timestamp", "");
            if (!recordedTime.empty())
            {
                auto nowTime = clock_t::to_time_t(now);
                auto recordedTimeAsTime = Common::UtilityImpl::TimeUtils::toUTCTime(recordedTime, "%FT%T");
                if (recordedTimeAsTime == 0)
                {
                    LOGWARN("Unable to parse outbreak mode timestamp to time: " << recordedTime);
                    return std::nullopt;
                }
                auto timeDifference = difftime(nowTime, recordedTimeAsTime);
                if (timeDifference <= 86400 && timeDifference >= 0) // 1 day in seconds
                {
                    return "true";
                }
                else if (timeDifference < 0)
                {
                    LOGERROR("System Clock has gone backwards: now="
                             <<nowTime
                             << " outbreakStartTime="<<recordedTimeAsTime
                             << " difference="<<timeDifference);
                }
                else
                {
                    LOGINFO("Outbreak more than 1 day ago: difference=" << timeDifference);
                }
                return "false";
            }
        }
        return std::nullopt;
    }

    std::optional<nlohmann::json> parseOutbreakStatusFile()
    {
        Path outbreakModeStatusFilepath =
            Common::ApplicationConfiguration::applicationPathManager().getOutbreakModeStatusFilePath();
        auto fs = Common::FileSystem::fileSystem();
        if (fs->isFile(outbreakModeStatusFilepath))
        {
            try
            {
                auto fileContents = fs->readFile(outbreakModeStatusFilepath);
                return nlohmann::json::parse(fileContents);
            }
            catch (Common::FileSystem::IFileSystemException& e)
            {
                LOGWARN("Unable to read file at: " << outbreakModeStatusFilepath << " with error: " << e.what());
            }
            catch (nlohmann::json::parse_error& e)
            {
                LOGWARN("Unable to parse json at: " << outbreakModeStatusFilepath << " with error: " << e.what());
            }
        }
        return std::nullopt;
    }

    std::optional<std::string> extractValueFromFile(const Path& filePath, const std::string& key)
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