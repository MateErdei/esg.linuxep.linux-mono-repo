/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#ifndef MANAGEMENTAGENT_PLUGINCOMMUNICATION_IPLUGINSERVERCALLBACK_H
#define MANAGEMENTAGENT_PLUGINCOMMUNICATION_IPLUGINSERVERCALLBACK_H

#include "IStatusReceiver.h"
#include "IEventReceiver.h"

#include <Common/PluginApi/IBaseServiceApi.h>
#include <Common/PluginProtocol/DataMessage.h>

#include <string>

namespace ManagementAgent
{
namespace PluginCommunication
{
    class IPluginServerCallback
            : public virtual IStatusReceiver
            , public virtual IEventReceiver
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
        virtual bool receivedGetPolicyRequest(const std::string& appId, const std::string& policyId) = 0;

        /**
         * Function called when a plugin is attempting to register with the management agent.
         *
         * @param pluginName
         */
        virtual void receivedRegisterWithManagementAgent(const std::string &pluginName) = 0;

    };
}
}

#endif //MANAGEMENTAGENT_PLUGINCOMMUNICATION_IPLUGINSERVERCALLBACK_H
