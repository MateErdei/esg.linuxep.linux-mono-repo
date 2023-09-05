// Copyright 2021-2023 Sophos Limited. All rights reserved.

#include "UpdateSchedulerUtils.h"

#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "Common/FileSystem/IFileSystem.h"
#include "Common/FileSystem/IFileSystemException.h"
#include "Common/Policy/SerialiseUpdateSettings.h"
#include "Common/UtilityImpl/StringUtils.h"
#include "common/Logger.h"

#include <json.hpp>

using namespace Common::Policy;
using namespace Common::UtilityImpl;

namespace
{
    inline void rethrow_ex(const std::exception& e, int level) // NOLINT(misc-no-recursion)
    {
        try
        {
            std::rethrow_if_nested(e);
        }
        catch (const Common::Exceptions::IException& nestedException)
        {
            UpdateSchedulerImpl::log_exception(nestedException, level + 1);
        }
        catch (const std::exception& nestedException)
        {
            UpdateSchedulerImpl::log_exception(nestedException, level + 1);
        }
        catch (...)
        {
        }
    }
} // namespace

namespace UpdateSchedulerImpl
{
    void log_exception(const Common::Exceptions::IException& e, int level) // NOLINT(misc-no-recursion)
    {
        LOGERROR(std::string(level, ' ') << "exception: " << e.what_with_location());
        rethrow_ex(e, level);
    }

    void log_exception(const std::exception& e, int level) // NOLINT(misc-no-recursion)
    {
        LOGERROR(std::string(level, ' ') << "exception: " << e.what());
        rethrow_ex(e, level);
    }

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

        if (eventStateLastTime == "0" && goodInstallTime == "0" && installFailedTime == "0" &&
            downloadFailedTime == "0")
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
                LOGERROR("Failed to remove file " << path << " due to error " << ex.what());
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
                LOGERROR("Failed to read file " << path << " due to error " << ex.what());
            }
        }
        return "";
    }

    std::optional<UpdateSettings> UpdateSchedulerUtils::getPreviousConfigurationData()
    {
        Path previousConfigFilePath = Common::FileSystem::join(
            Common::ApplicationConfiguration::applicationPathManager().getSulDownloaderReportPath(),
            Common::ApplicationConfiguration::applicationPathManager().getPreviousUpdateConfigFileName());

        std::optional<UpdateSettings> previousConfigurationData;
        if (Common::FileSystem::fileSystem()->isFile(previousConfigFilePath))
        {
            LOGDEBUG("Previous update configuration file found.");
            previousConfigurationData = UpdateSchedulerUtils::getConfigurationDataFromJsonFile(previousConfigFilePath);
        }

        return previousConfigurationData;
    }

    std::optional<UpdateSettings> UpdateSchedulerUtils::getCurrentConfigurationData()
    {
        Path currentConfigFilePath =
            Common::ApplicationConfiguration::applicationPathManager().getSulDownloaderConfigFilePath();

        std::optional<UpdateSettings> currentConfigurationData;
        if (Common::FileSystem::fileSystem()->isFile(currentConfigFilePath))
        {
            LOGDEBUG("Current update configuration file found.");
            currentConfigurationData = UpdateSchedulerUtils::getConfigurationDataFromJsonFile(currentConfigFilePath);
        }

        return currentConfigurationData;
    }

    std::pair<UpdateSettings, bool> UpdateSchedulerUtils::getUpdateConfigWithLatestJWT()
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

            return { currentConfigData.value(), config_updated };
        }
        return { UpdateSettings(), false };
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
            LOGERROR("Failed to read file " << path << " due to error " << ex.what());
        }
        return token;
    }

    std::optional<UpdateSettings> UpdateSchedulerUtils::getConfigurationDataFromJsonFile(const std::string& filePath)
    {
        try
        {
            std::string configSettings = Common::FileSystem::fileSystem()->readFile(filePath);

            auto configurationData = SerialiseUpdateSettings::fromJsonSettings(configSettings);

            return configurationData;
        }
        catch (Common::FileSystem::IFileSystemException& ex)
        {
            LOGWARN("Failed to read configuration file : " << filePath);
        }

        return std::nullopt;
    }
} // namespace UpdateSchedulerImpl