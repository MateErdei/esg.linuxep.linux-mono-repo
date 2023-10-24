// Copyright 2020-2023 Sophos Limited. All rights reserved.

#include "EdrCommon/ApplicationPaths.h"
#include "pluginimpl/LiveQueryPolicyParser.h"
#include "pluginimpl/Logger.h"
#include "pluginimpl/OsqueryConfigurator.h"
#include "pluginimpl/PluginUtils.h"

#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "Common/FileSystem/IFileSystemException.h"
#include "Common/UtilityImpl/FileUtils.h"
#include "Common/UtilityImpl/StringUtils.h"

#include <fstream>
#include <json.hpp>

namespace Plugin
{
    bool PluginUtils::retrieveGivenFlagFromSettingsFile(const std::string& flag)
    {
        auto fileSystem = Common::FileSystem::fileSystem();
        std::string configPath = Plugin::edrConfigFilePath();

        if (fileSystem->isFile(configPath))
        {
            bool flagPresent = true;
            try
            {
                std::string content = fileSystem->readFile(configPath);
                flagPresent = Common::UtilityImpl::StringUtils::isSubstring(content, flag);
            }
            catch (Common::FileSystem::IFileSystemException& ex)
            {
                LOGWARN("Failed to read " << flag << " configuration from config file due to error: " << ex.what());
            }

            if (flagPresent)
            {
                std::pair<std::string,std::string> value = Common::UtilityImpl::FileUtils::extractValueFromFile(configPath, flag);
                if (value.first.empty() && !value.second.empty())
                {
                    LOGWARN("Failed to read " << flag << " configuration from config file due to error: " << value.second);
                    throw std::runtime_error(flag + " not set in plugin setting file");
                }
                return (value.first == "1");;
            }
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
                LOGINFO("Updating " << flagName << " flag settings to: " << flagSetting);
                Plugin::PluginUtils::setGivenFlagFromSettingsFile(flagName, flagSetting);
                flagsHaveChanged = true;
            }
        }
        catch (const std::runtime_error& ex)
        {
            LOGINFO("Setting " << flagName << " flag settings to: " << flagSetting);
            Plugin::PluginUtils::setGivenFlagFromSettingsFile(flagName, flagSetting);
            flagsHaveChanged = true;
        }

    }
    void PluginUtils::clearAllJRLMarkers()
    {
        std::string installPath = Common::ApplicationConfiguration::applicationPathManager().sophosInstall();
        std::string jrlFolderPath = Common::FileSystem::join(installPath, "plugins/edr/var/jrl");
        try
        {
            Common::FileSystem::fileSystem()->removeFilesInDirectory(jrlFolderPath);
        }
        catch (const Common::FileSystem::IFileSystemException& ex)
        {
            LOGWARN("Unable to clear jrl files in directory " << jrlFolderPath << " with error: " << ex.what());
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
            needsOsqueryRestart = enableQueryPack(osqueryCustomConfigFilePath()) || needsOsqueryRestart;
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
                else
                {
                    LOGDEBUG("No action needed for: " << queryPack.second);
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
        {
            if (Common::FileSystem::fileSystem()->isFile(queryPackPathWhenEnabled))
            {
                LOGDEBUG(queryPackPathWhenEnabled << " is currently enabled");
                return true;
            }
            else if (Common::FileSystem::fileSystem()->isFile(queryPackPathWhenDisabled))
                LOGDEBUG( queryPackPathWhenEnabled << " is currently disabled");
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
        else if (fs->exists(osqueryCustomConfigFilePath()+".DISABLED"))
        {
            oldCustomQueries = fs->readFile(osqueryCustomConfigFilePath()+".DISABLED");
        }

        return customQueries != oldCustomQueries;
    }

    unsigned int PluginUtils::getEventsMaxFromConfig()
    {
        unsigned int eventsMaxValue = MAXIMUM_EVENTED_RECORDS_ALLOWED;
        try
        {
            std::pair<std::string,std::string> eventsMax = Common::UtilityImpl::FileUtils::extractValueFromFile(Plugin::edrConfigFilePath(), "events_max");
            if (!eventsMax.first.empty())
            {
                if (PluginUtils::isInteger(eventsMax.first))
                {
                    LOGINFO("Setting events_max to " << eventsMax.first << " as per value in " << Plugin::edrConfigFilePath());
                    eventsMaxValue = stoul(eventsMax.first);
                }
                else
                {
                    LOGWARN("events_max value in '" << Plugin::edrConfigFilePath() << "' not an integer, so using default of " << eventsMaxValue);
                }
            }
            else
            {
                if (Common::UtilityImpl::StringUtils::startswith(eventsMax.second, "No such node"))
                {
                    LOGDEBUG("No events_max value specified in " << Plugin::edrConfigFilePath() << " so using default of " << eventsMaxValue);
                }
                else
                {
                    LOGWARN("Failed to retrieve events_max value from " << Plugin::edrConfigFilePath() << " with error: " << eventsMax.second);
                }
            }
        }
        catch (const std::exception& exception)
        {
            LOGWARN("Failed to retrieve events_max value from " << Plugin::edrConfigFilePath() << " with exception: " << exception.what());
        }

        return eventsMaxValue;
    }

    std::string PluginUtils::getIntegerFlagFromConfig(const std::string& flag, int defaultValue)
    {
        try
        {
            std::string flagValue = Common::UtilityImpl::StringUtils::extractValueFromConfigFile(Plugin::edrConfigFilePath(), flag);

            if (!flagValue.empty())
            {
                if (PluginUtils::isInteger(flagValue))
                {
                    LOGINFO("Setting " << flag << " to " << flagValue << " as per value in " << Plugin::edrConfigFilePath());
                    std::stringstream value;
                    value << "--" << flag << "=" << Common::UtilityImpl::StringUtils::stringToULong(flagValue);
                    return value.str();
                }
                else
                {
                    LOGWARN(flag << " value in '" << Plugin::edrConfigFilePath() << "' not an integer");
                }
            }
            else
            {
                LOGDEBUG("No " << flag << " value specified in " << Plugin::edrConfigFilePath() << " so using default of " << defaultValue);
            }

        }
        catch (const std::exception& exception)
        {
            LOGWARN("Failed to retrieve " << flag << " value from " << Plugin::edrConfigFilePath() << " with exception: " << exception.what());
        }
        std::stringstream value;
        value << "--" << flag << "=" << defaultValue;
        return value.str();;
    }

    std::vector<std::string> PluginUtils::getWatchdogFlagsFromConfig()
    {
        std::vector<std::string> list = {};
        std::vector<std::pair<std::string,int>> flags = {
            {"watchdog_memory_limit",DEFAULT_WATCHDOG_MEMORY_LIMIT_MB}
            ,{"watchdog_utilization_limit",DEFAULT_WATCHDOG_CPU_PERCENTAGE}
            ,{"watchdog_latency_limit",DEFAULT_WATCHDOG_LATENCY_SECONDS},
            {"watchdog_delay",DEFAULT_WATCHDOG_DELAY_SECONDS}};

        for (auto const& flag :flags)
        {
            list.emplace_back(getIntegerFlagFromConfig(flag.first, flag.second));
        }

        return list;
    }

    std::string PluginUtils::getDiscoveryQueryFlagFromConfig()
    {
        return getIntegerFlagFromConfig(DISCOVERY_QUERY_INTERVAL, DEFAULT_DISCOVERY_QUERY_INTERVAL);
    }
}
