/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/PluginRegistryImpl/PluginInfo.h>
#include <Common/Process/IProcess.h>

#include <chrono>

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
            std::chrono::seconds startIfRequired();

            /**
             * Get the name of the plugin.
             *
             * @return plugin name (either from the JSON, or from the filename if not specified in the JSON info)
             */
            std::string name() const;

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
