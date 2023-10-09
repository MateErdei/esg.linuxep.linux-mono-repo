/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "Common/PluginApi/StatusInfo.h"
#include "Common/PluginProtocol/DataMessage.h"

namespace Common
{
    namespace PluginCommunication
    {
        class IPluginProxy
        {
        public:
            virtual ~IPluginProxy() = default;

            /**
             * Send request to the plugin that it should apply the policy.
             *
             * @param appId
             * @param policyXml
             */
            virtual void applyNewPolicy(const std::string& appId, const std::string& policyXml) = 0;

            /**
             * Send request to the plugin that it should perform the action.
             *
             * @param appId
             * @param actionXml
             */
            virtual void queueAction(
                const std::string& appId,
                const std::string& actionXml,
                const std::string& correlationId) = 0;

            /**
             * Query the plugin for the status of each appId that the plugin support.
             * @see setStatusAppIds
             *
             * @return Status XML
             */
            virtual std::vector<Common::PluginApi::StatusInfo> getStatus() = 0;

            /**
             * Query the plugin for its telemetry
             *
             * @return Telemetry string
             */
            virtual std::string getTelemetry() = 0;

            /**
             * Query the plugin for its health
             * @return Health JSON
             */
            virtual std::string getHealth() = 0;

            /**
             * Change the app IDs that the plugin is interested in for policy
             *
             * @param appIds
             */
            virtual void setPolicyAppIds(const std::vector<std::string>& appIds) = 0;

            /**
             * Change the app IDs that the plugin is interested in for actions
             *
             * @param appIds
             */
            virtual void setActionAppIds(const std::vector<std::string>& appIds) = 0;

            /**
             * Set the app IDs that the plugin claims to be able to provide status for.
             *
             * @param appIds
             */
            virtual void setStatusAppIds(const std::vector<std::string>& appIds) = 0;

            /**
             *
             * @param appId
             * @return true if the plugin is interested in the appId given for policy
             */
            virtual bool hasPolicyAppId(const std::string& appId) = 0;
            /**
             *
             * @param appId
             * @return true if the plugin is interested in the appId given for action
             */
            virtual bool hasActionAppId(const std::string& appId) = 0;
            /**
             *
             * @param appId
             * @return true if the plugin is interested in the appId given for status
             */
            virtual bool hasStatusAppId(const std::string& appId) = 0;

            /**
             * Get the plugin's name
             * @return
             */
            virtual std::string name() = 0;

            /**
             * Set if Service Health needs to be gathered or not
             * @param serviceHealth
             */
            virtual void setServiceHealth(bool serviceHealth) = 0;

            /**
             * Set if Threat Service Health needs to be gathered or not
             * @param serviceHealth
             */
            virtual void setThreatServiceHealth(bool threatServiceHealth) = 0;

            /**
             * Set the display name for the plugin
             * @param displayPluginName
             */
            virtual void setDisplayPluginName(const std::string& displayPluginName) = 0;

            /**
             *
             * @param serviceHealth
             * @return true if service health should be gathered, false otherwiese
             */
            virtual bool getServiceHealth() const = 0;

            /**
             *
             * @param threatServiceHealth
             * @return true if service health should be gathered, false otherwiese
             */
            virtual bool getThreatServiceHealth() const = 0;

            /**
             * @return string to be used as the display name for the plugin
             */
            virtual std::string getDisplayPluginName() const = 0;

        };

    } // namespace PluginCommunication
} // namespace Common
