//
// Created by pair on 05/07/18.
//

#ifndef EVEREST_BASE_PLUGINMANAGER_H
#define EVEREST_BASE_PLUGINMANAGER_H


#include <string>
#include <map>
#include "Common/ZeroMQWrapper/ISocketReplierPtr.h"
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

            void setServerCallback(std::shared_ptr<PluginCommunication::IPluginServerCallback> pluginCallback, Common::ZeroMQWrapper::ISocketReplierPtr replierPtr);

            void registerPlugin(std::string pluginName) override;
            std::shared_ptr<PluginCommunication::IPluginProxy> getPlugin(std::string pluginName) override;
            void removePlugin(std::string pluginName) override;

        private:
            std::map<std::string, std::shared_ptr<PluginCommunication::IPluginProxy>> m_RegisteredPlugins;
            std::unique_ptr<PluginServerCallbackHandler> m_serverCallbackHandler;
        };

    }
}



#endif //EVEREST_BASE_PLUGINMANAGER_H
