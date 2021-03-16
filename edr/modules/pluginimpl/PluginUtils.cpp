/******************************************************************************************************

Copyright 2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Logger.h"
#include "PluginUtils.h"
#include "ApplicationPaths.h"
#include "OsqueryConfigurator.h"
#include "LiveQueryPolicyParser.h"
#include <thirdparty/nlohmann-json/json.hpp>

#include <fstream>
#include <Common/UtilityImpl/StringUtils.h>
#include <Common/UtilityImpl/FileUtils.h>
#include <Common/FileSystem/IFileSystemException.h>

namespace Plugin
{
    bool PluginUtils::isFlagSet(const std::string& flag, const std::string& flagContent)
    {
        bool flagValue = false;

        try
        {
            nlohmann::json j = nlohmann::json::parse(flagContent);

            if (j.find(flag) != j.end())
            {
                if (j[flag] == true)
                {
                    flagValue = true;
                }
            }
        }
        catch (nlohmann::json::parse_error& ex)
        {
            std::stringstream errorMessage;
            errorMessage << "Could not parse json: " << flagContent << " with error: " << ex.what();
            LOGWARN(errorMessage.str());
        }

        return flagValue;
    }

    bool PluginUtils::retrieveGivenFlagFromSettingsFile(const std::string& flag)
    {
        auto fileSystem = Common::FileSystem::fileSystem();
        std::string configPath = Plugin::edrConfigFilePath();

        if (fileSystem->isFile(configPath))
        {
            std::pair<std::string,std::string> value = Common::UtilityImpl::FileUtils::extractValueFromFile(configPath, flag);
            if (value.first.empty() && !value.second.empty())
            {
                LOGWARN("Failed to read " << flag << " configuration from config file due to error: " << value.second);
                throw std::runtime_error(flag + " not set in plugin setting file");
            }

            return (value.first == "1");;
        }
        else
        {
            LOGWARN("Could not find EDR Plugin config file: " << configPath);
        }
        throw std::runtime_error(flag + " not set in plugin setting file");
    }

    bool PluginUtils::isInteger(const std::string& number)
    {
        if (number.find_first_not_of("1234567890") == std::string::npos)
        {
            return true;
        }
        return false;
    }
    void PluginUtils::setGivenFlagFromSettingsFile(const std::string& flag, const bool& flagValue)
    {
        auto fileSystem = Common::FileSystem::fileSystem();
        std::string mode;
        if (flagValue)
        {
            mode = flag + "=1";
        }
        else
        {
            mode = flag + "=0";
        }

        try
        {
            std::string configPath = Plugin::edrConfigFilePath();
            if (fileSystem->isFile(configPath))
            {
                bool modeIsNotSet = true;
                std::vector<std::string> content = fileSystem->readLines(configPath);
                std::stringstream newContent;
                for (const auto &line : content)
                {
                    // if running mode already set replace it
                    if (Common::UtilityImpl::StringUtils::isSubstring(line, flag))
                    {
                        newContent << mode << "\n";
                        modeIsNotSet = false;
                    }
                    else
                        {
                        newContent << line << "\n";
                    }
                }

                // if running mode not already set append to end of content
                if (modeIsNotSet)
                {
                    newContent << mode << "\n";
                }

                fileSystem->writeFileAtomically(configPath, newContent.str(), Plugin::etcDir());
            }
            else
            {
                fileSystem->writeFile(configPath, mode + "\n");
            }
            LOGINFO("Updated plugin conf with new " + flag);
        }
        catch (Common::FileSystem::IFileSystemException& ex)
        {
            LOGWARN("Failed to update plugin conf with error: " << ex.what());
        }
    }

    std::pair<std::string, std::string> PluginUtils::getRunningQueryPackFilePaths()
    {
        auto* ifileSystem = Common::FileSystem::fileSystem();
        std::string targetDirectoryPath = Plugin::osqueryConfigDirectoryPath();
        std::string targetMtrFilePath = Plugin::osqueryMTRConfigFilePath();
        std::string targetXdrFilePath = Plugin::osqueryXDRConfigFilePath();

        try
        {
            if (ifileSystem->isDirectory(targetDirectoryPath))
            {
                std::vector<std::string> paths = ifileSystem->listFiles(targetDirectoryPath);

                if (!paths.empty())
                {
                    for (const auto& filepath : paths)
                    {
                        std::string basename = Common::FileSystem::basename(filepath);
                        if (Common::UtilityImpl::StringUtils::startswith(basename, "sophos-scheduled-query-pack"))
                        {
                            if (Common::UtilityImpl::StringUtils::isSubstring(basename, "mtr"))
                            {
                                targetMtrFilePath = filepath;
                            }
                            else if (!Common::UtilityImpl::StringUtils::isSubstring(basename, "custom"))
                            {
                                targetXdrFilePath = filepath;
                            }
                        }
                    }
                }
            }
        }
        catch (Common::FileSystem::IFileSystemException& e)
        {
            LOGERROR("Cannot check for existing query pack names due to exception: " << e.what() << ". Using default filenames: "
            << Common::FileSystem::basename(targetMtrFilePath) << " & " << Common::FileSystem::basename(targetXdrFilePath));
        }
        return {targetMtrFilePath, targetXdrFilePath};
    }

    bool PluginUtils::nextQueryPacksShouldBeReloaded()
    {
        auto* ifileSystem = Common::FileSystem::fileSystem();
        std::pair<std::string, std::string> runningPaths = PluginUtils::getRunningQueryPackFilePaths();
        try
        {
            bool mtrNextPackChanged = false;
            bool xdrNextPackChanged = false;

            std::string currentMTRFileContents = ifileSystem->readFile(runningPaths.first);
            std::string stagingMTRFileContents = ifileSystem->readFile(Plugin::osqueryNextMTRConfigStagingFilePath());
            if (currentMTRFileContents != stagingMTRFileContents)
            {
                mtrNextPackChanged = true;
            }

            std::string currentXDRFileContents = ifileSystem->readFile(runningPaths.second);
            std::string stagingXDRFileContents = ifileSystem->readFile(Plugin::osqueryNextXDRConfigStagingFilePath());
            if (currentXDRFileContents != stagingXDRFileContents)
            {
                xdrNextPackChanged = true;
            }

            return (mtrNextPackChanged || xdrNextPackChanged);
        }
        catch (Common::FileSystem::IFileSystemException& e)
        {
            LOGERROR("Cannot compare query pack files due to exception: " << e.what() << ". Assuming difference.");
        }

        return true;
    }

    void PluginUtils::setQueryPacksInPlace(const bool& useNextQueryPack)
    {
        std::pair<std::string,std::string> targetFiles = getRunningQueryPackFilePaths();
        auto* ifileSystem = Common::FileSystem::fileSystem();

        try
        {
            if (useNextQueryPack)
            {
                LOGINFO("Overwriting existing scheduled query packs with 'NEXT' query packs");
                ifileSystem->copyFile(Plugin::osqueryNextMTRConfigStagingFilePath(), targetFiles.first);
                ifileSystem->copyFile(Plugin::osqueryNextXDRConfigStagingFilePath(), targetFiles.second);
            }
            else
            {
                LOGINFO("Reverting to existing scheduled query packs");
                ifileSystem->copyFile(Plugin::osqueryMTRConfigStagingFilePath(), targetFiles.first);
                ifileSystem->copyFile(Plugin::osqueryXDRConfigStagingFilePath(), targetFiles.second);
            }
        }
        catch (Common::FileSystem::IFileSystemException& e)
        {
            LOGERROR("Failed to set query packs to the correct version: " << e.what());
        }
    }

    void PluginUtils::updatePluginConfWithFlag(const std::string& flagName, const bool& flagSetting, bool& flagsHaveChanged)
    {
        try
        {
            bool oldMode = Plugin::PluginUtils::retrieveGivenFlagFromSettingsFile(flagName);
            if (oldMode != flagSetting)
            {
                LOGINFO("Updating " << flagName << " flag settings");
                Plugin::PluginUtils::setGivenFlagFromSettingsFile(flagName, flagSetting);
                flagsHaveChanged = true;
            }
        }
        catch (const std::runtime_error& ex)
        {
            LOGINFO("Setting " << flagName << " flag settings");
            Plugin::PluginUtils::setGivenFlagFromSettingsFile(flagName, flagSetting);
            flagsHaveChanged = true;
        }

    }

    bool PluginUtils::handleDisablingAndEnablingScheduledQueryPacks(std::vector<std::string> enabledQueryPacks, bool dataLimitHit)
    {
        bool needsOsqueryRestart = false;

        if (dataLimitHit)
        {
            LOGINFO("Disabling all query packs because the daily data limit has been hit");
            disableAllQueryPacks();
            return true;
        }
        else
        {
            // custom query pack is not listed in policy, and should always be enabled
            // if it exists and the data limit has not been hit
            needsOsqueryRestart = needsOsqueryRestart || enableQueryPack(osqueryCustomConfigFilePath());
        }

        std::vector<std::string> queryPackKeys;
        for (auto const& queryPack : getLiveQueryPackIdToConfigPath())
        {
            try
            {
                bool currentlyEnabled = isQueryPackEnabled(queryPack.second);
                bool shouldBeEnabled =
                        std::find(std::begin(enabledQueryPacks), std::end(enabledQueryPacks), queryPack.first) !=
                        std::end(enabledQueryPacks);

                if (currentlyEnabled != shouldBeEnabled) {
                    needsOsqueryRestart = true;
                    if (shouldBeEnabled) {
                        enableQueryPack(queryPack.second);
                        LOGDEBUG("Enabling :" << queryPack.second);
                    } else {
                        disableQueryPack(queryPack.second);
                        LOGDEBUG("Disabling :" << queryPack.second);
                    }
                }
            }
            catch (const MissingQueryPackException& exception)
            {
                LOGERROR(exception.what());
            }
        }
        return needsOsqueryRestart;
    }


    bool PluginUtils::isQueryPackEnabled(Path queryPackPathWhenEnabled)
    {
        Path queryPackPathWhenDisabled = queryPackPathWhenEnabled + ".DISABLED";
        if (Common::FileSystem::fileSystem()->isFile(queryPackPathWhenEnabled))
        {
            return true;
        }
        else if (Common::FileSystem::fileSystem()->isFile(queryPackPathWhenDisabled))
        {
            return false;
        }
        throw MissingQueryPackException("Expected at least one of: " + queryPackPathWhenEnabled + " or " + queryPackPathWhenDisabled + " to exist");
    }

    bool PluginUtils::enableQueryPack(const std::string& queryPackFilePath)
    {
        auto fs = Common::FileSystem::fileSystem();
        std::string disabledPath = queryPackFilePath + ".DISABLED";
        if (fs->exists(disabledPath))
        {
            try
            {
                fs->moveFile(disabledPath, queryPackFilePath);
                LOGDEBUG("Enabled query pack conf file: " << queryPackFilePath);
                return true;
            }
            catch (std::exception& ex)
            {
                LOGERROR("Failed to enabled query pack conf file: " << ex.what());
            }
        }
        return false;
    }

    void PluginUtils::disableQueryPack(const std::string& queryPackFilePath)
    {
        auto fs = Common::FileSystem::fileSystem();
        if (fs->exists(queryPackFilePath))
        {
            try
            {
                fs->moveFile(queryPackFilePath, queryPackFilePath + ".DISABLED");
                LOGDEBUG("Disabled query pack conf file");
            }
            catch (std::exception& ex)
            {
                LOGERROR("Failed to disable query pack conf file: " << ex.what());
            }
        }
    }

    void PluginUtils::disableAllQueryPacks()
    {
        for (auto const& queryPack : getLiveQueryPackIdToConfigPath())
        {
            disableQueryPack(queryPack.second);
        }
        disableQueryPack(osqueryCustomConfigFilePath());
    }

    void PluginUtils::enableCustomQueries(std::optional<std::string> customQueries, bool &osqueryRestartNeeded, bool dataLimitHit)
    {
        try
        {
            auto fs = Common::FileSystem::fileSystem();
            if (fs->exists(osqueryCustomConfigFilePath()))
            {
                fs->removeFileOrDirectory(osqueryCustomConfigFilePath());
            }
            if (fs->exists(osqueryCustomConfigFilePath()+".DISABLED"))
            {
                fs->removeFileOrDirectory(osqueryCustomConfigFilePath()+".DISABLED");
            }
            if (customQueries.has_value())
            {
                if (dataLimitHit)
                {
                    fs->writeFile(osqueryCustomConfigFilePath()+".DISABLED", customQueries.value());
                }
                else
                {
                    fs->writeFile(osqueryCustomConfigFilePath(), customQueries.value());
                }
            }
            osqueryRestartNeeded = true;

        }
        catch (Common::FileSystem::IFileSystemException &e)
        {
            LOGWARN("Filesystem Exception While removing/writing custom query file: " << e.what());
        }
    }

    bool PluginUtils::haveCustomQueriesChanged(const std::optional<std::string>& customQueries)
    {
        auto fs = Common::FileSystem::fileSystem();
        std::optional<std::string> oldCustomQueries;

        if (fs->exists(osqueryCustomConfigFilePath()))
        {
            oldCustomQueries = fs->readFile(osqueryCustomConfigFilePath());
        }

        return customQueries != oldCustomQueries;
    }

}
