//
// Created by pair on 05/07/18.
//

#include "PluginServerCallback.h"

namespace ManagementAgent
{
    namespace PluginCommunicationImpl
    {
        PluginServerCallback::PluginServerCallback(std::shared_ptr<PluginManager> pluginManagerPtr)
        : m_pluginManagerPtr(pluginManagerPtr)
        {}

        void PluginServerCallback::receivedSendEvent(const std::string& appId, const std::string &eventXml)
        {

        }

        void PluginServerCallback::receivedChangeStatus(const std::string& appId, const Common::PluginApi::StatusInfo &statusInfo)
        {

        }

        void PluginServerCallback::shutdown()
        {

        }

        std::string PluginServerCallback::receivedGetPolicy(const std::string &pluginName)
        {
            return std::string();
        }

        void PluginServerCallback::receivedRegisterWithManagementAgent(const Common::PluginApi::RegistrationInfo &regInfo)
        {
            m_pluginManagerPtr->registerPlugin(regInfo);
        }
    }
}
