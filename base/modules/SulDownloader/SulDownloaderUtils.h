// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include <SulDownloader/suldownloaderdata/ConfigurationData.h>
namespace SulDownloader
{
    class SulDownloaderUtils
    {
    public:
        static bool isEndpointPaused(const suldownloaderdata::ConfigurationData& configurationData);
        static bool checkIfWeShouldForceUpdates(const suldownloaderdata::ConfigurationData& configurationData);
    };
}


