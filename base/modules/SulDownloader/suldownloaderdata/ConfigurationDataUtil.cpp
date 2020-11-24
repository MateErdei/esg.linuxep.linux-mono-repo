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
        bool ConfigurationDataUtil::checkIfShouldForceUpdate(const ConfigurationData &configurationData,
                                                             const ConfigurationData &previousConfigurationData)
        {
            auto &newPrimarySubscription = configurationData.getPrimarySubscription();
            auto &previousPrimarySubscription = previousConfigurationData.getPrimarySubscription();

            if(newPrimarySubscription.rigidName() != previousPrimarySubscription.rigidName()
               || newPrimarySubscription.tag() != previousPrimarySubscription.tag()
               || newPrimarySubscription.fixedVersion() != previousPrimarySubscription.fixedVersion())
            {
                return true;
            }

            for (auto &newSubscription : configurationData.getProductsSubscription())
            {
                for (auto &previousSubscription : previousConfigurationData.getProductsSubscription())
                {
                    if (newSubscription.rigidName() == previousSubscription.rigidName())
                    {
                        if(newSubscription.tag() != previousSubscription.tag()
                        || newSubscription.fixedVersion() != previousSubscription.fixedVersion())
                        {
                            return true;
                        }
                        else
                        {
                            break;
                        }
                    }
                }
            }

            return false;
        }


        bool ConfigurationDataUtil::checkIfShouldForceInstallAllProducts(const ConfigurationData& configurationData,
                                                                         const ConfigurationData& previousConfigurationData,
                                                                         bool onlyCompareSubscriptionsAndFeatures)
        {
            /*
             * This function will be called from UpdateScheduler and SulDownloader.
             * When called from UpdateScheduler the Subscriptions and Features will be compared.
             * The UpdateScheduler cannot invoke the ConfigurationData verify process due to permissions,
             * therefore this check is also skipped.
             * When called from SulDownloader all checks are performed.
             *
             * These reason for both performing the checks is because update scheduler has no knowledge of the warehouse,
             * so triggering an update may not actually trigger an update that results in the components install.sh being called.
             *
             *
             * Scenarios this check is covering is base around the following problem
             *
             * A component has been downloaded previously and then been uninstalled, either manually (unsupported) or
             * via policy change.  If this happens Update Scheduler will detect that the product (subscription) or feature set has changed, however
             * if the files are in the primarywarehouse (local warehouse) and the remote warehouse are in sync the download
             * will just report nothing has changed, and the uninstalled / missing feature or component install.sh script will not be called.
             *
             * As a component can have multiple features and features can be listed against multiple components,
             * it is possible to have the set of features updated for component(s) that are already installed.  If the feature
             * has been previously added and removed, it is likely that we hit the main problem described above, thus we need to force an install.
             *
             * If the subscriptions are different then we need to force an install, because sulDownloader at this point in time has no
             * record of the past component installs / uninstalls, and does not know the state of the local warehouse, and if it is in sync with
             * the remote warehouse.
             *
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
                LOGDEBUG("Subscription list in update configuration has changed.");
                return true;
            }

            std::sort(newSortedFeatures.begin(), newSortedFeatures.end());
            std::sort(previousSortedFeatures.begin(), previousSortedFeatures.end());

            if (newSortedFeatures != previousSortedFeatures)
            {
                LOGDEBUG("Feature list in update configuration has changed.");
                return true;
            }

            LOGDEBUG("No difference between new update config and previous update config.");
            return false;
        }
    }
}