// Copyright 2020-2023 Sophos Limited. All rights reserved.
#pragma once

#include "Common/FileSystem/IFileSystem.h"
#include "Common/FileSystem/IFilePermissions.h"
#include "Common/FileSystem/IFileSystemException.h"
#include <optional>

namespace Plugin
{
    class MissingQueryPackException : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

    class PluginUtils
            {
    public:
        /**
        * Reads content of plugin.conf to see if a given field is enabled or not
        * @return true if the given field is enabled false if it is disabled
        * @param flag
        */
        static bool retrieveGivenFlagFromSettingsFile(const std::string& flag);

        /**
        * checks if a string is an integer
        * @return true if the given field is a number
        * @param number, string to check
        */
        static bool isInteger(const std::string& number);

        /**
        * sets value of a given field in plugin.conf
        * @param flag
        * @param value
        */
        static void setGivenFlagFromSettingsFile(const std::string& flag, const bool& flagValue);

        /**
        * Reads filenames from osquery.conf.d and returns the filepaths for both the xdr and mtr query packs
        */
        static std::pair<std::string, std::string> getRunningQueryPackFilePaths();

        /**
        * Removes all files in jrl folder
        */
        static void clearAllJRLMarkers();
        /**
        * Update plugin.conf file for a given flag setting and set whether the flag has changed
        * @param flagName
        * @param flagSetting
        * @param flagsHaveChanged
        */
        static void updatePluginConfWithFlag(const std::string& flagName, const bool& flagSetting, bool& flagsHaveChanged);

        /**
        * Given a query pack path returns whether it is enabled or disabled
        * @returns true if query pack is enabled and false if disabled. Throws if query pack missing
        * @param queryPackPathWhenEnabled
        */
        static bool isQueryPackEnabled(Path queryPackPathWhenEnabled);

        /**
        * Enables or disables the query packs as per policy and whether we have hit the data limit
        * @returns true if an osquery restart is required and false otherwise
        * @param enabledQueryPacks
        * @param dataLimitHit
        */
        static bool handleDisablingAndEnablingScheduledQueryPacks(std::vector<std::string> enabledQueryPacks, bool dataLimitHit);

        /**
         * Replaces the old custom query config with one that runs the given queries and updates osqueryRestartNeeded if changed
         * @param customQueries
         * @param osqueryRestartNeeded
         */
        static void enableCustomQueries(std::optional<std::string> customQueries, bool& osqueryRestartNeeded, bool dataLimitHit);

        /**
         * Checks whether new custom queries are different to current custom queries on disk
         * @param customQueries
         * @return true if custom queries have changed and false otherwise
         */
        static bool haveCustomQueriesChanged(const std::optional<std::string>& customQueries);

        static bool enableQueryPack(const std::string& queryPackFilePath);
        static void disableQueryPack(const std::string& queryPackFilePath);
        static void disableAllQueryPacks();

        static unsigned int getEventsMaxFromConfig();

        static std::vector<std::string>  getWatchdogFlagsFromConfig();

        /*
         * Get the flag --pack_refresh_interval from the config (pack_refresh_interval)
         * This is primarily used for testing.
         */
        static std::string  getDiscoveryQueryFlagFromConfig();

        inline static const std::string MODE_IDENTIFIER = "running_mode";
        inline static const std::string NETWORK_TABLES_AVAILABLE = "network_tables";
        inline static const std::string XDR_FLAG = "xdr.enabled";
        inline static const std::string NETWORK_TABLES_FLAG = "livequery.network-tables.available";

        inline static const unsigned long MAXIMUM_EVENTED_RECORDS_ALLOWED = 100000;
        inline static const unsigned long DEFAULT_WATCHDOG_MEMORY_LIMIT_MB = 250;
        inline static const unsigned long DEFAULT_WATCHDOG_CPU_PERCENTAGE = 30;
        inline static const unsigned long DEFAULT_WATCHDOG_DELAY_SECONDS = 60;
        inline static const unsigned long DEFAULT_WATCHDOG_LATENCY_SECONDS = 0;

        inline static const std::string DISCOVERY_QUERY_INTERVAL = "pack_refresh_interval";
        static constexpr unsigned long DEFAULT_DISCOVERY_QUERY_INTERVAL = 3600;
    private:
        static std::string getIntegerFlagFromConfig(const std::string& flag, int defaultValue);
   };
}


