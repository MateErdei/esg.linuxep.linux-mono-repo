//
// Created by pair on 05/07/18.
//

#ifndef EVEREST_BASE_PLUGINSERVERCALLBACK_H
#define EVEREST_BASE_PLUGINSERVERCALLBACK_H


#include <string>
#include "Common/PluginApi/IPluginCallback.h"
#include "IPluginServerCallback.h"
#include "PluginManager.h"

namespace ManagementAgent
{
    namespace PluginCommunicationImpl
    {
        class PluginServerCallback : virtual public PluginCommunication::IPluginServerCallback
        {
        public:
            PluginServerCallback(std::shared_ptr<PluginManager> pluginManagerPtr);
            void receivedSendEvent(const std::string& appId, const std::string &eventXml) override;
            void receivedChangeStatus(const std::string& appId, const Common::PluginApi::StatusInfo &statusInfo) override;
            void shutdown() override;
            std::string receivedGetPolicy(const std::string &pluginName) override;
            void receivedRegisterWithManagementAgent(const Common::PluginApi::RegistrationInfo &regInfo) override;
        private:
            std::shared_ptr<PluginManager> m_pluginManagerPtr;
        };
    }
}


#endif //EVEREST_BASE_PLUGINSERVERCALLBACK_H
