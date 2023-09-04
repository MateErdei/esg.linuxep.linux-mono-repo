// Copyright 2021-2023 Sophos Limited. All rights reserved.

#pragma once

#include "IStatusReceiver.h"
#include "IThreatHealthReceiver.h"

#include "Common/PluginApi/IBaseServiceApi.h"
#include "Common/PluginProtocol/DataMessage.h"
#include "ManagementAgent/EventReceiver/IEventReceiver.h"

#include <string>

namespace ManagementAgent::PluginCommunication
{
    class IPluginServerCallback : public virtual IStatusReceiver,
                                  public virtual IEventReceiver,
                                  public virtual IThreatHealthReceiver
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
        virtual bool receivedGetPolicyRequest(const std::string& appId, const std::string& pluginName) = 0;

        /**
         * Function called when a plugin is attempting to register with the management agent.
         *
         * @param pluginName
         */
        virtual void receivedRegisterWithManagementAgent(const std::string& pluginName) = 0;
    };
} // namespace ManagementAgent::PluginCommunication
