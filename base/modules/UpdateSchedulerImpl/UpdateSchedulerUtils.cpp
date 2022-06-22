/******************************************************************************************************

Copyright 2021-2022 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "UpdateSchedulerUtils.h"
#include "Logger.h"

#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include <Common/FileSystem/IFileSystem.h>
#include <Common/FileSystem/IFileSystemException.h>
#include <Common/UtilityImpl/StringUtils.h>


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

    std::pair<SulDownloader::suldownloaderdata::ConfigurationData,bool> UpdateSchedulerUtils::getUpdateConfigWithLatestJWT()
    {
        SulDownloader::suldownloaderdata::ConfigurationData config;
        std::string path = Common::ApplicationConfiguration::applicationPathManager().getSulDownloaderConfigFilePath();
        auto fs = Common::FileSystem::fileSystem();
        std::string contents;
        std::pair<SulDownloader::suldownloaderdata::ConfigurationData,bool> pair = std::make_pair(config,false);
        try
        {
            if (fs->isFile(path))
            {
                contents = fs->readFile(path);
            }
            else
            {
                return pair;
            }
        }
        catch (Common::FileSystem::IFileSystemException& ex)
        {
            LOGERROR("Failed to read file "<< path << " due to error "<< ex.what());

            return pair;
        }
        config = SulDownloader::suldownloaderdata::ConfigurationData::fromJsonSettings(contents);

        std::string token = getJWToken();
        if (token != config.getJWToken())
        {
            config.setJWToken(token);
            pair.first = config;
            pair.second = true;
        }

        return pair;
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

    bool UpdateSchedulerUtils::isFlagSet(const std::string& flag, const std::string& flagContent)
    {
        bool flagValue = false;

        try
        {
            nlohmann::json j = nlohmann::json::parse(flagContent);

            if (j.find(flag) != j.end())
            {
                if (j[flag] == true)
                {
                    flagValue = true;
                }
            }
        }
        catch (nlohmann::json::parse_error& ex)
        {
            std::stringstream errorMessage;
            errorMessage << "Could not parse json: " << flagContent << " with error: " << ex.what();
            LOGWARN(errorMessage.str());
        }

        return flagValue;
    }
}