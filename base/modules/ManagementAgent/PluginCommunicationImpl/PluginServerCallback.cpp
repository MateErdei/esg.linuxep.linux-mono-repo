/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "PluginServerCallback.h"
// TODO: this will be implemented in LINUXEP-5987
namespace ManagementAgent
{
    namespace PluginCommunicationImpl
    {
        PluginServerCallback::PluginServerCallback(PluginManager & pluginManagerPtr)
        : m_pluginManagerPtr(pluginManagerPtr)
        {}

        void PluginServerCallback::receivedSendEvent(const std::string& appId, const std::string &eventXml)
        {
            if (m_eventReceiver != nullptr)
            {
                m_eventReceiver->receivedSendEvent(appId, eventXml);
            }
        }

        void PluginServerCallback::receivedChangeStatus(const std::string& appId, const Common::PluginApi::StatusInfo &statusInfo)
        {
            if (m_statusReceiver != nullptr)
            {
                m_statusReceiver->receivedChangeStatus(appId, statusInfo);
            }
        }

        bool PluginServerCallback::receivedGetPolicyRequest(const std::string& appId, const std::string& policyId)
        {
            if(m_policyReceiver != nullptr)
            {
                return m_policyReceiver->receivedGetPolicyRequest(appId, policyId);
            }

            return false;
        }

        void PluginServerCallback::receivedRegisterWithManagementAgent(const std::string &pluginName)
        {
            m_pluginManagerPtr.registerPlugin(pluginName);
        }

        void PluginServerCallback::setStatusReceiver(std::shared_ptr<PluginCommunication::IStatusReceiver>& statusReceiver)
        {
            m_statusReceiver = statusReceiver;
        }

        void PluginServerCallback::setEventReceiver(std::shared_ptr<PluginCommunication::IEventReceiver>& receiver)
        {
            m_eventReceiver = receiver;
        }

        void PluginServerCallback::setPolicyReceiver(std::shared_ptr<PluginCommunication::IPolicyReceiver>& receiver)
        {
            m_policyReceiver = receiver;
        }
    }
}
