/******************************************************************************************************

Copyright 2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include <Common/FileSystem/IFileSystem.h>
#include <Common/FileSystem/IFilePermissions.h>
#include <Common/FileSystem/IFileSystemException.h>
#include <optional>

namespace Plugin
{

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
        * @param filesystem
        */
        static std::pair<std::string, std::string> getRunningQueryPackFilePaths(Common::FileSystem::IFileSystem* filesystem);

        /**
        * Overwrites the content of scheduled query packs with the correct contents based on the flag
        * @param useNextQueryPack
        */
        static void setQueryPacksInPlace(const bool& useNextQueryPack);

        /**
        * Update plugin.conf file for a given flag setting and set whether the flag has changed
        * @param flagName
        * @param flagSetting
        * @param flagsHaveChanged
        */
        static void updatePluginConfWithFlag(const std::string& flagName, const bool& flagSetting, bool& flagsHaveChanged);

        inline static const std::string MODE_IDENTIFIER = "running_mode";
        inline static const std::string NETWORK_TABLES_AVAILABLE = "network_tables";
        inline static const std::string XDR_FLAG = "xdr.enabled";
        inline static const std::string QUERY_PACK_NEXT = "scheduled_queries.next";
        inline static const std::string NETWORK_TABLES_FLAG = "livequery.network-tables.available";
   };
}


