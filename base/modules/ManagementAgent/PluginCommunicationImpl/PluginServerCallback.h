/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once



#include "IPluginServerCallback.h"
#include "PluginManager.h"
#include <Common/PluginProtocol/DataMessage.h>
#include <ManagementAgent/PluginCommunication/IPolicyReceiver.h>

#include <string>

namespace ManagementAgent
{
    namespace PluginCommunicationImpl
    {
        class PluginServerCallback : virtual public PluginCommunication::IPluginServerCallback
        {
        public:
            PluginServerCallback(PluginManager & pluginManagerPtr);
            void receivedSendEvent(const std::string& appId, const std::string &eventXml) override;
            void receivedChangeStatus(const std::string& appId, const Common::PluginApi::StatusInfo &statusInfo) override;
            bool receivedGetPolicyRequest(const std::string& appId, const std::string& policyId) override;
            void receivedRegisterWithManagementAgent(const std::string &pluginName) override;
        private:
            PluginManager& m_pluginManagerPtr;
            std::shared_ptr<PluginCommunication::IPolicyReceiver> m_policyReceiver;
        };
    }
}



