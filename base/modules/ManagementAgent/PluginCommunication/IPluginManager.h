// Copyright 2018-2023 Sophos Limited. All rights reserved.

#pragma once

#include "IPluginServerCallback.h"
#include "IPolicyReceiver.h"
#include "IStatusReceiver.h"

#include "Common/PluginCommunication/IPluginProxy.h"
#include "Common/PluginProtocol/DataMessage.h"
#include "Common/PluginRegistryImpl/PluginInfo.h"
#include "ManagementAgent/EventReceiver/IEventReceiver.h"
#include "ManagementAgent/HealthStatusCommon/PluginHealthStatus.h"

#include <chrono>
#include <memory>
#include <string>

using timepoint_t = std::chrono::steady_clock::time_point;

namespace ManagementAgent::PluginCommunication
{
    class PluginDetails
    {
    public:
        PluginDetails(const Common::PluginRegistryImpl::PluginInfo& pluginInfo) :
            policyAppIds(pluginInfo.getPolicyAppIds()),
            actionAppIds(pluginInfo.getActionAppIds()),
            statusAppIds(pluginInfo.getStatusAppIds()),
            hasServiceHealth(pluginInfo.getHasServiceHealth()),
            hasThreatServiceHealth(pluginInfo.getHasThreatServiceHealth()),
            displayName(pluginInfo.getDisplayPluginName())
        {
        }

        bool operator==(const PluginDetails& rhs) const
        {
            if ((policyAppIds == rhs.policyAppIds) && (actionAppIds == rhs.actionAppIds) &&
                (statusAppIds == rhs.statusAppIds) && (hasServiceHealth == rhs.hasServiceHealth) &&
                (hasThreatServiceHealth == rhs.hasThreatServiceHealth) && (displayName == rhs.displayName))
            {
                return true;
            }

            return false;
        }

        std::vector<std::string> policyAppIds;
        std::vector<std::string> actionAppIds;
        std::vector<std::string> statusAppIds;
        bool hasServiceHealth = false;
        bool hasThreatServiceHealth = false;
        std::string displayName;
    };

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
        virtual int applyNewPolicy(
            const std::string& appId,
            const std::string& policyXml,
            const std::string& pluginName) = 0;

        /**
         * Send actions to all plugins which are interested in appID
         *
         * @param appId
         * @param actionXml
         * @param correlationId  refers to the tag id of the command sent from Central. It will be empty if not
         * available
         * @return Number of plugins that received the action.
         * @note In case of error
         */
        virtual int queueAction(
            const std::string& appId,
            const std::string& actionXml,
            const std::string& correlationId) = 0;

        /**
         * Check if a plugin is still installed
         *
         * @param pluginName
         */
        virtual bool checkIfSinglePluginInRegistry(const std::string& pluginName) = 0;

        /**
         * Query the plugin for the status of each appId that the plugin named pluginName support.
         *
         * @return Status XML
         */
        virtual std::vector<Common::PluginApi::StatusInfo> getStatus(const std::string& pluginName) = 0;

        /**
         * Request that the plugin should send telemetry
         *
         * @param pluginName
         * @return Telemetry string
         */
        virtual std::string getTelemetry(const std::string& pluginName) = 0;

        /**
         * Query the health of the plugin named pluginName.
         *
         * @return Health JSON
         */
        virtual std::string getHealth(const std::string& pluginName) = 0;

        /**
         * Register a new plugin.
         * Keep the plugin in memory so we can send actions/policies/requests to it.
         *
         * @param pluginName
         */
        virtual void registerPlugin(const std::string& pluginName) = 0;

        /**
         * Register a new plugin.
         * Keep the plugin in memory so we can send actions/policies/requests to it.
         * Set the app ids that 'pluginName' cares about
         *
         * @param pluginName
         * @param appIds
         */
        virtual void registerAndConfigure(const std::string& pluginName, const PluginDetails& pluginDetails) = 0;

        /**
         * Remove the plugin from memory so we no longer send actions/policies/requests to it.
         *
         * @param pluginName
         */
        virtual void removePlugin(const std::string& pluginName) = 0;

        /**
         * Gets the names of the registered plugins
         * @return list registered plugin names
         */
        virtual std::vector<std::string> getRegisteredPluginNames() = 0;

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
        virtual void setEventReceiver(std::shared_ptr<IEventReceiver> receiver) = 0;

        /**
         * Set the policy receiver for polices requests from plugins, if using default callback handler
         *
         * @param receiver
         */
        virtual void setPolicyReceiver(std::shared_ptr<IPolicyReceiver>& receiver) = 0;

        /**
         * Set the Threat Health receiver for Threat Health updates from plugins
         * @param receiver
         */
        virtual void setThreatHealthReceiver(std::shared_ptr<PluginCommunication::IThreatHealthReceiver>& receiver) = 0;

        /**
         * used to identify if a plugin needs to contribute to periodic health check
         * @param pluginName
         * @param prevHealthMissing refers to whether the previous health status was missing
         * @return true if should contribute, false otherwise
         */
        virtual ManagementAgent::PluginCommunication::PluginHealthStatus getHealthStatusForPlugin(
            const std::string& pluginName,
            bool prevHealthMissing) = 0;

        /**
         * An accessor to expose the shared health object. Needed so that other parts of
         * Management Agent can interact with this Health object.
         */
        virtual std::shared_ptr<ManagementAgent::HealthStatusImpl::HealthStatus> getSharedHealthStatusObj() = 0;

        /**
         * Are we in a graceperiod during an update (or during the update itself)
         * @param gracePeriodSeconds
         * @param now
         * @return True if within grace period
         */
        virtual bool updateOngoingWithGracePeriod(unsigned int gracePeriodSeconds, timepoint_t now) = 0;
    };
} // namespace ManagementAgent::PluginCommunication
