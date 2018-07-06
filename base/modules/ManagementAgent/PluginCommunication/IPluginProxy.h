/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#ifndef EVEREST_BASE_IPLUGINPROXY_H
#define EVEREST_BASE_IPLUGINPROXY_H

#include "Common/PluginProtocol/DataMessage.h"

namespace ManagementAgent
{
namespace PluginCommunication
{
    class IPluginProxy
    {
    public:
        virtual void applyNewPolicy(const std::string &appId, const std::string &policyXml) = 0;

        virtual void doAction(const std::string &appId, const std::string &actionXml) = 0;

        virtual Common::PluginProtocol::StatusInfo getStatus() = 0;

        virtual std::string getTelemetry() = 0;

        virtual void setAppIds(const std::vector<std::string> &appIds) = 0;

        virtual bool hasAppId(const std::string &appId);
    };



}
}

#endif //EVEREST_BASE_IPLUGINPROXY_H
