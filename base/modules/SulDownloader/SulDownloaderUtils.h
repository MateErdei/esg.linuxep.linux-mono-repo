// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/Policy/UpdateSettings.h"

namespace SulDownloader
{
    class SulDownloaderUtils
    {
    public:
        static bool isEndpointPaused(const Common::Policy::UpdateSettings& configurationData);
        static bool checkIfWeShouldForceUpdates(const Common::Policy::UpdateSettings& configurationData);
    };
}


