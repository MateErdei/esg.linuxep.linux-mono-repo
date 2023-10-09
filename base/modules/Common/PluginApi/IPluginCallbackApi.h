/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "StatusInfo.h"

#include <string>

namespace Common::PluginApi
{
    /**
     * The IPluginCallback is the class that developers of Plugins will implement and pass to the
     * IPluginResourceManagement when creating an IPluginApi. It is related to the services that Management Agent
     * and the product expects from any Plugin in the system.
     *
     * The methods defined here will be called when the respective services are required from Management Agent and
     * the reply will be forwarded to the Management Agent via ipc channel.
     *
     * @attention all callback functions should aim to take no longer than 500 milliseconds to prevent performance
     * issues.
     */
    class IPluginCallbackApi
    {
    public:
        virtual ~IPluginCallbackApi() = default;

        /**
         * Require the plugin to apply a policy (Sent by Management Console).
         *
         * @attention the policy may be applied at a later time after the method returns.
         * @param policyXml: either the policy xml content
         * @throw Implementers of IPluginCallback may decide to throw ApiException to report error in applying new
         * policy.
         */
        virtual void applyNewPolicy(const std::string& policyXml) = 0;
        virtual void applyNewPolicyWithAppId(const std::string& appId, const std::string& policyXml)
        {
            (void)appId;
            applyNewPolicy(policyXml);
        };

        /**
         * Require the plugin to perform an action ( Sent by Management Console ).
         *
         * @attention the action request is queued to be executed at a later time after the method returns.
         * @param actionXml: either the action xml content
         * @throw Implementers of IPluginCallback may decide to throw ApiException to report error in the requested
         * action.
         */
        virtual void queueAction(const std::string& actionXml) = 0;
        virtual void queueActionWithCorrelation(const std::string& content, const std::string& correlationId)
        {
            (void)correlationId;
            queueAction(content);
        };

        /**
         *  Method that will be called when the Plugin receives a SIGTERM or SIGINT (@see
         * Reactor::setShutdownListener). This gives the Plugin the opportunity to shutdown cleanly. After this
         * method is called, the internal thread that servers the requests will be stopped and the expectation is
         * that the Plugin process will stop as soon as possible.
         *
         */
        virtual void onShutdown() = 0;

        /**
         * Method that will be called when Management Agent queries about the status of the Plugin when it needs the
         * information to send to Management Console.
         *
         * Plugin developer must implement this function to provide StatusInfo when requested.
         *
         * @return StatusInfo The content of StatusInfo is forwarded to the Management Agent via the ipc channel.
         */
        virtual PluginApi::StatusInfo getStatus(const std::string& appId) = 0;

        /**
         * Plugin developer must implement this function to provide the telemetry information relevant to the
         * Plugin. This method is called when a query to telemetry is received via the ipc channel.
         * @return The content of the telemetry to be forwarded via the ipc channel.
         */
        virtual std::string getTelemetry() = 0;

        /**
         * Plugin developer must implement this function to provide the health (SHS) information relevant to the
         * plugin. This method is called when a Management Agent requests the health of a plugin via the ipc channel.
         * @return  JSON string containing Service Health, Threat Service Health and Threat Health.
         *          returns a json containing either:
         *          {"Health": X, "UTM_ID": <ID>} //utmid may not be included
         *          {"ThreatHealth" : X}
         *          where x is 1/2/3 based on the health of the plugin
         *
         *          {} means the plugin has not implemented it's own getHealth and should be ignored
         */
        virtual std::string getHealth() = 0;
    };
} // namespace Common::PluginApi
