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
        * Read a json file to get the proxy info
        * @param flagContent, path for where the file containing the proxy info is located
        * @param fileSystem, to check if file exists
        * @return returns the address and password of proxy in a tuple
        */
        static bool isRunningModeXDR(const std::string &flagContent);
        static bool retrieveRunningModeFlagFromSettingsFile();
        static void setRunningModeFlagFromSettingsFile(const bool &isXDR);

            private:
                inline static const std::string m_mode_identifier = "running_mode";

   };
}


