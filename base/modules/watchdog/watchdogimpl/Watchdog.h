// Copyright 2018-2023 Sophos Limited. All rights reserved.

#pragma once

#include "PluginProxy.h"
#include "WatchdogServiceLine.h"

#include "Common/PluginRegistryImpl/PluginInfo.h"
#include "Common/ProcessMonitoringImpl/ProcessMonitor.h"
#include "Common/ZMQWrapperApi/IContext.h"
#include "Common/ZMQWrapperApi/IContextSharedPtr.h"
#include "Common/ZeroMQWrapper/ISocketReplier.h"
#include "Common/ZeroMQWrapper/ISocketReplierPtr.h"

#include <list>

namespace watchdog::watchdogimpl
{
    using PluginInfoVector = Common::PluginRegistryImpl::PluginInfoVector;
    using ProxyList = std::list<watchdog::watchdogimpl::PluginProxy>;

    class Watchdog : public Common::ProcessMonitoringImpl::ProcessMonitor
    {
    public:
        explicit Watchdog();
        explicit Watchdog(Common::ZMQWrapperApi::IContextSharedPtr context);
        ~Watchdog() override;
        int initialiseAndRun();
        PluginInfoVector readPluginConfigs();
        std::vector<std::string> getListOfPluginNames();

    protected:
        std::string getIPCPath();
        void setupSocket();
        void handleSocketRequest();
        void writeExecutableUserAndGroupToActualUserGroupIdConfig();
        std::string handleCommand(Common::ZeroMQWrapper::IReadable::data_t input);
        void reconfigureUserAndGroupIds();

        std::string disablePlugin(const std::string& pluginName);
        std::string enablePlugin(const std::string& pluginName);
        std::string checkPluginIsRunning(const std::string& pluginName);
        /**
         * Stop the plugin if it is running, and remove it from m_pluginProxies.
         * @param pluginName
         * @return "OK" or an error
         */
        std::string removePlugin(const std::string& pluginName);

    private:
        Common::ZeroMQWrapper::ISocketReplierPtr m_socket;
        WatchdogServiceLine m_watchdogservice;
    };
} // namespace watchdog::watchdogimpl
