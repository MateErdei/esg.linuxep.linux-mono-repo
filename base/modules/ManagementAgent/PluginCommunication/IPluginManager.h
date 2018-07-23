/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#ifndef MANAGEMENTAGENT_PLUGINCOMMUNICATION_IPLUGINMANAGER_H
#define MANAGEMENTAGENT_PLUGINCOMMUNICATION_IPLUGINMANAGER_H

#include "IPluginProxy.h"
#include "IPluginServerCallback.h"
#include "IStatusReceiver.h"
#include "IEventReceiver.h"

#include <Common/PluginProtocol/DataMessage.h>

#include <string>

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
         * @return Number of plugins that were given the new policy
         * @note In case of errors in delivering the policy a message will be logged but no exception is thrown.
         */
        virtual int applyNewPolicy(const std::string &appId, const std::string &policyXml) = 0;

        /**
         * Send actions to all plugins which are interested in appID
         *
         * @param appId
         * @param actionXml
         * @return Number of plugins that received the action.
         * @note In case of error
         */
        virtual int queueAction(const std::string &appId, const std::string &actionXml) = 0;

        /**
         * Query the plugin for the status of each appId that the plugin named pluginName support.
         *
         * @return Status XML
         */
        virtual std::vector<Common::PluginApi::StatusInfo> getStatus(const std::string &pluginName) = 0;

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
        virtual void setAppIds(const std::string &pluginName, const std::vector<std::string> &policyAppIds, const std::vector<std::string> & statusAppIds) =0;

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

        /**
         * Set the status receiver for statuses from plugins, if using default callback handler
         *
         * @param statusReceiver
         */
        virtual void setStatusReceiver(std::shared_ptr<IStatusReceiver>& statusReceiver) = 0;

        /**
         * Set the event receiver for statuses from plugins, if using default callback handler
         *
         * @param receiver
         */
        virtual void setEventReceiver(std::shared_ptr<IEventReceiver>& receiver) = 0;
    };
}
}

#endif //MANAGEMENTAGENT_PLUGINCOMMUNICATION_IPLUGINMANAGER_H

