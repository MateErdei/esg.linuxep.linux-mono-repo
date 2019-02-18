/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "StatusInfo.h"

#include <string>

namespace Common
{
    namespace PluginApi
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
             * @param policyXml: either the policy xml content or its translation.
             * @todo LINUXEP-5950: add information about 'translating xml'
             * @throw Implementers of IPluginCallback may decide to throw ApiException to report error in applying new
             * policy.
             */
            virtual void applyNewPolicy(const std::string& policyXml) = 0;

            /**
             * Require the plugin to perform an action ( Sent by Management Console ).
             *
             * @attention the action request is queued to be executed at a later time after the method returns.
             * @param actionXml: either the action xml content or its translation.
             * @todo LINUXEP-5950: add information about 'translating xml'
             * @throw Implementers of IPluginCallback may decide to throw ApiException to report error in the requested
             * action.
             */
            virtual void queueAction(const std::string& actionXml) = 0;

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
        };

    } // namespace PluginApi
} // namespace Common
