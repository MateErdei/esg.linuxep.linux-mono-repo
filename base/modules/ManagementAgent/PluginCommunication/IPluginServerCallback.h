/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#ifndef MANAGEMENTAGENT_PLUGINCOMMUNICATION_IPLUGINSERVERCALLBACK_H
#define MANAGEMENTAGENT_PLUGINCOMMUNICATION_IPLUGINSERVERCALLBACK_H

#include "IStatusReceiver.h"

#include <Common/PluginApi/IBaseServiceApi.h>
#include <Common/PluginProtocol/DataMessage.h>
#include <string>

namespace ManagementAgent
{
namespace PluginCommunication
{
    class IPluginServerCallback : public virtual IStatusReceiver
    {
    public:
        ~IPluginServerCallback() override = default;

        /**
         * Handle the situation where a plugin is sending an event to the management agent
         *
         * @param appId
         * @param eventXml
         */
        virtual void receivedSendEvent(const std::string& appId, const std::string &eventXml) = 0;

        /**
         * Plugin is querying for the policy associated with appId, return the current policy.
         *
         * @param appId
         * @return Policy XML
         */
        virtual std::string receivedGetPolicy(const std::string &appId) = 0;

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
