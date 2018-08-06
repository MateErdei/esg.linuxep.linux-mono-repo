/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include "PluginProxy.h"

#include <Common/PluginRegistryImpl/PluginInfo.h>
#include <Common/ZeroMQWrapper/IContext.h>
#include <Common/ZeroMQWrapper/IContextSharedPtr.h>
#include <Common/ZeroMQWrapper/ISocketReplier.h>
#include <Common/ZeroMQWrapper/ISocketReplierPtr.h>

namespace watchdog
{
    namespace watchdogimpl
    {
        using PluginInfoVector = Common::PluginRegistryImpl::PluginInfoVector;
        using ProxyVector = std::vector<watchdog::watchdogimpl::PluginProxy>;

        class Watchdog
        {
        public:
            explicit Watchdog();
            explicit Watchdog(Common::ZeroMQWrapper::IContextSharedPtr context);
            ~Watchdog();
            int run();
            PluginInfoVector read_plugin_configs();
        protected:
            std::string getIPCPath();
            void setupSocket();
            void handleSocketRequest();
            std::string handleCommand(Common::ZeroMQWrapper::IReadable::data_t input);

            std::string stopPlugin(const std::string& pluginName);

            Common::ZeroMQWrapper::IContextSharedPtr m_context;
            ProxyVector m_pluginProxies;
        private:
            Common::ZeroMQWrapper::ISocketReplierPtr m_socket;
        };
    }
}
