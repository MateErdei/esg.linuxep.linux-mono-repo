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

    void parseSUSResponse(
        const std::string& response,
        std::set<std::string>& suites,
        std::set<std::string>& releaseGroups)
    {
        try
        {
            auto json = nlohmann::json::parse(response);

            if (json.contains("suites"))
            {
                for (const auto& s : json["suites"].items())
                {
                    suites.insert(std::string(s.value()));
                }
            }

            if (json.contains("release-groups"))
            {
                for (const auto& g : json["release-groups"].items())
                {
                    releaseGroups.insert(std::string(g.value()));
                }
            }
        }
        catch (const std::exception& exception)
        {
            std::stringstream errorMessage;
            errorMessage << "Failed to parse SUS response with error: " << exception.what() << ", JSON content: " << response;
            throw std::runtime_error(errorMessage.str());
        }
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
