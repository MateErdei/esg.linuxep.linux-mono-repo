/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once



#include <string>
#include <map>
#include <mutex>
#include "Common/ZeroMQWrapper/ISocketReplierPtr.h"
#include "Common/ZeroMQWrapper/IContextPtr.h"
#include "IPluginManager.h"
#include "IPluginProxy.h"
#include "PluginServerCallbackHandler.h"
#include "IPluginServerCallback.h"


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
            std::vector<Common::PluginApi::StatusInfo> getStatus(const std::string & pluginName) override;
            std::string getTelemetry(const std::string & pluginName) override;
            void setAppIds(const std::string &pluginName, const std::vector<std::string> &policyAppIds, const std::vector<std::string> & statusAppIds) override;
            void registerPlugin(const std::string &pluginName) override;
            void removePlugin(const std::string &pluginName) override;

            /**
             * Used mainly for Tests
             */
            void setDefaultTimeout(int timeoutMs);
            void setDefaultConnectTimeout(int timeoutMs);

            Common::ZeroMQWrapper::IContext & getSocketContext();
            void setServerCallback(std::shared_ptr<PluginCommunication::IPluginServerCallback> pluginCallback, Common::ZeroMQWrapper::ISocketReplierPtr replierPtr);

            void setStatusReceiver(std::shared_ptr<PluginCommunication::IStatusReceiver>& statusReceiver) override;

            void setEventReceiver(std::shared_ptr<PluginCommunication::IEventReceiver>& receiver) override;

        private:

            PluginCommunication::IPluginProxy* getPlugin(const std::string &pluginName);

            void setTimeouts(Common::ZeroMQWrapper::ISocketSetup &socket);

            Common::ZeroMQWrapper::IContextPtr m_context;
            std::map<std::string, std::unique_ptr<PluginCommunication::IPluginProxy>> m_RegisteredPlugins;
            std::unique_ptr<PluginServerCallbackHandler> m_serverCallbackHandler;

            int m_defaultTimeout;
            int m_defaultConnectTimeout;
            std::mutex m_pluginMapMutex;
        };

    }
}




