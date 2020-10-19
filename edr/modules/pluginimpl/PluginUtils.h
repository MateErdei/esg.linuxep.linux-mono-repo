/******************************************************************************************************

Copyright 2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include <Common/FileSystem/IFileSystem.h>
#include <optional>

namespace Plugin
{

    class PluginUtils
            {
    public:
        /**
        * Reads content of flags policy to see if XDR is enabled or not
        * @param flagContent, content of flags policy
        * @return true if XDR is enabled, false if it is disabled
        */
        static bool isRunningModeXDR(const std::string& flagContent);

        /**
        * Reads content of flags policy to see if network tables are enabled or not
        * @param flagContent, content of flags policy
        * @return true if network tables are enabled, false if it is disabled
        */
        static bool areNetworkTablesAvailable(const std::string& flagContent);

        /**
        * Reads content of plugin.conf to see if a given field is enabled or not
        * @return true if the given field is enabled false if it is disabled
        * @param flag
        */
        static bool retrieveGivenFlagFromSettingsFile(const std::string& flag);

        /**
        * sets value of a given field in plugin.conf
        * @param flag
        * @param value
        */
        static void setGivenFlagFromSettingsFile(const std::string& flag, const bool& isXDR);

        inline static const std::string MODE_IDENTIFIER = "running_mode";
        inline static const std::string NETWORK_TABLES_AVAILABLE = "network_tables";

    private:
        inline static const std::string XDR_FLAG = "xdr.enabled";
        inline static const std::string NETWORK_TABLES_FLAG = "livequery.network-tables.available";

   };
}


