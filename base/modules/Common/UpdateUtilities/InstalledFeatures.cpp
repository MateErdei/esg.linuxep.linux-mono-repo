// Copyright 2022-2023 Sophos Limited. All rights reserved.

#include "InstalledFeatures.h"

#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "Common/FileSystem/IFilePermissions.h"
#include "Common/FileSystem/IFileSystem.h"
#include "Common/UtilityImpl/ProjectNames.h"

#include <sys/stat.h>

#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace Common::UpdateUtilities
{

    void writeInstalledFeaturesJsonFile(const std::vector<std::string>& features)
    {
        nlohmann::json jsonFeatures(features);

        std::string tempDir = ApplicationConfiguration::applicationPathManager().getTempPath();
        std::string filepath = Common::ApplicationConfiguration::applicationPathManager().getFeaturesJsonPath();

        Common::FileSystem::createAtomicFileWithPermissions(
            jsonFeatures.dump(),
            filepath,
            tempDir,
            sophos::updateSchedulerUser(),
            sophos::group(),
            S_IRUSR | S_IWUSR | S_IRGRP);
    }

    bool doesInstalledFeaturesListExist()
    {
        auto fileSystem = Common::FileSystem::fileSystem();
        return fileSystem->exists(Common::ApplicationConfiguration::applicationPathManager().getFeaturesJsonPath());
    }

    std::vector<std::string> readInstalledFeaturesJsonFile()
    {
        if (!doesInstalledFeaturesListExist())
        {
            return {};
        }
        auto fileSystem = Common::FileSystem::fileSystem();
        std::string featureCodes =
            fileSystem->readFile(Common::ApplicationConfiguration::applicationPathManager().getFeaturesJsonPath());
        nlohmann::json jsonFeatures = nlohmann::json::parse(featureCodes);
        return jsonFeatures.get<std::vector<std::string>>();
    }

    bool shouldProductBeInstalledBasedOnFeatures(
        const std::vector<std::string>& productFeatures,
        const std::vector<std::string>& installedFeatures,
        const std::vector<std::string>& requiredFeatures)
    {
        // If a package has any features that are in the configured features that we want but are not installed already
        // then return true to indicate product should be marked as changed
        std::vector<std::string> featuresRequiredButNotInstalled;
        std::set_difference(
            requiredFeatures.cbegin(),
            requiredFeatures.cend(),
            installedFeatures.cbegin(),
            installedFeatures.cend(),
            std::inserter(featuresRequiredButNotInstalled, featuresRequiredButNotInstalled.begin()));

        for (const auto& productFeature : productFeatures)
        {
            if (std::count(
                    featuresRequiredButNotInstalled.cbegin(), featuresRequiredButNotInstalled.cend(), productFeature))
            {
                return true;
            }
        }
        return false;
    }

} // namespace Common::UpdateUtilities