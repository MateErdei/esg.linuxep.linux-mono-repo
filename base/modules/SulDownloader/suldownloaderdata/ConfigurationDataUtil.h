/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "Common/Policy/UpdateSettings.h"

namespace SulDownloader
{
    namespace suldownloaderdata
    {
        class ConfigurationDataUtil
        {
        public:
            /**
            * Function will return true If:
            * either the subscription tag, or the subscription fixedVersion differ from previous configuration,
            * will return false otherwise.
            */
            static bool checkIfShouldForceUpdate(const Common::Policy::UpdateSettings& configurationData,
                                                 const Common::Policy::UpdateSettings &previousConfigurationData);

            /**
             * Function will return true If:
             * Configuration Data set to force install
             * Rigid names or features differ from previous configuration
             * Not all components are listed in the installedproducts directory (with the exception of base)
             */
            static bool checkIfShouldForceInstallAllProducts(const Common::Policy::UpdateSettings& configurationData,
                                                             const Common::Policy::UpdateSettings& previousConfigurationData,
                                                             bool onlyCompareSubscriptionsAndFeatures = true);
        };
    }
}


