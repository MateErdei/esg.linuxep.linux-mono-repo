/******************************************************************************************************

Copyright 2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include <Common/FileSystem/IFileSystem.h>
#include <Common/FileSystem/IFilePermissions.h>
#include <Common/FileSystem/IFileSystemException.h>
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
        * Reads content of flags policy to see if flag is enabled or not
        * @param flagContent, content of flags policy
        * @return true if flag is enabled, false otherwise (default)
        */
        static bool isFlagSet(const std::string& flag, const std::string& flagContent);

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
        * Compares the running query packs and the next staging query packs and returns whether there is a difference
        * @returns Whether the next query packs need to be reloaded
        */
        static bool nextQueryPacksShouldBeReloaded();

        /**
        * Overwrites the content of scheduled query packs with the correct contents based on the flag
        * @param useNextQueryPack
        */
        static void setQueryPacksInPlace(const bool& useNextQueryPack);
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



        inline static const std::string MODE_IDENTIFIER = "running_mode";
        inline static const std::string NETWORK_TABLES_AVAILABLE = "network_tables";
        inline static const std::string QUERY_PACK_NEXT_SETTING = "scheduled_queries_next";
        inline static const std::string XDR_FLAG = "xdr.enabled";
        inline static const std::string QUERY_PACK_NEXT = "scheduled_queries.next";
        inline static const std::string NETWORK_TABLES_FLAG = "livequery.network-tables.available";
   };
}


