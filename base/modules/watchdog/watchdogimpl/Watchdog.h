/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once


#include <Common/PluginRegistryImpl/PluginInfo.h>

namespace watchdog
{
    namespace watchdogimpl
    {
        using PluginInfoVector = Common::PluginRegistryImpl::PluginInfoVector;

        class Watchdog
        {
        public:
            int run();
            PluginInfoVector read_plugin_configs();
        };
    }
}
