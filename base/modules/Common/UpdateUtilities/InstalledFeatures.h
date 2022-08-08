#pragma once

#include <string>
#include <vector>

namespace Common::UpdateUtilities
{
    /**
     * Persists a list of feature codes to disk in JSON format. Used by Update Scheduler so that it can correctly
     * generate ALC status feature code list on an update failure or when first started.
     * @param Reference to std::vector<std::string> which holds list of feature codes, e.g. CORE
     * @throws IFileSystemException if write fails.
     */
    void writeInstalledFeaturesJsonFile(const std::vector<std::string>& features);

    /**
     * Returns true if the installed features JSON file exists
     * @return bool
     */
    bool doesInstalledFeaturesListExist();

    /**
     * Returns the list of features that are currently installed, if there is no file then this returns an empty list.
     * @return std::vector<std::string> of feature codes, e.g. CORE
     * @throws IFileSystemException if read fails
     * @throws nlohmann::detail::exception if there's a json parsing error
     */
    std::vector<std::string> readInstalledFeaturesJsonFile();

    /**
     * Returns true if any of the features of a product are required but not installed.
     * @return bool
     */
    bool shouldProductBeInstalledBasedOnFeatures(
        const std::vector<std::string>& productFeatures,
        const std::vector<std::string>& installedFeatures,
        const std::vector<std::string>& requiredFeatures);

} // namespace Common::UpdateUtilities