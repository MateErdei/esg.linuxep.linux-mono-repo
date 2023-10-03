// Copyright 2020-2023 Sophos Limited. All rights reserved.

#include "SophosServerTable.h"

#include "Logger.h"

#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "Common/FileSystem/IFileSystem.h"
#include "Common/UtilityImpl/FileUtils.h"
#include "Common/UtilityImpl/StringUtils.h"

#include <nlohmann/json.hpp>

#include <iostream>

namespace
{
    /**
     *
     * @return A JSON string of an array containing each component, in the format
     * [{"Name": "...", "installed_version": "..."}, ...]
     * Base is always the first component listed
     * Returns an empty string if base version can't be determined
     */
    [[nodiscard]] std::string getInstalledVersions()
    {
        nlohmann::json result = nlohmann::json::array();

        // Base
        {
            std::string baseVersionIniPath =
                Common::ApplicationConfiguration::applicationPathManager().getVersionFilePath();
            std::string baseName, baseVersion;
            try
            {
                baseName =
                    Common::UtilityImpl::StringUtils::extractValueFromIniFile(baseVersionIniPath, "PRODUCT_NAME");
            }
            catch (const std::runtime_error& e)
            {
                LOGWARN("Failed to read PRODUCT_NAME from " << baseVersionIniPath << ": " << e.what());
                return "[]";
            }
            if (baseName.empty())
            {
                LOGWARN("PRODUCT_NAME not found in " << baseVersionIniPath);
                return "[]";
            }

            try
            {
                baseVersion =
                    Common::UtilityImpl::StringUtils::extractValueFromIniFile(baseVersionIniPath, "PRODUCT_VERSION");
            }
            catch (const std::runtime_error& e)
            {
                LOGWARN("Failed to read PRODUCT_VERSION from " << baseVersionIniPath << ": " << e.what());
                return "[]";
            }
            if (baseVersion.empty())
            {
                LOGWARN("PRODUCT_VERSION not found in " << baseVersionIniPath);
                return "[]";
            }

            result += { { "Name", baseName }, { "installed_version", baseVersion } };
        }

        // Plugins
        std::string installPath = Common::ApplicationConfiguration::applicationPathManager().sophosInstall();
        auto fileSystem = Common::FileSystem::fileSystem();
        auto pluginDirectories = fileSystem->listDirectories(Common::FileSystem::join(installPath, "plugins"));

        for (const auto& pluginDirectory : pluginDirectories)
        {
            std::string pluginVersionIniPath = Common::FileSystem::join(pluginDirectory, "VERSION.ini");
            std::string pluginName, pluginVersion;
            try
            {
                pluginName =
                    Common::UtilityImpl::StringUtils::extractValueFromIniFile(pluginVersionIniPath, "PRODUCT_NAME");
            }
            catch (const std::runtime_error& e)
            {
                LOGWARN("Failed to read PRODUCT_NAME from " << pluginVersionIniPath << ": " << e.what());
                continue;
            }
            if (pluginName.empty())
            {
                LOGWARN("PRODUCT_NAME not found in " << pluginVersionIniPath);
                continue;
            }

            try
            {
                pluginVersion =
                    Common::UtilityImpl::StringUtils::extractValueFromIniFile(pluginVersionIniPath, "PRODUCT_VERSION");
            }
            catch (const std::runtime_error& e)
            {
                LOGWARN("Failed to read PRODUCT_VERSION from " << pluginVersionIniPath << ": " << e.what());
                continue;
            }
            if (pluginVersion.empty())
            {
                LOGWARN("PRODUCT_VERSION not found in " << pluginVersionIniPath);
                continue;
            }

            result += { { "Name", pluginName }, { "installed_version", pluginVersion } };
        }

        return result.dump();
    }
} // namespace

namespace OsquerySDK
{
    TableRows SophosServerTable::Generate(QueryContextInterface& /*request*/)
    {
        TableRows results;
        TableRow r;
        r["endpoint_id"] = "";
        r["customer_id"] = "";
        std::string installPath = Common::ApplicationConfiguration::applicationPathManager().sophosInstall();
        auto fileSystem = Common::FileSystem::fileSystem();

        std::string mcsConfigPath = Common::FileSystem::join(installPath, "base/etc/sophosspl/mcs.config");
        std::string mcsPolicyConfigPath = Common::FileSystem::join(installPath, "base/etc/sophosspl/mcs_policy.config");

        if (fileSystem->isFile(mcsConfigPath))
        {
            std::pair<std::string, std::string> value =
                Common::UtilityImpl::FileUtils::extractValueFromFile(mcsConfigPath, "MCSID");
            r["endpoint_id"] = value.first;
            if (value.first.empty() && !value.second.empty())
            {
                LOGWARN(
                    "Failed to read MCSID configuration from config file: " << mcsConfigPath
                                                                            << " with error: " << value.second);
            }
        }
        else
        {
            LOGWARN(
                "Failed to read MCSID configuration from config file: " << mcsConfigPath
                                                                        << " with error: file doesn't exist");
        }

        if (fileSystem->isFile(mcsPolicyConfigPath))
        {
            std::pair<std::string, std::string> value =
                Common::UtilityImpl::FileUtils::extractValueFromFile(mcsPolicyConfigPath, "customerId");
            r["customer_id"] = value.first;
            if (value.first.empty() && !value.second.empty())
            {
                LOGWARN(
                    "Failed to read customerID configuration from config file: " << mcsPolicyConfigPath
                                                                                 << " with error: " << value.second);
            }
        }
        else
        {
            LOGWARN(
                "Failed to read customerID configuration from config file: " << mcsPolicyConfigPath
                                                                             << " with error: file doesn't exist");
        }

        try
        {
            r["installed_versions"] = getInstalledVersions();
        }
        catch (const nlohmann::json::exception& e)
        {
            LOGWARN("Failed while creating the installed_versions JSON: " << e.what());
            r["installed_versions"] = "[]";
        }

        results.push_back(std::move(r));
        return results;
    }

} // namespace OsquerySDK
