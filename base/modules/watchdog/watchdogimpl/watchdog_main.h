/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#ifndef WATCHDOG_WATCHDOGIMPL_WATCHDOG_MAIN_H
#define WATCHDOG_WATCHDOGIMPL_WATCHDOG_MAIN_H

#include <Common/PluginRegistryImpl/PluginInfo.h>

namespace watchdog
{
    namespace watchdogimpl
    {
        using PluginInfoVector = Common::PluginRegistryImpl::PluginInfoVector;

        class watchdog_main
        {
        public:
            static int main(int argc, char* argv[]);
        protected:
            int run();
            PluginInfoVector read_plugin_configs();
        };
    }
}

#endif //WATCHDOG_WATCHDOGIMPL_WATCHDOG_MAIN_H
