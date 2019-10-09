/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "PluginProxy.h"
#include "WatchdogServiceLine.h"

#include <Common/PluginRegistryImpl/PluginInfo.h>
#include <Common/ProcessMonitoringImpl/ProcessMonitor.h>
#include <Common/ZMQWrapperApi/IContext.h>
#include <Common/ZMQWrapperApi/IContextSharedPtr.h>
#include <Common/ZeroMQWrapper/ISocketReplier.h>
#include <Common/ZeroMQWrapper/ISocketReplierPtr.h>

#include <list>

namespace watchdog
{
    namespace watchdogimpl
    {
        using PluginInfoVector = Common::PluginRegistryImpl::PluginInfoVector;
        using ProxyList = std::list<watchdog::watchdogimpl::PluginProxy>;

        static const std::string watchdogReturnsOk = "OK";                  // NOLINT
        static const std::string watdhdogReturnsNotRunning = "Not Running"; // NOLINT

        class Watchdog : public Common::ProcessMonitoringImpl::ProcessMonitor
        {
        public:
            explicit Watchdog();
            explicit Watchdog(Common::ZMQWrapperApi::IContextSharedPtr context);
            ~Watchdog();
            int initialiseAndRun();
            PluginInfoVector readPluginConfigs();
            const std::vector<std::string> getListOfPluginNames();

        protected:
            std::string getIPCPath();
            void setupSocket();
            void handleSocketRequest();
            std::string handleCommand(Common::ZeroMQWrapper::IReadable::data_t input);

            std::string disablePlugin(const std::string& pluginName);
            std::string enablePlugin(const std::string& pluginName);
            std::string checkPluginIsRunning(const std::string& pluginName);
            /**
             * Stop the plugin if it is running, and remove it from m_pluginProxies.
             * @param pluginName
             * @return "OK" or an error
             */
            std::string removePlugin(const std::string& pluginName);

            /**
             * Find a plugin by name from m_pluginProxies, returning nullptr if no plugin found,
             * @param pluginName
             * @return BORROWED pointer to plugin, or nullptr.
             */
            PluginProxy* findPlugin(const std::string& pluginName);

        private:
            Common::ZeroMQWrapper::ISocketReplierPtr m_socket;
            WatchdogServiceLine m_watchdogservice;
        };
    } // namespace watchdogimpl
} // namespace watchdog
