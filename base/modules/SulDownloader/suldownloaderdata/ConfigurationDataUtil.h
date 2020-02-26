/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <SulDownloader/suldownloaderdata/ConfigurationData.h>

namespace SulDownloader
{
    namespace suldownloaderdata
    {
        class ConfigurationDataUtil
        {
        public:
            /**
             * Function will return true If:
             * Configuration Data set to force install
             * Rigid names or features differ from previous configuration
             * Not all components are listed in the installedproducts directory (with the exception of base)
             */
            static bool checkIfShouldForceInstallAllProducts(const ConfigurationData& configurationData,
                                                             const ConfigurationData& previousConfigurationData,
                                                             bool onlyCompareSubscriptionsAndFeatures = true);
        };
    }
}


