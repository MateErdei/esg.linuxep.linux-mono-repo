// Copyright 2023 Sophos Limited. All rights reserved.

#include "SulDownloaderUtils.h"
#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "Common/FileSystem/IFileSystem.h"
namespace SulDownloader
{
    bool SulDownloaderUtils::isEndpointPaused(const suldownloaderdata::ConfigurationData& configurationData)
    {
        std::vector<suldownloaderdata::ProductSubscription> subs = configurationData.getProductsSubscription();
        for (const auto& subscription : subs)
        {
            if (!subscription.fixedVersion().empty())
            {
                return true;
            }
        }

        if (!configurationData.getPrimarySubscription().fixedVersion().empty())
        {
            return true;
        }
        return false;
    }

    bool SulDownloaderUtils::checkIfWeShouldForceUpdates(const suldownloaderdata::ConfigurationData& configurationData)
    {
        auto& pathManager = Common::ApplicationConfiguration::applicationPathManager();
        auto fs = Common::FileSystem::fileSystem();

        bool paused = isEndpointPaused(configurationData);

        std::string markerFile = pathManager.getForcedAnUpdateMarkerPath();
        if (configurationData.getDoForcedUpdate())
        {
            if (!fs->isFile(markerFile) && !paused)
            {
                return true;
            }
        }
        else
        {

            fs->removeFile(markerFile,true);

        }

        std::string pausedMarkerFile = pathManager.getForcedAPausedUpdateMarkerPath();
        if (configurationData.getDoPausedForcedUpdate())
        {
            if (paused && !fs->isFile(pausedMarkerFile))
            {
                return true;
            }
        }
        else
        {
            fs->removeFile(pausedMarkerFile,true);
        }

        return false;
    }

}