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
        * Reads content of flagspolicy to see is XDR is enabled or not
        * @param flagContent, content of flags policy
        * @return true if XDR is enabled false if it is disabled
        */
        static bool isRunningModeXDR(const std::string &flagContent);
        static bool retrieveRunningModeFlagFromSettingsFile();
        static void setRunningModeFlagFromSettingsFile(const bool &isXDR);

    private:
        inline static const std::string m_mode_identifier = "running_mode";

   };
}


