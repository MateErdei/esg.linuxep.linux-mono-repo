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
            ~PluginProxy() noexcept;
            PluginProxy(PluginProxy&&) = default;

            // Don't allow copying
            PluginProxy(const PluginProxy&) = delete;
            PluginProxy& operator=(const PluginProxy&) = delete;

            void start();
            void stop();
            void checkForExit();

            /**
             *
             * @return How many seconds to wait before we are ready to start again, 3600 if we are running already
             */
            time_t startIfRequired();

        private:

            Common::Process::ProcessStatus status();
            int exitCode();

            Common::PluginRegistryImpl::PluginInfo m_info;
            Common::Process::IProcessPtr m_process;
            std::string m_exe;
            bool m_running;
            time_t m_deathTime;
        };
    }
}


#endif //WATCHDOG_WATCHDOGIMPL_PLUGINPROXY_H
