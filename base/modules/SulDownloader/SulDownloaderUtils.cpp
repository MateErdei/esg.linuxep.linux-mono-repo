// Copyright 2023 Sophos Limited. All rights reserved.

#include "SulDownloaderUtils.h"
#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "Common/FileSystem/IFileSystem.h"

using namespace Common::Policy;

namespace SulDownloader
{
    bool SulDownloaderUtils::isEndpointPaused(const UpdateSettings& updateSettings)
    {
        std::vector<Common::Policy::ProductSubscription> subs = updateSettings.getProductsSubscription();
        for (const auto& subscription : subs)
        {
            if (!subscription.fixedVersion().empty())
            {
                return true;
            }
        }

        if (!updateSettings.getPrimarySubscription().fixedVersion().empty())
        {
            return true;
        }
        return false;
    }

    bool SulDownloaderUtils::checkIfWeShouldForceUpdates(const UpdateSettings& updateSettings)
    {
        auto& pathManager = Common::ApplicationConfiguration::applicationPathManager();
        auto fs = Common::FileSystem::fileSystem();

        bool paused = isEndpointPaused(updateSettings);

        std::string markerFile = pathManager.getForcedAnUpdateMarkerPath();
        if (updateSettings.getDoForcedUpdate())
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
        if (updateSettings.getDoPausedForcedUpdate())
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