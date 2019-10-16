/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "ProcessProxy.h"

#include <Common/PluginRegistryImpl/PluginInfo.h>
#include <Common/ProcessMonitoring/IProcessMonitor.h>
#include <Common/ZMQWrapperApi/IContext.h>
#include <Common/ZMQWrapperApi/IContextSharedPtr.h>
#include <Common/ZeroMQWrapper/ISocketReplier.h>
#include <Common/ZeroMQWrapper/ISocketReplierPtr.h>

#include <list>
#include <mutex>

namespace Common
{
    namespace ProcessMonitoringImpl
    {
        using ProxyList = std::list<Common::ProcessMonitoring::IProcessProxyPtr>;
        using SocketHandleFunctionPair = std::pair<Common::ZeroMQWrapper::ISocketReplier*, std::function<void(void)>>;
        using SocketHandleFunctionList = std::list<SocketHandleFunctionPair>;

        class ProcessMonitor : public Common::ProcessMonitoring::IProcessMonitor
        {
        public:
            explicit ProcessMonitor();
            explicit ProcessMonitor(Common::ZMQWrapperApi::IContextSharedPtr context);
            ~ProcessMonitor() override;

            void addProcessToMonitor(Common::ProcessMonitoring::IProcessProxyPtr processProxyPtr) override;
            void addReplierSocketAndHandleToPoll(
                Common::ZeroMQWrapper::ISocketReplier* socketReplier,
                std::function<void(void)> socketHandleFunction) override;

            int run() override;

        private:
            std::mutex m_processProxiesMutex;
            ProxyList m_processProxies;
            SocketHandleFunctionList m_socketHandleFunctionList;

        protected:
            std::vector<std::string> getListOfPluginNames();

            bool removePluginByName( const std::string & pluginName );

            void applyToProcessProxy(const std::string & processProxyName, std::function<void(Common::ProcessMonitoring::IProcessProxy&)> functorToApply);

            Common::ZMQWrapperApi::IContextSharedPtr m_context;

        };
    } // namespace ProcessMonitoringImpl
} // namespace Common
