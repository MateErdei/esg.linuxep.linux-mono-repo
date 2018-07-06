/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#ifndef EVEREST_BASE_PLUGINSERVERCALLBACK_H
#define EVEREST_BASE_PLUGINSERVERCALLBACK_H


#include <string>
#include "IPluginServerCallback.h"
#include "PluginManager.h"
#include "Common/PluginProtocol/DataMessage.h"

namespace ManagementAgent
{
    namespace PluginCommunicationImpl
    {
        class PluginServerCallback : virtual public PluginCommunication::IPluginServerCallback
        {
        public:
            PluginServerCallback(std::shared_ptr<PluginManager> pluginManagerPtr);
            void receivedSendEvent(const std::string& appId, const std::string &eventXml) override;
            void receivedChangeStatus(const std::string& appId, const Common::PluginProtocol::StatusInfo &statusInfo) override;
            void shutdown() override;
            std::string receivedGetPolicy(const std::string &pluginName) override;
            void receivedRegisterWithManagementAgent(const std::string &pluginName) override;
        private:
            std::shared_ptr<PluginManager> m_pluginManagerPtr;
        };
    }
}


#endif //EVEREST_BASE_PLUGINSERVERCALLBACK_H
