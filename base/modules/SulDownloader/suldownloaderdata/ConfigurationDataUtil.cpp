/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ConfigurationDataUtil.h"

#include <Common/FileSystem/IFileSystem.h>
#include <Common/ApplicationConfiguration/IApplicationPathManager.h>
#include "Logger.h"

#include <algorithm>

namespace SulDownloader
{
    namespace suldownloaderdata
    {
        bool ConfigurationDataUtil::checkIfShouldForceInstallAllProducts(const ConfigurationData& configurationData,
                                                                         const ConfigurationData& previousConfigurationData,
                                                                         bool onlyCompareSubscriptionsAndFeatures)
        {
            /*
             * This function will be called from UpdateScheduler and SulDownloadeder.
             * When called from UpdateScheduler the Subscriptions and Features will be compared.
             * The UpdateScheduler can also to invoke the ConfigurationData verify process due to permissions,
             * therefore this check is also skipped.
             *
             * When called from SulDownloader all checks are performed.
             */

            LOGDEBUG("Checking if should force install on all products");
            if (!onlyCompareSubscriptionsAndFeatures && !previousConfigurationData.isVerified())
            {
                // if Configuration data is not verified no previous configuration data provided
                LOGDEBUG("Previous update configuration data has not been set or verified.");
                return false;
            }

            if (configurationData.getForceReinstallAllProducts() ||
                (configurationData.getProductsSubscription().size() !=
                 previousConfigurationData.getProductsSubscription().size()) ||
                (configurationData.getFeatures().size() != previousConfigurationData.getFeatures().size()))
            {
                LOGDEBUG("Found new subscription or features in update configuration.");
                return true;
            }

            std::vector <std::string> newSortedRigidNames;
            std::vector <std::string> previousSortedRigidNames;
            std::vector <std::string> newSortedFeatures;
            std::vector <std::string> previousSortedFeatures;

            for (auto &subscription : configurationData.getProductsSubscription())
            {
                newSortedRigidNames.push_back(subscription.rigidName());
            }

            for (auto &subscription : previousConfigurationData.getProductsSubscription())
            {
                previousSortedRigidNames.push_back(subscription.rigidName());
            }

            for (auto &feature : configurationData.getFeatures())
            {
                newSortedFeatures.push_back(feature);
            }

            for (auto &feature : previousConfigurationData.getFeatures())
            {
                previousSortedFeatures.push_back(feature);
            }

            std::sort(newSortedRigidNames.begin(), newSortedRigidNames.end());

            std::sort(previousSortedRigidNames.begin(), previousSortedRigidNames.end());

            if (newSortedRigidNames != previousSortedRigidNames)
            {
                LOGDEBUG("Feature list in update configuration has changed.");
                return true;
            }

            std::sort(newSortedFeatures.begin(), newSortedFeatures.end());
            std::sort(previousSortedFeatures.begin(), previousSortedFeatures.end());

            if (newSortedFeatures != previousSortedFeatures)
            {
                LOGDEBUG("Subscription list in update configuration has changed.");
                return true;
            }


            if (!onlyCompareSubscriptionsAndFeatures)
            {
                auto fileSystem = Common::FileSystem::fileSystem();

                std::vector <Path> installedFiles = fileSystem->listFiles(
                        Common::ApplicationConfiguration::applicationPathManager().getLocalUninstallSymLinkPath());

                for (auto &rigidName : newSortedRigidNames)
                {
                    if (!fileSystem->exists(rigidName + ".sh"))
                    {
                        LOGDEBUG("Component in subscription list has been previously removed, or has not been installed.");
                        return true;
                    }
                }
            }

            LOGDEBUG("No difference between new update config and previous update config.");
            return false;
        }
    }
}