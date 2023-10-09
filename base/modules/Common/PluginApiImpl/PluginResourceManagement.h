// Copyright 2018-2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/PluginApi/IPluginResourceManagement.h"
#include "Common/ZMQWrapperApi/IContextSharedPtr.h"
#include "Common/ZeroMQWrapper/ISocketReplier.h"
#include "Common/ZeroMQWrapper/ISocketRequester.h"
#include "Common/ZeroMQWrapper/ISocketSetup.h"

namespace Common
{
    namespace PluginApiImpl
    {
        class PluginResourceManagement : public virtual Common::PluginApi::IPluginResourceManagement
        {
        public:
            PluginResourceManagement();
            explicit PluginResourceManagement(Common::ZMQWrapperApi::IContextSharedPtr);

            static void setupReplier(
                Common::ZeroMQWrapper::ISocketReplier& replier,
                const std::string& pluginName,
                int defaultTimeout,
                int connectTimeout);
            static void setupRequester(
                Common::ZeroMQWrapper::ISocketRequester& requester,
                const std::string& pluginName,
                int defaultTimeout,
                int connectTimeout);

            std::unique_ptr<Common::PluginApi::IBaseServiceApi> createPluginAPI(
                const std::string& pluginName,
                std::shared_ptr<Common::PluginApi::IPluginCallbackApi> pluginCallback) override;

            /* mainly for tests */
            void setDefaultTimeout(int timeoutMs);
            void setDefaultConnectTimeout(int timeoutMs);
            Common::ZMQWrapperApi::IContextSharedPtr getSocketContext();

        private:
            static void setTimeouts(
                Common::ZeroMQWrapper::ISocketSetup& socket,
                int defaultTimeout,
                int ConnectTimeOut);
            void setTimeouts(Common::ZeroMQWrapper::ISocketSetup& socket);
            Common::ZMQWrapperApi::IContextSharedPtr m_contextPtr;
            int m_defaultTimeout;
            int m_defaultConnectTimeout;
        };

    } // namespace PluginApiImpl
} // namespace Common
