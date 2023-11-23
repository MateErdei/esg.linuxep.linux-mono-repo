// Copyright 2019-2023 Sophos Limited. All rights reserved.

#pragma once

#include "ProcessProxy.h"

#include "Common/ProcessMonitoring/IProcessMonitor.h"
#include "Common/Threads/NotifyPipe.h"
#include "Common/ZMQWrapperApi/IContext.h"
#include "Common/ZMQWrapperApi/IContextSharedPtr.h"
#include "Common/ZeroMQWrapper/ISocketReplier.h"
#include "Common/ZeroMQWrapper/ISocketReplierPtr.h"

#include <list>
#include <mutex>

namespace Common::ProcessMonitoringImpl
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
        void stopProcesses();

    private:
        std::mutex m_processProxiesMutex;
        ProxyList m_processProxies;
        SocketHandleFunctionList m_socketHandleFunctionList;
        Common::ProcessMonitoring::IProcessProxy::NotifyPipeSharedPtr m_processTerminationCallbackPipe;


    protected:
        std::vector<std::string> getListOfPluginNames();

        bool removePluginByName(const std::string& pluginName);

        void applyToProcessProxy(
            const std::string& processProxyName,
            std::function<void(Common::ProcessMonitoring::IProcessProxy&)> functorToApply);

        Common::ZMQWrapperApi::IContextSharedPtr m_context;
    };
} // namespace Common::ProcessMonitoringImpl
