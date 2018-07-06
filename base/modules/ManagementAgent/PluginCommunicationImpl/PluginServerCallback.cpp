/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

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

        void PluginServerCallback::receivedRegisterWithManagementAgent(const std::string &pluginName)
        {
            m_pluginManagerPtr->registerPlugin(pluginName);
        }
    }
}
