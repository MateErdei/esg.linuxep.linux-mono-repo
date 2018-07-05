//
// Created by pair on 05/07/18.
//

#ifndef EVEREST_BASE_IPLUGINPROXY_H
#define EVEREST_BASE_IPLUGINPROXY_H

#include "Common/PluginApi/IPluginCallback.h"

namespace ManagementAgent
{
namespace PluginCommunication
{
    class IPluginProxy
    {
    public:
        virtual void applyNewPolicy(const std::string &appId, const std::string &policyXml) = 0;

        virtual void doAction(const std::string &appId, const std::string &actionXml) = 0;

        virtual Common::PluginApi::StatusInfo getStatus(void) = 0;

        virtual std::string getTelemetry() = 0;
    };



}
}

#endif //EVEREST_BASE_IPLUGINPROXY_H
