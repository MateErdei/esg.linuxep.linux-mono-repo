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
            explicit PluginProxy(Common::PluginRegistryImpl::PluginInfo info);
            ~PluginProxy() noexcept;
            PluginProxy(PluginProxy&&) noexcept;

            /**
             * Move-assignment operator.
             * Need to provide our own implementation, since we need to swap m_running, not copy it.
             * @return
             */
            PluginProxy& operator=(PluginProxy&&) noexcept;

            // Don't allow copying
            PluginProxy(const PluginProxy&) = delete;
            PluginProxy& operator=(const PluginProxy&) = delete;

            /**
             * Stops the plugin if it is running.
             */
            void stop();
            std::chrono::seconds checkForExit();

            /**
             * If plugin is enabled, and is not running, and enough time has passed, start plugin.
             * If plugin is enabled, and is not running, and enough time has not passed, return amount of time to wait.
             * If plugin is disabled, and is running, stop plugin.
             *
             * @return How many seconds to wait before we are ready to start again, 3600 if we are running already
             */
            std::chrono::seconds ensureStateMatchesOptions();

            /**
             * Get the name of the plugin.
             *
             * @return plugin name (either from the JSON, or from the filename if not specified in the JSON info)
             */
            std::string name() const;

            void setEnabled(bool enabled);

            /**
             * Info has been reloaded, so update the information
             * @param info
             */
            void updatePluginInfo(const Common::PluginRegistryImpl::PluginInfo& info);

        private:
            /**
             * Starts the plugin.
             */
            void start();

            /**
             * Get the current status of the plugin.
             * @return
             */
            Common::Process::ProcessStatus status();

            /**
             * Wait for the exit code from the plugin.
             * @return
             */
            int exitCode();

            void swap(PluginProxy& other);

            /**
             * Loaded information about the plugin
             */
            Common::PluginRegistryImpl::PluginInfo m_info;
            /**
             * Execution object for the plugin
             */
            Common::Process::IProcessPtr m_process;
            /**
             * Full path to the executable
             */
            std::string m_exe;
            /**
             * True if the plugin is currently running.
             */
            bool m_running;

            /**
             * When the plugin process last died.
             */
            time_t m_deathTime;

            bool m_enabled;

        };
    } // namespace watchdogimpl
} // namespace watchdog
