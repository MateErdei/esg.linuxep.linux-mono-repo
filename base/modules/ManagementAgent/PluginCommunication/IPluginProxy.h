/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#ifndef EVEREST_BASE_IPLUGINPROXY_H
#define EVEREST_BASE_IPLUGINPROXY_H

#include "Common/PluginApi/StatusInfo.h"
#include "Common/PluginProtocol/DataMessage.h"

namespace ManagementAgent
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
        virtual void applyNewPolicy(const std::string &appId, const std::string &policyXml) = 0;

        /**
         * Send request to the plugin that it should perform the action.
         *
         * @param appId
         * @param actionXml
         */
        virtual void doAction(const std::string &appId, const std::string &actionXml) = 0;

        /**
         * Query the plugin for its status
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
         * Change the app IDs that the plugin is interested in.
         *
         * @param appIds
         */
        virtual void setAppIds(const std::vector<std::string> &appIds) = 0;

        /**
         *
         * @param appId
         * @return true if the plugin is interested in the appId given
         */
        virtual bool hasAppId(const std::string &appId) = 0;
    };



}
}

#endif //EVEREST_BASE_IPLUGINPROXY_H
