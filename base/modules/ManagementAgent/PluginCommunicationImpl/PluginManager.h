/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "PluginServerCallbackHandler.h"

#include <Common/PluginCommunication/IPluginProxy.h>
#include <Common/ZMQWrapperApi/IContextSharedPtr.h>
#include <Common/ZeroMQWrapper/IProxy.h>
#include <Common/ZeroMQWrapper/ISocketReplierPtr.h>
#include <ManagementAgent/PluginCommunication/IPluginManager.h>
#include <ManagementAgent/PluginCommunication/IPluginServerCallback.h>

#include <map>
#include <mutex>
#include <string>

namespace ManagementAgent
{
    namespace PluginCommunicationImpl
    {
        class PluginManager : virtual public PluginCommunication::IPluginManager
        {
        public:
            PluginManager();
            ~PluginManager() override;

            int applyNewPolicy(const std::string& appId, const std::string& policyXml) override;
            int queueAction(const std::string& appId, const std::string& actionXml, const std::string& correlationId) override;
            void checkPluginRegistry(const std::vector<std::pair<std::string, std::string>>& pluginsAndErrors);
            std::vector<Common::PluginApi::StatusInfo> getStatus(const std::string& pluginName) override;
            std::string getTelemetry(const std::string& pluginName) override;

            void registerPlugin(const std::string& pluginName) override;
            void registerAndSetAppIds(
                const std::string& pluginName,
                const std::vector<std::string>& policyAppIds,
                const std::vector<std::string>& statusAppIds) override;

            void removePlugin(const std::string& pluginName) override;

            std::vector<std::string> getRegisteredPluginNames() override;

            Common::ZMQWrapperApi::IContextSharedPtr getSocketContext();
            void setServerCallback(
                std::shared_ptr<PluginCommunication::IPluginServerCallback> pluginCallback,
                Common::ZeroMQWrapper::ISocketReplierPtr replierPtr);

            void setStatusReceiver(std::shared_ptr<PluginCommunication::IStatusReceiver>& statusReceiver) override;

            void setEventReceiver(std::shared_ptr<PluginCommunication::IEventReceiver>& receiver) override;

            void setPolicyReceiver(std::shared_ptr<PluginCommunication::IPolicyReceiver>& receiver) override;

            /**
             * Used mainly for Tests
             */
            void setDefaultTimeout(int timeoutMs);
            void setDefaultConnectTimeout(int timeoutMs);

        private:
            Common::PluginCommunication::IPluginProxy* getPlugin(const std::string& pluginName);

            void setTimeouts(Common::ZeroMQWrapper::ISocketSetup& socket);

            Common::ZMQWrapperApi::IContextSharedPtr m_context;
            Common::ZeroMQWrapper::IProxyPtr m_proxy;
            std::map<std::string, std::unique_ptr<Common::PluginCommunication::IPluginProxy>> m_RegisteredPlugins;
            std::unique_ptr<PluginServerCallbackHandler> m_serverCallbackHandler;

            int m_defaultTimeout;
            int m_defaultConnectTimeout;
            std::mutex m_pluginMapMutex;

            /**
             * Create a plugin, always re-creating it
             * Must already hold m_pluginMapMutex
             * @param pluginName
             * @param lock Force a lock_guard to be passed in to verify that we have locked a mutex
             * @return BORROWED pointer to the proxy
             */
            Common::PluginCommunication::IPluginProxy* locked_createPlugin(
                const std::string& pluginName,
                std::lock_guard<std::mutex>& lock);

            /**
             *
             * Must already hold m_pluginMapMutex
             *
             * @param pluginName
             * @param policyAppIds
             * @param statusAppIds
             * @param lock Force a lock_guard to be passed in to verify that we have locked a mutex
             */
            void locked_setAppIds(
                Common::PluginCommunication::IPluginProxy* plugin,
                const std::vector<std::string>& policyAppIds,
                const std::vector<std::string>& statusAppIds,
                std::lock_guard<std::mutex>& lock);
        };

    } // namespace PluginCommunicationImpl
} // namespace ManagementAgent
