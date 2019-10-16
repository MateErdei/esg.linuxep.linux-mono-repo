/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/PluginRegistryImpl/PluginInfo.h>
#include <Common/Process/IProcess.h>
#include <Common/ProcessMonitoringImpl/ProcessProxy.h>

#include <chrono>

namespace watchdog
{
    namespace watchdogimpl
    {
        class PluginProxy : public Common::ProcessMonitoringImpl::ProcessProxy
        {
        public:
            explicit PluginProxy(Common::PluginRegistryImpl::PluginInfo info);

            std::pair<std::chrono::seconds, Common::Process::ProcessStatus> checkForExit() override;

            /**
             * Get the name of the plugin.
             *
             * @return plugin name (either from the JSON, or from the filename if not specified in the JSON info)
             */
            std::string name() const override ;

            /**
             * Info has been reloaded, so update the information
             * @param info
             */
            void updatePluginInfo(const Common::PluginRegistryImpl::PluginInfo& info);

        private:
            /**
             * Returns a reference to the plugin info used in the plugin proxy
             * @return
             */
            Common::PluginRegistryImpl::PluginInfo& getPluginInfo() const;
        };
    } // namespace watchdogimpl
} // namespace watchdog
