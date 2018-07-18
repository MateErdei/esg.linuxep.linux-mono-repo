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
            // Write file to directory
        }

        void PluginServerCallback::receivedChangeStatus(const std::string& appId, const Common::PluginApi::StatusInfo &statusInfo)
        {
//
//            if (m_statusCache.statusChanged(appId,statusInfo.statusWithoutTimestampsXml))
//            {
//                // write file to directory
//            }
        }

        std::string PluginServerCallback::receivedGetPolicy(const std::string &appId)
        {
            return std::string();
        }

        void PluginServerCallback::receivedRegisterWithManagementAgent(const std::string &pluginName)
        {
            m_pluginManagerPtr.registerPlugin(pluginName);
        }
    }
}
