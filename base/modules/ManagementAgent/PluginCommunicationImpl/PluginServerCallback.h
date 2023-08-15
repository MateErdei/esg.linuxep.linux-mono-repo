// Copyright 2018-2023 Sophos Limited. All rights reserved.

#pragma once

#include "ISettablePluginServerCallback.h"

#include "Common/PluginProtocol/DataMessage.h"
#include "ManagementAgent/PluginCommunication/IPluginManager.h"
#include "ManagementAgent/PluginCommunication/IPolicyReceiver.h"

#include <string>

namespace ManagementAgent
{
    namespace PluginCommunicationImpl
    {
        class PluginServerCallback : public ISettablePluginServerCallback
        {
        public:
            explicit PluginServerCallback(PluginCommunication::IPluginManager& pluginManagerPtr);

            void receivedSendEvent(const std::string& appId, const std::string& eventXml) override;
            void handleAction(const std::string& actionXml) override;

            void receivedChangeStatus(const std::string& appId, const Common::PluginApi::StatusInfo& statusInfo)
                override;
            bool receivedGetPolicyRequest(const std::string& appId, const std::string& pluginName) override;
            void receivedRegisterWithManagementAgent(const std::string& pluginName) override;
            bool receivedThreatHealth(const std::string& pluginName, const std::string& threatHealth,  std::shared_ptr<ManagementAgent::HealthStatusImpl::HealthStatus> healthStatusSharedObj) override;

            void setStatusReceiver(std::shared_ptr<PluginCommunication::IStatusReceiver>& statusReceiver) override;
            void setEventReceiver(std::shared_ptr<PluginCommunication::IEventReceiver>& receiver) override;
            void setPolicyReceiver(std::shared_ptr<PluginCommunication::IPolicyReceiver>& receiver) override;
            void setThreatHealthReceiver(std::shared_ptr<PluginCommunication::IThreatHealthReceiver>& receiver) override;

            bool outbreakMode() const override;

        private:
            PluginCommunication::IPluginManager& m_pluginManager;
            std::shared_ptr<PluginCommunication::IStatusReceiver> m_statusReceiver;
            std::shared_ptr<PluginCommunication::IEventReceiver> m_eventReceiver;
            std::shared_ptr<PluginCommunication::IPolicyReceiver> m_policyReceiver;
            std::shared_ptr<PluginCommunication::IThreatHealthReceiver> m_threatHealthReceiver;
        };
    } // namespace PluginCommunicationImpl
} // namespace ManagementAgent
