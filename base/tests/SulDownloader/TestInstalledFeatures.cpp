/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Common/Logging/ConsoleLoggingSetup.h"
#include "Common/UpdateUtilities/InstalledFeatures.h"
#include <gtest/gtest.h>

using namespace Common::UpdateUtilities;

class InstalledFeaturesTest : public ::testing::Test
{
};

TEST_F(InstalledFeaturesTest, testRequestedProductNotInstalled) // NOLINT
{
    std::vector<std::string> productFeatures{ "liveterminal" };
    std::vector<std::string> installedFeatures{ "CORE" };
    std::vector<std::string> requiredFeatures{ "CORE", "liveterminal" };
    ASSERT_TRUE(shouldProductBeInstalledBasedOnFeatures(productFeatures, installedFeatures, requiredFeatures));
}

TEST_F(InstalledFeaturesTest, testUnrequiredProductIsNotMarkedForInstall) // NOLINT
{
    std::vector<std::string> productFeatures{ "liveterminal" };
    std::vector<std::string> installedFeatures{ "CORE", "liveterminal" };
    std::vector<std::string> requiredFeatures{ "CORE" };
    ASSERT_FALSE(shouldProductBeInstalledBasedOnFeatures(productFeatures, installedFeatures, requiredFeatures));
}

TEST_F(InstalledFeaturesTest, testShouldProductBeInstalledBasedOnFeaturesHandlesEmpty) // NOLINT
{
    std::vector<std::string> productFeatures;
    std::vector<std::string> installedFeatures;
    std::vector<std::string> requiredFeatures;
    ASSERT_FALSE(shouldProductBeInstalledBasedOnFeatures(productFeatures, installedFeatures, requiredFeatures));
}

TEST_F(InstalledFeaturesTest, testProductIsNotRequestedWhenNoFeatureRequired) // NOLINT
{
    std::vector<std::string> productFeatures{ "liveterminal" };
    std::vector<std::string> installedFeatures{ "CORE", "liveterminal" };
    std::vector<std::string> requiredFeatures;
    ASSERT_FALSE(shouldProductBeInstalledBasedOnFeatures(productFeatures, installedFeatures, requiredFeatures));
}

TEST_F(InstalledFeaturesTest, testProductAlreadyInstalledReturnsFalse) // NOLINT
{
    std::vector<std::string> productFeatures{ "CORE" };
    std::vector<std::string> installedFeatures{ "CORE", "liveterminal" };
    std::vector<std::string> requiredFeatures{ "CORE" };
    ASSERT_FALSE(shouldProductBeInstalledBasedOnFeatures(productFeatures, installedFeatures, requiredFeatures));
}