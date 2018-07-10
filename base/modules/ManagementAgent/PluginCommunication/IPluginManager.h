/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#ifndef EVEREST_BASE_IPLUGINMANAGER_H
#define EVEREST_BASE_IPLUGINMANAGER_H

#include <string>
#include "IPluginProxy.h"
#include "IPluginServerCallback.h"
#include "Common/PluginProtocol/DataMessage.h"

namespace ManagementAgent
{
namespace PluginCommunication
{
    /**
     * Plugin Manager should handle all communication with the plugins.
     *
     * It should therefore start up a callback handler to handle requests from a plugin, and also
     * set up 'Plugin Proxies' which can send requests to the plugins.
     */
    class IPluginManager
    {
    public:
        virtual ~IPluginManager() = default;

        /**
         * Send policy to all plugins which are interested in appID
         *
         * @param appId
         * @param policyXml
         */
        virtual void applyNewPolicy(const std::string &appId, const std::string &policyXml) = 0;

        /**
         * Send actions to all plugins which are interested in appID
         *
         * @param appId
         * @param actionXml
         */
        virtual void doAction(const std::string &appId, const std::string &actionXml) = 0;

        /**
         * Request that the plugin should send its status
         *
         * @param pluginName
         * @return Status XML
         */
        virtual Common::PluginApi::StatusInfo getStatus(const std::string &pluginName) = 0;

        /**
         * Request that the plugin should send telemetry
         *
         * @param pluginName
         * @return Telemetry string
         */
        virtual std::string getTelemetry(const std::string &pluginName) = 0;

        /**
         * Set the app ids that 'pluginName' cares about
         *
         * @param pluginName
         * @param appIds
         */
        virtual void setAppIds(const std::string& pluginName, const std::vector<std::string> &appIds) = 0;

        /**
         * Register a new plugin.
         * Keep the plugin in memory so we can send actions/policies/requests to it.
         *
         * @param pluginName
         */
        virtual void registerPlugin(const std::string &pluginName) = 0;

        /**
         * Remove the plugin from memory so we no longer send actions/policies/requests to it.
         *
         * @param pluginName
         */
        virtual void removePlugin(const std::string &pluginName) = 0;
    };
}
}

#endif //EVEREST_BASE_IPLUGINMANAGER_H

