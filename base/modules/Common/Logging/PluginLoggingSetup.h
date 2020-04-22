/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <string>

namespace Common
{
    namespace Logging
    {
        class PluginLoggingSetup
        {
        public:
            explicit PluginLoggingSetup(const std::string& pluginName, const std::string & overrideFileName="");
            ~PluginLoggingSetup();
        private:
            static void setupFileLogging(const std::string& pluginNameDir, const std::string & pluginFileName);
        };
    } // namespace Logging
} // namespace Common
