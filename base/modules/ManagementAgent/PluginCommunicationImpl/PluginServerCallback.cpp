/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "PluginServerCallback.h"
#include <ManagementAgent/LoggerImpl/Logger.h>
#include <Common/PluginApi/ApiException.h>
#include <Common/PluginRegistryImpl/PluginInfo.h>

namespace ManagementAgent
{
    namespace PluginCommunicationImpl
    {
        PluginServerCallback::PluginServerCallback(PluginManager & pluginManagerPtr)
                : m_pluginManager(pluginManagerPtr)
        {}

        void PluginServerCallback::receivedSendEvent(const std::string& appId, const std::string &eventXml)
        {
            LOGSUPPORT("Management: Event received for  " << appId);
            if (m_eventReceiver != nullptr)
            {
                m_eventReceiver->receivedSendEvent(appId, eventXml);
            }
        }

        void PluginServerCallback::receivedChangeStatus(const std::string& appId, const Common::PluginApi::StatusInfo &statusInfo)
        {
            LOGSUPPORT("Management: Status changed notification for " << appId);
            if (m_statusReceiver != nullptr)
            {
                m_statusReceiver->receivedChangeStatus(appId, statusInfo);
            }
        }

        bool PluginServerCallback::receivedGetPolicyRequest(const std::string& appId)
        {
            LOGSUPPORT("Management: Policy request received for  " << appId);
            if(m_policyReceiver != nullptr)
            {
                return m_policyReceiver->receivedGetPolicyRequest(appId);
            }

            return false;
        }

        void PluginServerCallback::receivedRegisterWithManagementAgent(const std::string &pluginName)
        {
            LOGSUPPORT("Plugin registration received for plugin: " << pluginName);
            Common::PluginRegistryImpl::PluginInfo pluginInfo;
            bool validPlugin;
            std::tie(pluginInfo, validPlugin) =
                    Common::PluginRegistryImpl::PluginInfo::loadPluginInfoFromRegistry(pluginName);

            if (!validPlugin)
            {
                // Create the pluginProxy even if we don't have a configuration,
                // so that we can handle unknown plugins registering - although we do return an error from the register RPC
                m_pluginManager.registerPlugin(pluginName);
                LOGERROR("No information found about this plugin: " << pluginName);
                throw Common::PluginApi::ApiException(
                        "Information for the plugin not found in the registry: " + pluginName
                );
            }
            else
            {
                // Create the pluginProxy
                m_pluginManager.registerAndSetAppIds(pluginName, pluginInfo.getPolicyAppIds(), pluginInfo.getStatusAppIds());
            }
        }

        void PluginServerCallback::setStatusReceiver(std::shared_ptr<PluginCommunication::IStatusReceiver>& statusReceiver)
        {
            LOGDEBUG("Status Receiver armed");
            m_statusReceiver = statusReceiver;
        }

        void PluginServerCallback::setEventReceiver(std::shared_ptr<PluginCommunication::IEventReceiver>& receiver)
        {
            LOGDEBUG("Event Receiver armed");
            m_eventReceiver = receiver;
        }

        void PluginServerCallback::setPolicyReceiver(std::shared_ptr<PluginCommunication::IPolicyReceiver>& receiver)
        {
            LOGDEBUG("Policy Receiver armed");
            m_policyReceiver = receiver;
        }
    }
}
