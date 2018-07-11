/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#ifndef EVEREST_BASE_IPLUGINAPISERVERCALLBACK_H
#define EVEREST_BASE_IPLUGINAPISERVERCALLBACK_H

#include <string>
#include "Common/PluginApi/IBaseServiceApi.h"
#include "Common/PluginProtocol/DataMessage.h"

namespace ManagementAgent
{
namespace PluginCommunication
{
    class IPluginServerCallback
    {
    public:
        virtual ~IPluginServerCallback() = default;

        /**
         * Handle the situation where a plugin is sending an event to the management agent
         *
         * @param appId
         * @param eventXml
         */
        virtual void receivedSendEvent(const std::string& appId, const std::string &eventXml) = 0;

        /**
         * Handle the situation where a plugin is sending a changed status to the management agent
         *
         * @param appId
         * @param statusInfo
         */
        virtual void receivedChangeStatus(const std::string& appId, const Common::PluginApi::StatusInfo &statusInfo) = 0;

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

#endif //EVEREST_BASE_IPLUGINAPISERVERCALLBACK_H
