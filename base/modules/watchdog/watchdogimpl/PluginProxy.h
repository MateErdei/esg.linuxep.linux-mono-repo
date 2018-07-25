/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#ifndef WATCHDOG_WATCHDOGIMPL_PLUGINPROXY_H
#define WATCHDOG_WATCHDOGIMPL_PLUGINPROXY_H

#include <Common/PluginRegistryImpl/PluginInfo.h>
#include <Common/Process/IProcess.h>

namespace watchdog
{
    namespace watchdogimpl
    {
        class PluginProxy
        {
        public:
            explicit PluginProxy(
                    Common::PluginRegistryImpl::PluginInfo info
                    );
            void start();
            void stop();
            Common::Process::ProcessStatus status();
            int exitCode();
        private:
            Common::PluginRegistryImpl::PluginInfo m_info;
            Common::Process::IProcessPtr m_process;
        };
    }
}


#endif //WATCHDOG_WATCHDOGIMPL_PLUGINPROXY_H
