/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "PluginManager.h"

#include <Common/PluginProtocol/DataMessage.h>
#include <ManagementAgent/PluginCommunication/IPluginServerCallback.h>
#include <ManagementAgent/PluginCommunication/IPolicyReceiver.h>

#include <string>

namespace ManagementAgent
{
    namespace PluginCommunicationImpl
    {
        class PluginServerCallback : virtual public PluginCommunication::IPluginServerCallback
        {
        public:
            explicit PluginServerCallback(PluginManager& pluginManagerPtr);
            void receivedSendEvent(const std::string& appId, const std::string& eventXml) override;
            void receivedChangeStatus(const std::string& appId, const Common::PluginApi::StatusInfo& statusInfo)
                override;

            bool receivedGetPolicyRequest(const std::string& appId) override;
            void receivedRegisterWithManagementAgent(const std::string& pluginName) override;
            bool receivedThreatHealth(const std::string& pluginName, const std::string& threatHealth,  std::shared_ptr<ManagementAgent::HealthStatusImpl::HealthStatus> healthStatusSharedObj) override;

            void setStatusReceiver(std::shared_ptr<PluginCommunication::IStatusReceiver>& statusReceiver);
            void setEventReceiver(std::shared_ptr<PluginCommunication::IEventReceiver>& receiver);
            void setPolicyReceiver(std::shared_ptr<PluginCommunication::IPolicyReceiver>& receiver);
            void setThreatHealthReceiver(std::shared_ptr<PluginCommunication::IThreatHealthReceiver>& receiver);

        private:
            PluginManager& m_pluginManager;
            std::shared_ptr<PluginCommunication::IStatusReceiver> m_statusReceiver;
            std::shared_ptr<PluginCommunication::IEventReceiver> m_eventReceiver;
            std::shared_ptr<PluginCommunication::IPolicyReceiver> m_policyReceiver;
            std::shared_ptr<PluginCommunication::IThreatHealthReceiver> m_threatHealthReceiver;
        };
    } // namespace PluginCommunicationImpl
} // namespace ManagementAgent
