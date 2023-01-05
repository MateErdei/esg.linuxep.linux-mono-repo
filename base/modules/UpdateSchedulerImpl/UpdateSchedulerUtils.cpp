/******************************************************************************************************

Copyright 2021-2022 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/FileSystem/IFileSystem.h>
#include <Common/FileSystem/IFileSystemException.h>
#include <Common/UtilityImpl/StringUtils.h>
#include <SulDownloader/suldownloaderdata/SulDownloaderException.h>
#include <UpdateSchedulerImpl/Logger.h>
#include <UpdateSchedulerImpl/UpdateSchedulerUtils.h>

#include <json.hpp>
namespace UpdateSchedulerImpl
{

    std::string UpdateSchedulerUtils::calculateHealth(StateData::StateMachineData stateMachineData)
    {

        nlohmann::json healthResponseMessage;
        healthResponseMessage["downloadState"] = 0;
        healthResponseMessage["installState"] = 0;
        healthResponseMessage["overall"] = 0;

        auto const& eventStateLastTime = stateMachineData.getEventStateLastTime();
        auto const& goodInstallTime = stateMachineData.getLastGoodInstallTime();
        auto const& installFailedTime = stateMachineData.getInstallFailedSinceTime();
        auto const& downloadFailedTime = stateMachineData.getDownloadFailedSinceTime();

        if (eventStateLastTime == "0" && goodInstallTime == "0"
            && installFailedTime == "0" && downloadFailedTime == "0" )
        {
            // update has not run yet
            return healthResponseMessage.dump();
        }
        auto const& downloadState = stateMachineData.getDownloadState();
        auto const& installState = stateMachineData.getInstallState();

        if (downloadState != "0")
        {
            healthResponseMessage["downloadState"] = 1;
            healthResponseMessage["overall"] = 1;
        }
        if (installState != "0")
        {
            healthResponseMessage["installState"] = 1;
            healthResponseMessage["overall"] = 1;
        }

        return healthResponseMessage.dump();
    }

    void UpdateSchedulerUtils::cleanUpMarkerFile()
    {
        auto fs = Common::FileSystem::fileSystem();
        std::string path = Common::ApplicationConfiguration::applicationPathManager().getUpdateMarkerFile();
        if (fs->isFile(path))
        {
            LOGWARN("Upgrade marker file left behind after suldownloader finished, cleaning up now");
            try
            {
                fs->removeFile(path);
            }
            catch (Common::FileSystem::IFileSystemException& ex)
            {
                LOGERROR("Failed to remove file "<< path << " due to error "<< ex.what());
            }
        }
    }
    std::string UpdateSchedulerUtils::readMarkerFile()
    {
        auto fs = Common::FileSystem::fileSystem();
        std::string path = Common::ApplicationConfiguration::applicationPathManager().getUpdateMarkerFile();
        if (fs->isFile(path))
        {
            try
            {
                return fs->readFile(path);
            }
            catch (Common::FileSystem::IFileSystemException& ex)
            {
                LOGERROR("Failed to read file "<< path << " due to error "<< ex.what());
            }
        }
        return "";
    }

    std::optional<SulDownloader::suldownloaderdata::ConfigurationData> UpdateSchedulerUtils::getPreviousConfigurationData()
    {
        Path previousConfigFilePath = Common::FileSystem::join(
            Common::ApplicationConfiguration::applicationPathManager().getSulDownloaderReportPath(),
            Common::ApplicationConfiguration::applicationPathManager().getPreviousUpdateConfigFileName());

        std::optional<SulDownloader::suldownloaderdata::ConfigurationData> previousConfigurationData;
        if (Common::FileSystem::fileSystem()->isFile(previousConfigFilePath))
        {
            LOGDEBUG("Previous update configuration file found.");
            previousConfigurationData = UpdateSchedulerUtils::getConfigurationDataFromJsonFile(previousConfigFilePath);
        }

        return previousConfigurationData;
    }

    std::optional<SulDownloader::suldownloaderdata::ConfigurationData> UpdateSchedulerUtils::getCurrentConfigurationData()
    {
        Path currentConfigFilePath = Common::ApplicationConfiguration::applicationPathManager().getSulDownloaderConfigFilePath();

        std::optional<SulDownloader::suldownloaderdata::ConfigurationData> currentConfigurationData;
        if (Common::FileSystem::fileSystem()->isFile(currentConfigFilePath))
        {
            LOGDEBUG("Current update configuration file found.");
            currentConfigurationData = UpdateSchedulerUtils::getConfigurationDataFromJsonFile(currentConfigFilePath);
        }

        return currentConfigurationData;
    }

    std::pair<SulDownloader::suldownloaderdata::ConfigurationData,bool> UpdateSchedulerUtils::getUpdateConfigWithLatestJWT()
    {
        bool config_updated = false;
        auto currentConfigData = getCurrentConfigurationData();
        if (currentConfigData.has_value())
        {
            std::string token = getJWToken();
            if (token != currentConfigData.value().getJWToken())
            {
                currentConfigData.value().setJWToken(token);
                config_updated = true;
            }

            std::string device_id = getDeviceId();
            if (device_id != currentConfigData.value().getDeviceId())
            {
                currentConfigData.value().setDeviceId(device_id);
                config_updated = true;
            }

            std::string tenant_id = getTenantId();
            if (tenant_id != currentConfigData.value().getTenantId())
            {
                currentConfigData.value().setTenantId(tenant_id);
                config_updated = true;
            }

            return {currentConfigData.value(), config_updated};

        }
        return {SulDownloader::suldownloaderdata::ConfigurationData(), false};
    }

    std::string UpdateSchedulerUtils::getDeviceId()
    {
        return getValueFromMCSConfig("device_id");
    }
    std::string UpdateSchedulerUtils::getTenantId()
    {
        return getValueFromMCSConfig("tenant_id");
    }
    std::string UpdateSchedulerUtils::getJWToken()
    {
        return getValueFromMCSConfig("jwt_token");
    }

    std::string UpdateSchedulerUtils::getSDDSMechanism(bool sdds3Enabled)
    {
        if (sdds3Enabled)
        {
            return "SDDS3";
        }
        return "SDDS2";
    }

    std::string UpdateSchedulerUtils::getValueFromMCSConfig(const std::string& key)
    {
        auto fs = Common::FileSystem::fileSystem();
        std::string path = Common::ApplicationConfiguration::applicationPathManager().getMcsConfigFilePath();
        std::string token;
        try
        {
            if (fs->isFile(path))
            {
                token = Common::UtilityImpl::StringUtils::extractValueFromConfigFile(path, key);
            }
            else
            {
                LOGWARN("mcs.config file not found at path " << path);
            }
        }
        catch (Common::FileSystem::IFileSystemException& ex)
        {
            LOGERROR("Failed to read file "<< path << " due to error "<< ex.what());
        }
        return token;
    }

    std::optional<SulDownloader::suldownloaderdata::ConfigurationData> UpdateSchedulerUtils::getConfigurationDataFromJsonFile(const std::string& filePath)
    {
        try
        {
            std::string configSettings = Common::FileSystem::fileSystem()->readFile(filePath);

            SulDownloader::suldownloaderdata::ConfigurationData configurationData =
                SulDownloader::suldownloaderdata::ConfigurationData::fromJsonSettings(configSettings);

            return configurationData;
        }
        catch (SulDownloader::suldownloaderdata::SulDownloaderException& ex)
        {
            LOGWARN("Failed to load configuration settings from : " << filePath);
        }
        catch (Common::FileSystem::IFileSystemException& ex)
        {
            LOGWARN("Failed to read configuration file : " << filePath);
        }

        return std::nullopt;
    }
}