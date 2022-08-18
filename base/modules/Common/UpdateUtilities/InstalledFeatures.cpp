#include "InstalledFeatures.h"

#include "json.hpp"

#include "ApplicationConfigurationImpl/ApplicationPathManager.h"
#include "FileSystem/IFilePermissions.h"
#include "FileSystem/IFileSystem.h"
#include "UtilityImpl/ProjectNames.h"

#include <sys/stat.h>
#include <string>
#include <vector>

namespace Common::UpdateUtilities
{

    void writeInstalledFeaturesJsonFile(const std::vector<std::string>& features)
    {
        nlohmann::json jsonFeatures(features);
        auto fileSystem = Common::FileSystem::fileSystem();
        fileSystem->writeFile(
            Common::ApplicationConfiguration::applicationPathManager().getFeaturesJsonPath(), jsonFeatures.dump());
        auto fp = Common::FileSystem::filePermissions();
        fp->chown(Common::ApplicationConfiguration::applicationPathManager().getFeaturesJsonPath(), sophos::updateSchedulerUser(), sophos::group());
        fp->chmod(Common::ApplicationConfiguration::applicationPathManager().getFeaturesJsonPath(), S_IRUSR | S_IWUSR | S_IRGRP);
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