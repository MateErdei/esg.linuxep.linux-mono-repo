/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#ifndef EVEREST_BASE_PLUGINMANAGER_H
#define EVEREST_BASE_PLUGINMANAGER_H


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
            ~PluginManager();

            void setDefaultTimeout(int timeoutMs) override;
            void setDefaultConnectTimeout(int timeoutMs) override;

            void setServerCallback(std::shared_ptr<PluginCommunication::IPluginServerCallback> pluginCallback, Common::ZeroMQWrapper::ISocketReplierPtr replierPtr) override;


            void applyNewPolicy(const std::string &appId, const std::string &policyXml) override;
            void doAction(const std::string &appId, const std::string &actionXml) override;
            Common::PluginProtocol::StatusInfo getStatus(const std::string & pluginName) override;
            std::string getTelemetry(const std::string & pluginName) override;
            void setAppIds(const std::string &pluginName, const std::vector<std::string> &appIds) override;

        public:

            void registerPlugin(const std::string &pluginName) override;
            void removePlugin(const std::string &pluginName) override;

        private:

            std::unique_ptr<PluginCommunication::IPluginProxy>& getPlugin(const std::string &pluginName);

            void setTimeouts(Common::ZeroMQWrapper::ISocketSetup &socket);

            std::map<std::string, std::unique_ptr<PluginCommunication::IPluginProxy>> m_RegisteredPlugins;
            std::unique_ptr<PluginServerCallbackHandler> m_serverCallbackHandler;
            Common::ZeroMQWrapper::IContextPtr m_context;

            int m_defaultTimeout;
            int m_defaultConnectTimeout;
            std::mutex m_pluginMapMutex;
        };

    }
}



#endif //EVEREST_BASE_PLUGINMANAGER_H
