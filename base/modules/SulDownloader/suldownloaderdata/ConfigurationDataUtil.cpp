/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ConfigurationDataUtil.h"

#include <Common/FileSystem/IFileSystem.h>
#include <Common/ApplicationConfiguration/IApplicationPathManager.h>

#include <algorithm>

namespace SulDownloader
{
    namespace suldownloaderdata
    {
        bool ConfigurationDataUtil::checkIfShouldForceInstallAllProducts(const ConfigurationData configurationData,
                                                                         const ConfigurationData previousConfigurationData,
                                                                         bool onlyCompareSubscriptionsAndFeatures) {
            if (!previousConfigurationData.isVerified())
            {
                // if Configuration data is not verified no previous configuration data provided
                return false;
            }

            if (configurationData.getForceReinstallAllProducts() ||
                (configurationData.getProductsSubscription().size() !=
                 previousConfigurationData.getProductsSubscription().size()) ||
                (configurationData.getFeatures().size() != previousConfigurationData.getFeatures().size()))
            {
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
                return true;
            }

            std::sort(newSortedFeatures.begin(), newSortedFeatures.end());
            std::sort(previousSortedFeatures.begin(), previousSortedFeatures.end());

            if (newSortedFeatures != previousSortedFeatures)
            {
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
                        return true;
                    }
                }
            }

            return false;
        }
    }
}