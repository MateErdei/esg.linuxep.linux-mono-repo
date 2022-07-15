/***********************************************************************************************

Copyright 2022 Sophos Limited. All rights reserved.

***********************************************************************************************/

#include <Common/ApplicationConfigurationImpl/ApplicationPathManager.h>
#include <Common/FileSystem/IFileSystem.h>
#include <Common/FileSystem/IFileSystemException.h>
#include <SulDownloader/Logger.h>
#include <SulDownloader/sdds3/SDDS3Utils.h>

#include <Config.h>
#include <PackageRef.h>
#include <json.hpp>

namespace SulDownloader
{

    std::string createSUSRequestBody(const SUSRequestParameters& parameters)
    {
        nlohmann::json json;
        json["schema_version"] = 1;
        json["product"] = parameters.product;
        json["server"] = parameters.isServer;
        json["platform_token"] = parameters.platformToken;
        json["subscriptions"] = nlohmann::json::array();
        for (const auto& subscription : parameters.subscriptions)
        {
            std::map<std::string, std::string> subscriptionMap = { { "id", subscription.rigidName() }, { "tag", subscription.tag() }};
            if (!subscription.fixedVersion().empty())
            {
                subscriptionMap.insert({ "fixedVersion", subscription.fixedVersion() });
            }
            json["subscriptions"].push_back(subscriptionMap);
        }

        return json.dump();
    }

    void removeSDDS3Cache()
    {
        std::string sdds3DistributionPath =
            Common::ApplicationConfiguration::applicationPathManager().getLocalSdds3DistributionRepository();
        std::string sdds3RepositoryPath =
            Common::ApplicationConfiguration::applicationPathManager().getLocalSdds3Repository();
        std::string sdds3PackageConfigPath =
            Common::ApplicationConfiguration::applicationPathManager().getSdds3PackageConfigPath();

        LOGDEBUG("Purging local SDDS3 cache");
        try
        {
            auto fileSystem = Common::FileSystem::fileSystem();
            if (fileSystem->isDirectory(sdds3DistributionPath))
            {
                LOGINFO("Removing local SDDS3 distribution repository from cache: " + sdds3DistributionPath);
                fileSystem->recursivelyDeleteContentsOfDirectory(sdds3DistributionPath);
            }
            if (fileSystem->isDirectory(sdds3RepositoryPath))
            {
                LOGINFO("Removing local SDDS3 repository from cache: " + sdds3RepositoryPath);
                fileSystem->recursivelyDeleteContentsOfDirectory(sdds3RepositoryPath);
            }
            if (fileSystem->isFile(sdds3PackageConfigPath))
            {
                LOGINFO("Removing SDDS3 package config: " + sdds3PackageConfigPath);
                fileSystem->removeFile(sdds3PackageConfigPath);
            }
        }
        catch (Common::FileSystem::IFileSystemException& ex)
        {
            LOGWARN("Failed to delete SDDS3 cache, reason:" << ex.what());
        }
    }

} // namespace SulDownloader
