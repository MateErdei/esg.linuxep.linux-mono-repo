/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <string>
#include "LoggerConfig.h"

namespace Common::Logging
{
    class PluginLoggingSetupEx
    {
    public:
        explicit PluginLoggingSetupEx(const std::string& pluginName, const std::string& overrideFileName = "", const std::string& instanceName = "");
        ~PluginLoggingSetupEx();

    private:
        static void setupFileLogging(const std::string& pluginNameDir, const std::string& pluginFileName, const std::string& instanceName);
    };
} // namespace Common::Logging
