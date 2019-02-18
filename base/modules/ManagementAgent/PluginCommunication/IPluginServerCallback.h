/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "IEventReceiver.h"
#include "IStatusReceiver.h"

#include <Common/PluginApi/IBaseServiceApi.h>
#include <Common/PluginProtocol/DataMessage.h>

#include <string>

namespace ManagementAgent
{
    namespace PluginCommunication
    {
        class IPluginServerCallback : public virtual IStatusReceiver, public virtual IEventReceiver
        {
        public:
            ~IPluginServerCallback() override = default;

            /**
             * Plugin is querying for the policy associated with appId, return the current policy.
             *
             * @param appId
             * @param policyId
             * @return true if succeeding in adding policy request to Task Queue, false otherwise.  False will normally
             *         mean that there is no local policy file that can be re-processed.
             */
            virtual bool receivedGetPolicyRequest(const std::string& appId) = 0;

            /**
             * Function called when a plugin is attempting to register with the management agent.
             *
             * @param pluginName
             */
            virtual void receivedRegisterWithManagementAgent(const std::string& pluginName) = 0;
        };
    } // namespace PluginCommunication
} // namespace ManagementAgent
