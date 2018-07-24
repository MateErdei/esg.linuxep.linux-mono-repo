/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "watchdog_main.h"

#include "Logger.h"

#include <Common/PluginRegistryImpl/PluginInfo.h>

/**
 * Static method called from watchdog executable
 * @param argc
 * @param argv
 * @return
 */
int watchdog::watchdogimpl::watchdog_main::main(int argc, char **argv)
{
    static_cast<void>(argv); // unused
    if(argc > 1)
    {
        LOGERROR("Error, invalid command line arguments. Usage: watchdog");
        return -1;
    }

    watchdog_main m;
    return m.run();
}

int watchdog::watchdogimpl::watchdog_main::run()
{
    read_plugin_configs();

    return 0;
}

void watchdog::watchdogimpl::watchdog_main::read_plugin_configs()
{
    std::vector<Common::PluginRegistryImpl::PluginInfo> plugins = Common::PluginRegistryImpl::PluginInfo::loadFromPluginRegistry();

    for (auto &plugin: plugins)
    {
        LOGINFO("plugin " << plugin.getPluginName() << " is at " << plugin.getExecutableFullPath());
    }
}
