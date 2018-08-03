/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include "PluginProxy.h"

#include <Common/PluginRegistryImpl/PluginInfo.h>

namespace watchdog
{
    namespace watchdogimpl
    {
        using PluginInfoVector = Common::PluginRegistryImpl::PluginInfoVector;
        using ProxyVector = std::vector<watchdog::watchdogimpl::PluginProxy>;

        class Watchdog
        {
        public:
            int run();
            PluginInfoVector read_plugin_configs();
        protected:
            std::string getIPCPath();
        private:
            ProxyVector m_pluginProxies;
        };
    }
}
