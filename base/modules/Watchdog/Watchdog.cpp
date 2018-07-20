/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#include "Watchdog.h"
#include "Logger.h"

#include "Common/PluginRegistryImpl/PluginInfo.h"

namespace
{
    // unused
}


namespace Watchdog
{
    void read_plugin_configs()
    {
        std::vector<Common::PluginRegistryImpl::PluginInfo> plugins = Common::PluginRegistryImpl::PluginInfo::loadFromPluginRegistry();

        for (auto &plugin: plugins)
        {
            LOGINFO("plugin " << plugin.getPluginName() << " is at " << plugin.getExecutableFullPath());
        }

    }

    int main_entry(int argc, char **argv)
    {
        static_cast<void>(argv); // unused
        if(argc > 1)
        {
            LOGERROR("Error, invalid command line arguments. Usage: Watchdog");
            return -1;
        }

        read_plugin_configs();

        return 0;
    }
}


