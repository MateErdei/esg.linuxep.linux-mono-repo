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
        * @return true if XDR is enabled false if it is disabled
        */

        static bool isRunningModeXDR(const std::string &flagContent);
        /**
        * Reads content of plugin.conf to see if XDR is enabled or not
        * @return true if XDR is enabled false if it is disabled
        */
        static bool retrieveRunningModeFlagFromSettingsFile();

        /**
        * sets value of running mode in plugin.conf
        * @param isXDR, defines whether or not the running mode should be XDR
        */
        static void setRunningModeFlagFromSettingsFile(const bool &isXDR);

    private:
        inline static const std::string m_mode_identifier = "running_mode";

   };
}


