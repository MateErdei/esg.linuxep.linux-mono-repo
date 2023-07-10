// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/Policy/UpdateSettings.h"

namespace SulDownloader
{
    class SulDownloaderUtils
    {
    public:
        [[nodiscard]] static bool isEndpointPaused(const Common::Policy::UpdateSettings& configurationData);
        [[nodiscard]] static bool checkIfWeShouldForceUpdates(const Common::Policy::UpdateSettings& configurationData);
        /**
         * Stop the whole spl product
         */
        static void stopProduct();

        /**
         * Stop the whole spl product
         */
        static void startProduct();

        /**
         * Check if the product is running
         */
        [[nodiscard]] static bool isProductRunning();

        /**
         * Check if all the updated components are running
         */
        static std::vector<std::string> checkUpdatedComponentsAreRunning();

    private:
        [[nodiscard]] static bool waitForComponentToRun(const std::string& component);
        [[nodiscard]] static bool isComponentIsRunning(const std::string& component);
    };
}


