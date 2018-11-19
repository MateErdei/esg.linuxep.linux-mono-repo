/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once



#include "PluginServerCallbackHandler.h"

#include <ManagementAgent/PluginCommunication/IPluginManager.h>
#include <ManagementAgent/PluginCommunication/IPluginProxy.h>
#include <ManagementAgent/PluginCommunication/IPluginServerCallback.h>

#include <Common/ZeroMQWrapper/ISocketReplierPtr.h>
#include <Common/ZeroMQWrapper/IContextSharedPtr.h>

#include <string>
#include <map>
#include <mutex>


namespace ManagementAgent
{
    namespace PluginCommunicationImpl
    {
        class PluginManager : virtual public PluginCommunication::IPluginManager
        {

        public:

            PluginManager();
            ~PluginManager() override;


            int applyNewPolicy(const std::string &appId, const std::string &policyXml) override;
            int queueAction(const std::string &appId, const std::string &actionXml) override;
            void checkPluginRegistry(const std::vector<std::pair<std::string, std::string>>& pluginsAndErrors);
            std::vector<Common::PluginApi::StatusInfo> getStatus(const std::string & pluginName) override;
            std::string getTelemetry(const std::string & pluginName) override;

            void registerPlugin(const std::string &pluginName) override;
            void registerAndSetAppIds(const std::string &pluginName, const std::vector<std::string> &policyAppIds, const std::vector<std::string> & statusAppIds) override;

            void removePlugin(const std::string &pluginName) override;

            std::vector<std::string> getRegisteredPluginNames() override;

            Common::ZeroMQWrapper::IContextSharedPtr getSocketContext();
            void setServerCallback(std::shared_ptr<PluginCommunication::IPluginServerCallback> pluginCallback, Common::ZeroMQWrapper::ISocketReplierPtr replierPtr);

            void setStatusReceiver(std::shared_ptr<PluginCommunication::IStatusReceiver>& statusReceiver) override;

            void setEventReceiver(std::shared_ptr<PluginCommunication::IEventReceiver>& receiver) override;

            void setPolicyReceiver(std::shared_ptr<PluginCommunication::IPolicyReceiver>& receiver) override;

            /**
             * Used mainly for Tests
             */
            void setDefaultTimeout(int timeoutMs);
            void setDefaultConnectTimeout(int timeoutMs);

        private:

            PluginCommunication::IPluginProxy* getPlugin(const std::string& pluginName);

            void setTimeouts(Common::ZeroMQWrapper::ISocketSetup& socket);

            Common::ZeroMQWrapper::IContextSharedPtr m_context;
            std::map<std::string, std::unique_ptr<PluginCommunication::IPluginProxy>> m_RegisteredPlugins;
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
            PluginCommunication::IPluginProxy* locked_createPlugin(
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
                    PluginCommunication::IPluginProxy* plugin,
                    const std::vector<std::string> &policyAppIds,
                    const std::vector<std::string> & statusAppIds,
                    std::lock_guard<std::mutex>& lock);
        };

    }
}




