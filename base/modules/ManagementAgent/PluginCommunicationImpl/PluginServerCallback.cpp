// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "PluginServerCallback.h"

#include "Common/PluginApi/ApiException.h"
#include "Common/PluginRegistryImpl/PluginInfo.h"
#include "ManagementAgent/LoggerImpl/Logger.h"

namespace ManagementAgent::PluginCommunicationImpl
{
    PluginServerCallback::PluginServerCallback(PluginCommunication::IPluginManager& pluginManagerPtr)
            : m_pluginManager(pluginManagerPtr)
    {
    }

    void PluginServerCallback::receivedSendEvent(const std::string& appId, const std::string& eventXml)
    {
        LOGDEBUG("Management: Event received for  " << appId);
        if (m_eventReceiver)
        {
            m_eventReceiver->receivedSendEvent(appId, eventXml);
            m_pluginManager.getSharedHealthStatusObj()->setOutbreakMode(m_eventReceiver->outbreakMode());
        }
    }

    void PluginServerCallback::handleAction(const std::string& actionXml)
    {
        if (m_eventReceiver)
        {
            m_eventReceiver->handleAction(actionXml);
        }
        else
        {
            LOGFATAL("handleAction called without m_eventReceiver set!");
            assert(m_eventReceiver); // Fail in debug mode
        }
    }

    void PluginServerCallback::receivedChangeStatus(
        const std::string& appId,
        const Common::PluginApi::StatusInfo& statusInfo)
    {
        LOGDEBUG("Management: Status changed notification for " << appId);
        if (m_statusReceiver != nullptr)
        {
            m_statusReceiver->receivedChangeStatus(appId, statusInfo);
        }
    }

    bool PluginServerCallback::receivedGetPolicyRequest(const std::string& appId, const std::string& pluginName)
    {
        LOGDEBUG("Management: Policy request received for " << appId << " from " << pluginName);
        if (m_policyReceiver != nullptr)
        {
            return m_policyReceiver->receivedGetPolicyRequest(appId, pluginName);
        }

        return false;
    }

    void PluginServerCallback::receivedRegisterWithManagementAgent(const std::string& pluginName)
    {
        LOGINFO("Plugin registration received for plugin: " << pluginName);
        Common::PluginRegistryImpl::PluginInfo pluginInfo;
        bool validPlugin;
        std::tie(pluginInfo, validPlugin) =
            Common::PluginRegistryImpl::PluginInfo::loadPluginInfoFromRegistry(pluginName);

        if (!validPlugin)
        {
            LOGERROR("No information found about this plugin: " << pluginName);
            throw Common::PluginApi::ApiException(
                "Information for the plugin not found in the registry: " + pluginName);
        }
        // Creates the pluginProxy
        m_pluginManager.registerAndConfigure(
            pluginName, pluginInfo);
    }

    void PluginServerCallback::setStatusReceiver(
        std::shared_ptr<PluginCommunication::IStatusReceiver>& statusReceiver)
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

    void PluginServerCallback::setThreatHealthReceiver(std::shared_ptr<PluginCommunication::IThreatHealthReceiver>& receiver)
    {
        LOGDEBUG("Threat Health Receiver armed");
        m_threatHealthReceiver = receiver;
    }

    bool PluginServerCallback::receivedThreatHealth(const std::string& pluginName, const std::string& threatHealth,  std::shared_ptr<ManagementAgent::HealthStatusImpl::HealthStatus> healthStatusSharedObj)
    {
        LOGDEBUG("Received Threat Health: " << pluginName << ":" << threatHealth);
        bool successful = false;
        if (m_threatHealthReceiver != nullptr)
        {
            successful = m_threatHealthReceiver->receivedThreatHealth(pluginName, threatHealth, healthStatusSharedObj);
        }
        else
        {
            LOGWARN("ThreatHealthReceiver was not initialised, failed to process Threat Health from: " << pluginName);
        }
        return successful;
    }

    bool PluginServerCallback::outbreakMode() const
    {
        assert(m_eventReceiver);
        if (m_eventReceiver)
        {
            return m_eventReceiver->outbreakMode();
        }
        return false;
    }
} // namespace ManagementAgent::PluginCommunicationImpl
