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
            explicit PluginLoggingSetup(const std::string& pluginName);
            ~PluginLoggingSetup();
            static void setupFileLogging(const std::string& pluginName);
        };
    }
}
